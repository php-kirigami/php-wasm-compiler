#!/usr/bin/env node
/**
 * Refreshes matrix.json's "versions" arrays for libraries whose sourceType
 * is "github-release" or "github-tag", by querying the GitHub API. Mirrors
 * update-php-versions.mjs's approach (see CLAUDE.md decision 11) but only
 * covers GitHub-hosted libraries for now — tarball/sourceforge/googlesource
 * sources still have to be checked manually via their "checkUrl".
 *
 * Usage: node compile/update-lib-versions.mjs [--write]
 *   --write   Persist changes to matrix.json. Without it, just prints a
 *             report of what would change (dry run).
 */
import { readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const sourceDir = path.dirname(fileURLToPath(import.meta.url));
const matrixPath = path.resolve(sourceDir, '..', 'matrix.json');

async function fetchJSON(url) {
	const response = await fetch(url, {
		headers: { 'User-Agent': 'php-wasm-compiler' },
	});
	if (!response.ok) {
		throw new Error(`HTTP ${response.status} for ${url}`);
	}
	return response.json();
}

/**
 * Strip a leading "v" and any non-numeric prefix/suffix noise from a tag.
 * Some projects (e.g. curl's "curl-8_22_0") separate version components with
 * underscores instead of dots — normalize those first. ImageMagick tags
 * carry a trailing "-NN" patch counter ("7.1.2-31") that's significant (not
 * a prerelease suffix to discard) — keep it when present.
 */
function normalizeVersion(tag) {
	const match = tag.replace(/_/g, '.').match(/(\d+(?:\.\d+){1,3}(?:-\d+)?)/);
	return match ? match[1] : tag;
}

async function latestGithubRelease(repo) {
	try {
		const release = await fetchJSON(
			`https://api.github.com/repos/${repo}/releases/latest`
		);
		return normalizeVersion(release.tag_name);
	} catch {
		return null;
	}
}

/**
 * Fallback for repos that don't use GitHub's Releases feature: fetch tags
 * and keep only ones that look like a plain version number, then take the
 * highest by numeric comparison.
 */
async function latestGithubTag(repo) {
	const tags = await fetchJSON(
		`https://api.github.com/repos/${repo}/tags?per_page=100`
	);
	// ImageMagick tags this way ("7.1.2-31" — a trailing "-NN" patch counter,
	// not a prerelease suffix), so allow it alongside plain dotted versions.
	const versions = tags
		.map((t) => t.name)
		.filter((name) => /^v?\d+(\.\d+){1,3}(-\d+)?$/.test(name))
		.map(normalizeVersion);
	if (versions.length === 0) {
		return null;
	}
	// Split "7.1.2-31" into [7, 1, 2, 31] so the trailing patch counter sorts
	// numerically alongside the dotted components, not as a NaN.
	const toParts = (v) => v.replace('-', '.').split('.').map(Number);
	versions.sort((a, b) => {
		const pa = toParts(a);
		const pb = toParts(b);
		for (let i = 0; i < Math.max(pa.length, pb.length); i++) {
			const diff = (pa[i] || 0) - (pb[i] || 0);
			if (diff !== 0) return diff;
		}
		return 0;
	});
	return versions[versions.length - 1];
}

export async function updateLibVersions({ write = false } = {}) {
	const matrix = JSON.parse(readFileSync(matrixPath, 'utf8'));
	let changed = 0;

	for (const [name, lib] of Object.entries(matrix.libraries)) {
		if (lib.sourceType !== 'github-release' && lib.sourceType !== 'github-tag') {
			continue;
		}
		if (!lib.repo || lib.repo.includes('/') === false) {
			continue;
		}
		const repo = lib.repo.replace(/^https?:\/\/github\.com\//, '').replace(/\.git$/, '');
		const latest =
			lib.sourceType === 'github-release'
				? (await latestGithubRelease(repo)) ?? (await latestGithubTag(repo))
				: await latestGithubTag(repo);

		if (!latest) {
			console.warn(`⚠️  ${name}: could not resolve a version from ${repo}`);
			continue;
		}

		const current = lib.versions[lib.versions.length - 1];
		if (latest !== current) {
			console.log(`${name}: ${current ?? '(none)'} → ${latest}`);
			if (!lib.versions.includes(latest)) {
				lib.versions.push(latest);
			}
			changed++;
		}
	}

	if (changed === 0) {
		console.log('✨ All GitHub-hosted libraries are already up to date!');
	} else if (write) {
		matrix.lastChecked = new Date().toISOString();
		writeFileSync(matrixPath, JSON.stringify(matrix, null, '\t') + '\n');
		console.log(`\nWrote ${changed} update(s) to ${matrixPath}`);
	} else {
		console.log(`\n${changed} update(s) found (dry run — pass --write to persist).`);
	}

	return changed;
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
	updateLibVersions({ write: process.argv.includes('--write') }).catch((error) => {
		console.error(error);
		process.exit(1);
	});
}
