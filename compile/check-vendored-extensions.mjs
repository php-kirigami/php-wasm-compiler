#!/usr/bin/env node
/**
 * Reports drift between the php-src-bundled extensions vendored under
 * compile/extensions/<name>/ (pgsql, ldap, tidy, enchant, soap, gettext,
 * dba, posix, gmp, mysqli, mysqlnd, pdo_mysql, pdo_pgsql, ftp, ...) and the
 * PHP tag this repo's core build is currently pinned to
 * (supported-php-versions.mjs's own `lastRelease`, for the PHP version(s)
 * config.yaml builds).
 *
 * Unlike matrix.json's own `libraries`/`extensions` entries (external C
 * libraries and independently-versioned PECL/Kirigami extensions, kept
 * current by update-lib-versions.mjs), these extensions have no version of
 * their own — they're tied 1:1 to whatever PHP tag they were last
 * re-vendored from, and nothing currently re-checks that automatically.
 * Real drift found once already, the reason this script exists: several
 * extensions vendored from PHP-8.5.10 while the core build had already
 * moved to 8.5.11.
 *
 * For each compile/extensions/<name>/PROVENANCE.md (the source of truth
 * for both "which PHP tag was this vendored from" and "which files are
 * vendored" -- both already documented there in prose, parsed back out
 * here rather than duplicated into a second manifest), every listed file
 * is fetched from BOTH the vendored tag and the target tag and compared to
 * each other (not to the local copy -- a local copy can carry a
 * deliberate, documented patch, e.g. tidy's config.m4, which would
 * otherwise show up as noise on every run). A file only gets flagged if
 * upstream itself changed between the two tags.
 *
 * This is a read-only report -- it never re-vendors anything itself
 * (a patched file needs its patch re-applied by a human, not silently
 * overwritten). Re-vendoring instructions are already in each
 * PROVENANCE.md's own "Regenerating" line.
 *
 * Usage: node compile/check-vendored-extensions.mjs [--tag PHP-8.5.11] [--only pgsql,ldap]
 */
import { readFileSync, readdirSync, statSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { execFileSync } from 'node:child_process';

const sourceDir = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(sourceDir, '..');
const extensionsDir = path.join(sourceDir, 'extensions');

let githubToken;
function getGithubToken() {
	if (githubToken !== undefined) return githubToken;
	if (process.env.GITHUB_TOKEN) {
		githubToken = process.env.GITHUB_TOKEN;
		return githubToken;
	}
	try {
		githubToken = execFileSync('gh', ['auth', 'token'], { encoding: 'utf8' }).trim() || null;
	} catch {
		githubToken = null;
	}
	return githubToken;
}

/** Fetches raw file content from a php-src tag, or null on 404 (file moved/generated/removed). */
async function fetchPhpSrcFile(tag, extDir, filename) {
	const url = `https://raw.githubusercontent.com/php/php-src/${tag}/ext/${extDir}/${filename}`;
	const response = await fetch(url, { headers: { 'User-Agent': 'php-wasm-compiler' } });
	if (response.status === 404) return null;
	if (!response.ok) throw new Error(`HTTP ${response.status} fetching ${url}`);
	return response.text();
}

/** Resolves the target PHP tag (e.g. "PHP-8.5.11") from supported-php-versions.mjs + config.yaml. */
export async function resolveTargetTag(tagOverride) {
	if (tagOverride) return tagOverride;
	const { phpVersions } = await import(pathToFileURLSafe(path.join(repoRoot, 'supported-php-versions.mjs')));
	const yaml = await import('yaml');
	const config = yaml.parse(readFileSync(path.join(repoRoot, 'config.yaml'), 'utf8'));
	const primaryVersion = config.php.versions[0];
	const entry = phpVersions.find((v) => v.version === primaryVersion);
	if (!entry) {
		throw new Error(`supported-php-versions.mjs has no entry for PHP ${primaryVersion} (config.yaml's php.versions[0]).`);
	}
	return `PHP-${entry.lastRelease}`;
}

function pathToFileURLSafe(p) {
	return new URL(`file:///${p.replace(/\\/g, '/')}`);
}

/** Parses "...from the `PHP-X.Y.Z` tag..." out of a PROVENANCE.md's own prose. */
function parseVendoredTag(provenanceText) {
	const match = provenanceText.match(/`(PHP-\d+\.\d+\.\d+)`\s+tag/);
	return match ? match[1] : null;
}

/** Parses the trailing "Regenerating: re-download `a`, `b`, `c` from..." line's file list. */
function parseVendoredFiles(provenanceText) {
	const match = provenanceText.match(/Regenerating:\s*re-download\s+([\s\S]+?)\s+from the matching PHP tag/);
	if (!match) return null;
	const filenames = [...match[1].matchAll(/`([^`]+)`/g)].map((m) => m[1]);
	return filenames.length ? filenames : null;
}

/** Files that are never part of the real php-src extension, wherever they appear in the tree. */
const LOCAL_ONLY_FILES = new Set(['PROVENANCE.md', 'smoke-test.php']);

/**
 * Fallback for PROVENANCE.md files that vendor "the complete file set"
 * (e.g. dba's, whose own note explains why an itemized list wouldn't work
 * for a multi-backend extension) rather than an itemized "Regenerating"
 * list: recursively lists every real file actually present in the local
 * extension directory instead, as POSIX-style relative paths (matching
 * what a GitHub raw URL needs for a nested file like `libcdb/cdb.c`).
 */
function listLocalFiles(extDir) {
	const results = [];
	function walk(dir, relPrefix) {
		for (const entry of readdirSync(dir, { withFileTypes: true })) {
			if (LOCAL_ONLY_FILES.has(entry.name)) continue;
			const relPath = relPrefix ? `${relPrefix}/${entry.name}` : entry.name;
			if (entry.isDirectory()) {
				if (entry.name === 'vendor' || entry.name === 'tests') continue;
				walk(path.join(dir, entry.name), relPath);
			} else {
				results.push(relPath);
			}
		}
	}
	walk(extDir, '');
	return results;
}

async function checkExtension(name, targetTag) {
	const extDir = path.join(extensionsDir, name);
	const provenancePath = path.join(extDir, 'PROVENANCE.md');
	let provenanceText;
	try {
		provenanceText = readFileSync(provenancePath, 'utf8');
	} catch {
		return { name, status: 'no-provenance' };
	}

	const vendoredTag = parseVendoredTag(provenanceText);
	if (!vendoredTag) {
		return { name, status: 'unparseable-provenance' };
	}
	const files = parseVendoredFiles(provenanceText) ?? listLocalFiles(extDir);
	if (vendoredTag === targetTag) {
		return { name, status: 'up-to-date', vendoredTag, targetTag };
	}

	const changedFiles = [];
	const notFoundFiles = [];
	for (const filename of files) {
		const [oldContent, newContent] = await Promise.all([
			fetchPhpSrcFile(vendoredTag, name, filename),
			fetchPhpSrcFile(targetTag, name, filename),
		]);
		if (oldContent === null && newContent === null) {
			notFoundFiles.push(filename);
			continue;
		}
		if (oldContent !== newContent) {
			changedFiles.push(filename);
		}
	}

	return {
		name,
		status: changedFiles.length ? 'drift' : 'tag-bump-only',
		vendoredTag,
		targetTag,
		changedFiles,
		notFoundFiles,
	};
}

/**
 * Checks every php-src-bundled extension under compile/extensions/ against
 * the PHP tag this repo's core is currently pinned to (or `tag`, if given)
 * and prints a report. Returns `{ hasDrift }` so callers (e.g. cli.mjs's
 * `update-versions` command) can decide whether to surface that in their
 * own summary — this never throws just because drift was found, only on a
 * real fetch/parse failure.
 */
export async function checkVendoredExtensions({ tag, only } = {}) {
	const targetTag = await resolveTargetTag(tag);
	console.log(`Target PHP tag: ${targetTag}\n`);

	const names = readdirSync(extensionsDir).filter((name) => statSync(path.join(extensionsDir, name)).isDirectory());
	const toCheck = only ? names.filter((n) => only.includes(n)) : names;

	const results = [];
	for (const name of toCheck.sort()) {
		process.stdout.write(`Checking ${name}... `);
		try {
			const result = await checkExtension(name, targetTag);
			results.push(result);
			console.log(result.status);
		} catch (err) {
			results.push({ name, status: 'error', error: err.message });
			console.log(`error: ${err.message}`);
		}
	}

	console.log('\n--- Summary ---\n');
	const driftResults = results.filter((r) => r.status === 'drift');
	const bumpOnlyResults = results.filter((r) => r.status === 'tag-bump-only');
	const upToDate = results.filter((r) => r.status === 'up-to-date');
	const skipped = results.filter((r) => r.status === 'no-provenance' || r.status === 'unparseable-provenance');
	const errored = results.filter((r) => r.status === 'error');

	if (driftResults.length) {
		console.log(`${driftResults.length} extension(s) with real upstream changes to re-vendor:`);
		for (const r of driftResults) {
			console.log(`  - ${r.name}: vendored from ${r.vendoredTag}, target is ${r.targetTag}`);
			for (const f of r.changedFiles) console.log(`      changed: ${f}`);
			for (const f of r.notFoundFiles) console.log(`      not found in either tag (skipped): ${f}`);
		}
		console.log(
			"\n  See each extension's own PROVENANCE.md \"Regenerating\" line for the re-download procedure. " +
				'If a PROVENANCE.md documents a deliberate local patch, re-apply it after re-vendoring.'
		);
	}
	if (bumpOnlyResults.length) {
		console.log(
			`\n${bumpOnlyResults.length} extension(s) behind the target tag, but with no real upstream file changes ` +
				'(safe to just bump the tag mentioned in PROVENANCE.md, no content to re-diff):'
		);
		for (const r of bumpOnlyResults) console.log(`  - ${r.name} (vendored from ${r.vendoredTag})`);
	}
	console.log(`\n${upToDate.length} extension(s) already vendored from the target tag.`);
	if (skipped.length) {
		console.log(`${skipped.length} extension(s) skipped (no PROVENANCE.md, or unparseable): ${skipped.map((r) => r.name).join(', ')}`);
	}
	if (errored.length) {
		console.log(`${errored.length} extension(s) failed to check:`);
		for (const r of errored) console.log(`  - ${r.name}: ${r.error}`);
	}

	return { hasDrift: driftResults.length > 0, results, targetTag };
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
	const args = process.argv.slice(2);
	const tagIndex = args.indexOf('--tag');
	const tag = tagIndex !== -1 ? args[tagIndex + 1] : undefined;
	const onlyIndex = args.indexOf('--only');
	const only = onlyIndex !== -1 ? args[onlyIndex + 1].split(',').map((s) => s.trim()) : undefined;

	checkVendoredExtensions({ tag, only })
		.then(({ hasDrift }) => {
			process.exitCode = hasDrift ? 1 : 0;
		})
		.catch((err) => {
			console.error('FATAL:', err);
			process.exit(1);
		});
}
