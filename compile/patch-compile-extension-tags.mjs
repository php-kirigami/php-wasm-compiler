#!/usr/bin/env node
// @php-wasm/compile-extension's own cli.js hardcodes its Docker image tags
// as "playground-php-wasm:base" / "playground-php-wasm:compile-extension-
// php<version>-<mode>" — cosmetic, but confusing once decision 45's cache
// fix (setup-compile-extension-cache.mjs) already made these images build
// from this repo's own, already-patched base-image content with zero
// network dependency on WordPress/wordpress-playground. Renamed here to
// "kirigami-compile-extension" to match, since the tag string isn't
// something the tool exposes any CLI flag or env var for — the only way
// to change it without forking the package outright is to patch the
// installed cli.js's own text, the same "patch third-party text we don't
// own, fail loudly if it's ever not there" shape as replace.sh already
// uses throughout this pipeline. compile/vendor/compile-extension-docker-
// assets/compile-extension/docker/Dockerfile.ext's own `FROM` line is kept
// in sync by hand (it's vendored, not generated) — see that file.
//
// Used two ways: as a CLI (`node patch-compile-extension-tags.mjs`) and as
// a module (`import { patchCompileExtensionTags } from
// './patch-compile-extension-tags.mjs'`, called by cli.mjs before every
// `compile-extension` run — cheap and idempotent, so it can't go stale).
import { readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

import { phpVersions as SUPPORTED_PHP_VERSIONS } from '../supported-php-versions.mjs';

const sourceDir = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(sourceDir, '..');

const OLD_NAME = 'playground-php-wasm';
const NEW_NAME = 'kirigami-compile-extension';

/**
 * The same cli.js also hardcodes its own PHP version table
 * (`{version:"8.5",...,lastRelease:"8.5.10"}`), which it uses to pick
 * the php-src tag its Docker image builds against -- frozen at whatever
 * release was current when the package was published. Left alone, every
 * shared extension silently compiles against an older php-src than the core
 * (found 2026-09-22: extensions built against 8.5.10 while the core had
 * already moved to 8.5.11). Rewritten here from supported-php-versions.mjs,
 * the same source build.js uses, so both sides always agree.
 */
function syncLastReleases(contents) {
	let result = contents;
	for (const { version, lastRelease } of SUPPORTED_PHP_VERSIONS) {
		const entry = new RegExp(
			`(\\{version:"${version.replace('.', '\\.')}",loaderFilename:"[^"]+",wasmFilename:"[^"]+",lastRelease:")[^"]+(")`
		);
		if (!entry.test(result)) {
			throw new Error(
				`@php-wasm/compile-extension's cli.js has no PHP ${version} entry in its version table -- ` +
					'the installed package was likely upgraded; re-verify this patch against its new source.'
			);
		}
		result = result.replace(entry, `$1${lastRelease}$2`);
	}
	return result;
}

/** Returns true if it actually patched the file, false if already patched. */
export function patchCompileExtensionTags() {
	const cliPath = path.join(repoRoot, 'compile', 'node_modules', '@php-wasm', 'compile-extension', 'cli.js');
	const contents = readFileSync(cliPath, 'utf8');

	let patched = contents;
	if (!patched.includes(NEW_NAME)) {
		if (!patched.includes(OLD_NAME)) {
			throw new Error(
				`@php-wasm/compile-extension's cli.js (${cliPath}) no longer contains "${OLD_NAME}" — ` +
					'the installed package was likely upgraded and this patch (CLAUDE.md decision 45) ' +
					'needs to be re-verified against its new source before relying on it again.'
			);
		}
		patched = patched.replaceAll(OLD_NAME, NEW_NAME);
	}
	patched = syncLastReleases(patched);

	if (patched === contents) {
		return false;
	}
	writeFileSync(cliPath, patched, 'utf8');
	return true;
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
	const changed = patchCompileExtensionTags();
	console.log(
		changed
			? `Patched @php-wasm/compile-extension's cli.js (image tags, PHP lastRelease table).`
			: 'Already patched.'
	);
}
