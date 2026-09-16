#!/usr/bin/env node
// Assembles the exact directory structure @php-wasm/compile-extension
// expects to find under PHP_WASM_COMPILE_EXTENSION_CACHE_DIR, from files
// this repo already owns, so it never fetches Docker assets from
// WordPress/wordpress-playground at all (CLAUDE.md decision 45's
// follow-up — that fetch previously meant every mode:shared extension was
// built against a different, unpatched base image than our own
// kirigami-php-wasm:base). Cheap enough to regenerate on every
// `compile-extension` run, so it can never go stale relative to
// compile/base-image/*.
//
// Used two ways: as a CLI (`node setup-compile-extension-cache.mjs`) and as
// a module (`import { prepareCompileExtensionCache } from
// './setup-compile-extension-cache.mjs'`, used by cli.mjs before shelling
// out to the compile-extension binary).
import { copyFileSync, mkdirSync, readFileSync, rmSync } from 'node:fs';
import path from 'node:path';
import { createHash } from 'node:crypto';
import { fileURLToPath } from 'node:url';

const sourceDir = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(sourceDir, '..');

// The only two files genuinely specific to @php-wasm/compile-extension
// itself (its own side-module build recipe) — not duplicated anywhere else
// in this repo, vendored once from the tag matching our installed version
// (see compile/vendor/compile-extension-docker-assets/PROVENANCE.md).
const VENDORED_FILES = [
	['compile-extension/docker/Dockerfile.ext', 'compile-extension/docker/Dockerfile.ext'],
	['compile-extension/scripts/build-in-docker.sh', 'compile-extension/scripts/build-in-docker.sh'],
];

// Everything else @php-wasm/compile-extension's own asset check
// (`docker-assets.ts`'s file list) expects is something this repo already
// owns and actively maintains — copied fresh every time, never vendored,
// so it can't drift out of sync with compile/base-image/*.
const OWNED_FILES = [
	['compile/base-image/Dockerfile', 'compile/base-image/Dockerfile'],
	['compile/base-image/emcc-for-php-wasm.sh', 'compile/base-image/emcc-for-php-wasm.sh'],
	['compile/base-image/replace.sh', 'compile/base-image/replace.sh'],
	['compile/base-image/replace-across-lines.sh', 'compile/base-image/replace-across-lines.sh'],
	// Only PHP 8.5 is supported by this pipeline (CLAUDE.md decision 16) —
	// the only php*.patch compile-extension's own Dockerfile.ext ever needs.
	['compile/php/php8.5.patch', 'compile/php/php8.5.patch'],
];

/**
 * Builds (or refreshes) the cache directory, and returns the cache root to
 * set PHP_WASM_COMPILE_EXTENSION_CACHE_DIR to.
 */
export function prepareCompileExtensionCache() {
	const compileExtensionPkgPath = path.join(
		repoRoot,
		'compile',
		'node_modules',
		'@php-wasm',
		'compile-extension',
		'package.json'
	);
	const version = JSON.parse(readFileSync(compileExtensionPkgPath, 'utf8')).version;
	// Matches @php-wasm/compile-extension's own cache-key computation
	// exactly (docker-assets.ts: sha256(`v${version}`).slice(0, 16)) — this
	// repo's cache root must resolve to the identical path it would check
	// on its own, just pre-populated instead of fetched.
	const hash = createHash('sha256').update(`v${version}`).digest('hex').slice(0, 16);

	const cacheRoot = path.join(repoRoot, 'compile', '.compile-extension-cache');
	const phpWasmRoot = path.join(cacheRoot, hash, 'php-wasm');

	rmSync(phpWasmRoot, { recursive: true, force: true });
	for (const [, dest] of [...OWNED_FILES, ...VENDORED_FILES]) {
		mkdirSync(path.join(phpWasmRoot, path.dirname(dest)), { recursive: true });
	}
	for (const [src, dest] of OWNED_FILES) {
		copyFileSync(path.join(repoRoot, src), path.join(phpWasmRoot, dest));
	}
	for (const [src, dest] of VENDORED_FILES) {
		copyFileSync(
			path.join(repoRoot, 'compile', 'vendor', 'compile-extension-docker-assets', src),
			path.join(phpWasmRoot, dest)
		);
	}

	return cacheRoot;
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
	console.log(prepareCompileExtensionCache());
}
