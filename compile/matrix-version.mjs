#!/usr/bin/env node
// Resolves the version compile/Makefile (and cli.mjs) should build for a
// given matrix.json library entry — its "latest" value (the last entry of
// that lib's `versions` array, per the header comment in matrix.json).
// Used two ways: as a CLI (`node matrix-version.mjs <lib>`, called from
// Makefile via `$(shell ...)`, so `CURL_VERSION`/`OPENSSL_VERSION`/etc. read
// from matrix.json instead of being duplicated as separately hand-synced
// Makefile variables) and as a module (`import { getMatrixVersion } from
// './matrix-version.mjs'`, used by cli.mjs for the same reason on the
// config.yaml side — see CLAUDE.md decision 27).
import { readFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const sourceDir = path.dirname(fileURLToPath(import.meta.url));
const matrixPath = path.resolve(sourceDir, '..', 'matrix.json');

export function getMatrixVersion(libraryKey) {
	const matrix = JSON.parse(readFileSync(matrixPath, 'utf8'));
	const lib = matrix.libraries?.[libraryKey];
	if (!lib) {
		throw new Error(`matrix.json: no "libraries.${libraryKey}" entry.`);
	}
	const versions = lib.versions ?? [];
	if (versions.length === 0) {
		throw new Error(`matrix.json: "libraries.${libraryKey}.versions" is empty.`);
	}
	return versions[versions.length - 1];
}

/**
 * Same "last entry of versions[] = latest" convention as getMatrixVersion(),
 * but for a matrix.json `extensions.<key>` entry (a PECL extension's own
 * source, e.g. yaml/cmark — CLAUDE.md decision 34) rather than a
 * `libraries.<key>` C dependency.
 */
export function getMatrixExtensionVersion(extensionKey) {
	const matrix = JSON.parse(readFileSync(matrixPath, 'utf8'));
	const ext = matrix.extensions?.[extensionKey];
	if (!ext) {
		throw new Error(`matrix.json: no "extensions.${extensionKey}" entry.`);
	}
	const versions = ext.versions ?? [];
	if (versions.length === 0) {
		throw new Error(`matrix.json: "extensions.${extensionKey}.versions" is empty.`);
	}
	return versions[versions.length - 1];
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
	const libraryKey = process.argv[2];
	if (!libraryKey) {
		console.error('Usage: node matrix-version.mjs <libraryKey>');
		process.exit(1);
	}
	try {
		console.log(getMatrixVersion(libraryKey));
	} catch (error) {
		console.error(error.message);
		process.exit(1);
	}
}
