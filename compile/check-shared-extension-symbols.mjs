#!/usr/bin/env node
// Finds every ABI symbol a `mode: shared` extension's .so needs but the core
// php.wasm doesn't currently export to it -- WITHOUT a full Docker rebuild.
//
// Background (CLAUDE.md decisions 32/45): when a side module fails to load,
// the dynamic linker in the generated php_*.js throws on the FIRST missing
// symbol it hits ("bad export type for 'X': undefined (undefined)"), so
// finding each one used to mean one Docker rebuild per newly-discovered
// symbol. This script patches a throwaway copy of the ALREADY-BUILT
// php_*.js so that throw site logs and continues instead of aborting.
//
// Two resolution paths, two patches:
//   - EAGER (GOT / Global Offset Table), resolved at dlopen() time for
//     every extension regardless of what it's actually used for.
//   - LAZY (a JS proxy stub), resolved only the first time an import is
//     actually CALLED -- e.g. dba's atoi/memcmp/strcasecmp calls are only
//     reached by really exercising dba_open()/dba_insert()/dba_fetch(),
//     not just by loading dba.so (CLAUDE.md decision 47's follow-up: the
//     first version of this script only patched the eager path, so three
//     lazy symbols for dba each needed their own manual, throwaway debug
//     script and rebuild before this file was fixed to catch them too).
//
// To actually catch lazy symbols, this script needs the checked extension's
// real code paths to run -- so for each packages/phpext-<name>/, it looks
// for compile/extensions/<name>/smoke-test.php (a small, real PHP script
// exercising that extension's actual functions) and runs it if present, in
// its own fresh runtime boot (isolated per extension, so one extension's
// crash doesn't block checking the others or corrupt their results). An
// extension with no smoke-test.php is still checked for eager/GOT symbols
// (loading it is enough for that), but lazy symbols its own functions call
// won't surface until one is written -- write one for every new mode:shared
// extension, it is what makes this tool actually work.
//
// Usage: node compile/check-shared-extension-symbols.mjs
import { readFileSync, writeFileSync, unlinkSync, readdirSync, existsSync, statSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';
import { PHP, loadPHPRuntime } from '@php-wasm/universal';

const sourceDir = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(sourceDir, '..');

const THROW_MARKER =
	"throw new Error(`bad export type for '${symName}': ${typeof value} (${value})`);";
const PATCHED_REPLACEMENT =
	"{ globalThis.__missingAbiSymbols ??= new Set(); globalThis.__missingAbiSymbols.add(symName); entry.value = 0; continue; }";

const STUB_MARKER =
	'resolved ||= resolveSymbol(prop);\n            return resolved(...args);';
const STUB_REPLACEMENT =
	'resolved ||= resolveSymbol(prop);\n            if (!resolved) { globalThis.__missingAbiSymbols ??= new Set(); globalThis.__missingAbiSymbols.add(prop); return 0; }\n            return resolved(...args);';

function findBuiltLoader() {
	const nodeBuilds = path.join(repoRoot, 'node-builds');
	if (!existsSync(nodeBuilds)) {
		throw new Error('node-builds/ does not exist -- run a build first (node compile/cli.mjs).');
	}
	for (const versionDir of readdirSync(nodeBuilds)) {
		const dir = path.join(nodeBuilds, versionDir);
		if (!statSync(dir).isDirectory()) continue;
		const jsFile = readdirSync(dir).find((f) => /^php_[\d_]+\.js$/.test(f));
		if (jsFile) {
			return { jsPath: path.join(dir, jsFile), phpVersion: versionDir.replace('-', '.') };
		}
	}
	throw new Error('No php_*.js found under node-builds/ -- run a build first.');
}

function writePatchedCopy(jsPath) {
	const original = readFileSync(jsPath, 'utf8');
	if (!original.includes(THROW_MARKER)) {
		throw new Error(
			`Could not find the expected "bad export type" throw site in ${jsPath} -- ` +
				'the Emscripten dynamic-linker runtime code this script patches may have changed; ' +
				're-verify against the new source before relying on this script again.'
		);
	}
	if (!original.includes(STUB_MARKER)) {
		throw new Error(
			`Could not find the expected lazy-symbol-stub site in ${jsPath} -- ` +
				'the Emscripten dynamic-linker runtime code this script patches may have changed; ' +
				're-verify against the new source before relying on this script again.'
		);
	}
	const patched = original
		.replaceAll(THROW_MARKER, PATCHED_REPLACEMENT)
		.replaceAll(STUB_MARKER, STUB_REPLACEMENT);
	const patchedPath = jsPath.replace(/\.js$/, '.symbolcheck.js');
	writeFileSync(patchedPath, patched, 'utf8');
	return patchedPath;
}

function findInstalledExtensionPackages() {
	const packagesDir = path.join(repoRoot, 'packages');
	if (!existsSync(packagesDir)) return [];
	return readdirSync(packagesDir).filter(
		(name) => name.startsWith('phpext-') && existsSync(path.join(packagesDir, name, 'index.js'))
	);
}

/** Boots a fresh runtime with just this package's own register() entries loaded, runs its smoke-test.php (if any), and returns whatever missing symbols surfaced. */
async function checkOnePackage(phpLoaderModule, pkgName, phpVersion) {
	const extName = pkgName.replace(/^phpext-/, '');
	const smokeTestPath = path.join(repoRoot, 'compile', 'extensions', extName, 'smoke-test.php');
	const hasSmokeTest = existsSync(smokeTestPath);

	const mod = await import(pathToFileURL(path.join(repoRoot, 'packages', pkgName, 'index.js')).href);
	const entries = mod.default(phpVersion);

	const EXTENSIONS_DIR = '/internal/shared/extensions';
	const preRun = (Module) => {
		Module.FS.mkdirTree(EXTENSIONS_DIR);
		entries.forEach((entry, i) => {
			const soBytes = readFileSync(entry.soPath);
			const soDest = `${EXTENSIONS_DIR}/${entry.name}.so`;
			Module.FS.writeFile(soDest, soBytes);
			const iniName = `${String(i).padStart(2, '0')}-${entry.name}.ini`;
			Module.FS.writeFile(`${EXTENSIONS_DIR}/${iniName}`, `extension=${soDest}\n`);
		});
	};

	globalThis.__missingAbiSymbols = new Set();
	let runtimeError = null;
	try {
		const runtimeId = await loadPHPRuntime(phpLoaderModule.default ?? phpLoaderModule, {
			ENV: { PHP_INI_SCAN_DIR: EXTENSIONS_DIR },
			preRun: [preRun],
		});
		const php = new PHP(runtimeId);
		if (hasSmokeTest) {
			const result = await php.run({ code: readFileSync(smokeTestPath, 'utf8') });
			if (result.exitCode !== 0) {
				runtimeError = new Error(`smoke-test.php exited ${result.exitCode}: ${result.text || result.errors}`);
			}
		} else {
			await php.run({ code: '<?php get_loaded_extensions();' });
		}
	} catch (err) {
		runtimeError = err;
	}

	return { hasSmokeTest, missing: globalThis.__missingAbiSymbols, runtimeError };
}

async function main() {
	const { jsPath, phpVersion } = findBuiltLoader();
	console.log(`Using built loader: ${jsPath} (PHP ${phpVersion})`);

	const extPackages = findInstalledExtensionPackages();
	if (extPackages.length === 0) {
		console.log('No packages/phpext-*/ with a built index.js found -- nothing to check.');
		return;
	}

	const patchedPath = writePatchedCopy(jsPath);
	const allMissing = new Set();
	let anyRuntimeError = false;
	try {
		const phpLoaderModule = await import(pathToFileURL(patchedPath).href);

		for (const pkgName of extPackages) {
			const { hasSmokeTest, missing, runtimeError } = await checkOnePackage(phpLoaderModule, pkgName, phpVersion);
			for (const sym of missing) allMissing.add(sym);
			const label = hasSmokeTest ? 'smoke-test.php' : 'load-only (no smoke-test.php)';
			if (runtimeError) {
				anyRuntimeError = true;
				console.log(`FAIL ${pkgName} [${label}]: ${runtimeError.message ?? runtimeError}`);
			} else {
				console.log(`OK   ${pkgName} [${label}]${missing.size ? ` -- found: ${[...missing].join(', ')}` : ''}`);
			}
		}

		console.log('');
		if (allMissing.size > 0) {
			console.log(`Found ${allMissing.size} missing ABI export(s):`);
			for (const sym of allMissing) console.log(`  - ${sym}`);
			console.log('\nAdd each as `_<symbol>` to .JS_ABI_EXPORTS in compile/php/Dockerfile (see the');
			console.log('atoll/free/strlen/stdin/stderr/strcmp/strcasecmp/atoi/memcmp examples already there), then rebuild once.');
		} else {
			console.log('No missing ABI exports found for the checked extensions.');
		}
		const missingSmokeTests = extPackages.filter(
			(p) => !existsSync(path.join(repoRoot, 'compile', 'extensions', p.replace(/^phpext-/, ''), 'smoke-test.php'))
		);
		if (missingSmokeTests.length > 0) {
			console.log(
				`\nNote: ${missingSmokeTests.join(', ')} have no smoke-test.php -- only eager/GOT symbols were ` +
					'checked for them, not symbols their own functions call lazily. Write one to check those too.'
			);
		}
		process.exit(allMissing.size > 0 || anyRuntimeError ? 1 : 0);
	} finally {
		unlinkSync(patchedPath);
	}
}

main().catch((err) => {
	console.error('FATAL:', err);
	process.exit(1);
});
