#!/usr/bin/env node
// Finds every ABI symbol a `mode: shared` extension's .so needs but the core
// php.wasm doesn't currently export to it -- WITHOUT a full Docker rebuild.
//
// Background (CLAUDE.md decisions 32/45): when a side module fails to load,
// the dynamic linker in the generated php_*.js throws on the FIRST missing
// symbol it hits ("bad export type for 'X': undefined (undefined)"), so
// finding each one has meant one Docker rebuild per newly-discovered symbol.
// This script patches a throwaway copy of the ALREADY-BUILT php_*.js so that
// throw site logs and continues instead of aborting, then loads every
// installed packages/phpext-*/ extension against it in one run -- surfacing
// every currently-missing symbol at once, so a single Dockerfile fix
// (compile/php/Dockerfile's `.JS_ABI_EXPORTS` block) can be written and
// verified with one rebuild instead of many.
//
// Usage: node compile/check-shared-extension-symbols.mjs
//
// Caveat: this only catches symbols resolved eagerly via the GOT (Global
// Offset Table) at dlopen() time -- the same class of bug every symbol found
// so far (atoll/free/strlen/stdin/stderr/strcmp/__stack_pointer/__table_base)
// turned out to be. A small number of imports are resolved lazily (only when
// actually CALLED, via a JS proxy stub) and won't show up here unless the
// code path that calls them actually runs during this script's smoke test --
// this is a strong first pass, not an absolute guarantee.
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
	const patched = original.replaceAll(THROW_MARKER, PATCHED_REPLACEMENT);
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

async function main() {
	const { jsPath, phpVersion } = findBuiltLoader();
	console.log(`Using built loader: ${jsPath} (PHP ${phpVersion})`);

	const extPackages = findInstalledExtensionPackages();
	if (extPackages.length === 0) {
		console.log('No packages/phpext-*/ with a built index.js found -- nothing to check.');
		return;
	}
	console.log(`Checking extensions: ${extPackages.join(', ')}`);

	// Dedupe by extension `name`: more than one package can bundle the same
	// dependency (e.g. both phpext-mysqli and phpext-pdo_mysql bundle their
	// own copy of mysqlnd) -- a real scanner must load each named extension
	// only once, or the runtime logs a harmless but noisy "Module 'X' is
	// already loaded" warning on the second attempt.
	const entries = [];
	const seenNames = new Set();
	for (const pkgName of extPackages) {
		const mod = await import(pathToFileURL(path.join(repoRoot, 'packages', pkgName, 'index.js')).href);
		for (const entry of mod.default(phpVersion)) {
			if (seenNames.has(entry.name)) continue;
			seenNames.add(entry.name);
			entries.push(entry);
		}
	}

	const patchedPath = writePatchedCopy(jsPath);
	try {
		const phpLoaderModule = await import(pathToFileURL(patchedPath).href);

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

		let bootError = null;
		try {
			const runtimeId = await loadPHPRuntime(phpLoaderModule.default ?? phpLoaderModule, {
				ENV: { PHP_INI_SCAN_DIR: EXTENSIONS_DIR },
				preRun: [preRun],
			});
			// Boot alone is usually enough (GOT resolution happens at dlopen time,
			// during boot) -- but run get_loaded_extensions() too, in case any
			// extension's MINIT itself needs something not caught until executed.
			const php = new PHP(runtimeId);
			await php.run({ code: '<?php get_loaded_extensions();' });
		} catch (err) {
			bootError = err;
		}

		const missing = globalThis.__missingAbiSymbols;
		console.log('');
		if (missing && missing.size > 0) {
			console.log(`Found ${missing.size} missing ABI export(s):`);
			for (const sym of missing) console.log(`  - ${sym}`);
			console.log('\nAdd each as `_<symbol>` to .JS_ABI_EXPORTS in compile/php/Dockerfile (see the');
			console.log('atoll/free/strlen/stdin/stderr/strcmp examples already there), then rebuild once.');
		} else {
			console.log('No missing ABI exports found (via GOT resolution) for the checked extensions.');
		}
		if (bootError) {
			console.log(
				`\nNote: boot raised an error after symbol collection (may be unrelated to missing symbols):\n  ${bootError.message ?? bootError}`
			);
		}
		process.exit(missing && missing.size > 0 ? 1 : 0);
	} finally {
		unlinkSync(patchedPath);
	}
}

main().catch((err) => {
	console.error('FATAL:', err);
	process.exit(1);
});
