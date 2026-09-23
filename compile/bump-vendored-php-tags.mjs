#!/usr/bin/env node
/**
 * Bumps every "vendored from PHP-X.Y.Z" tag mention -- in both
 * compile/extensions/<name>/PROVENANCE.md and its published
 * packages/phpext-<name>/README.md -- to the PHP tag this repo's core
 * build is currently pinned to (same target-tag resolution as
 * check-vendored-extensions.mjs).
 *
 * Deliberately runs checkVendoredExtensions() first and only bumps an
 * extension whose status comes back "tag-bump-only" or "up-to-date" --
 * never "drift". Bumping the tag mention on a `drift` extension would be a
 * real false claim (its own vendored .c/.h files would still be the OLDER
 * tag's content, having not actually been re-vendored) -- found in
 * practice the same session this script was written: mysqlnd, pdo_pgsql,
 * and soap all had genuine upstream file changes between PHP-8.5.10 and
 * PHP-8.5.11, so their PROVENANCE.md/README.md stay untouched here until
 * someone re-vendors them by hand (see each one's own PROVENANCE.md
 * "Regenerating" line) and bumps the tag as part of that.
 *
 * A plain find/replace on the `PHP-X.Y.Z` substring, not a template
 * regeneration -- every file's hand-written prose around it stays exactly
 * as authored. package.json's own `minVersion` is already regenerated
 * fresh on every `compile-extension` run (from the actually-built
 * php.wasm's own version string), so it's out of scope here.
 *
 * Usage: node compile/bump-vendored-php-tags.mjs [--tag PHP-8.5.11] [--write]
 *   --write   Persist changes. Without it, just prints a dry-run report.
 */
import { readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { checkVendoredExtensions } from './check-vendored-extensions.mjs';

const sourceDir = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(sourceDir, '..');

const TAG_PATTERN = /PHP-\d+\.\d+\.\d+/g;

function bumpFile(filePath, targetTag) {
	let text;
	try {
		text = readFileSync(filePath, 'utf8');
	} catch {
		return null;
	}
	const foundTags = [...new Set(text.match(TAG_PATTERN) ?? [])];
	if (foundTags.length === 0) return null;
	if (foundTags.length === 1 && foundTags[0] === targetTag) return null;
	return { text: text.replace(TAG_PATTERN, targetTag), foundTags };
}

/**
 * Bumps `PHP-X.Y.Z` mentions in every safe-to-bump extension's
 * PROVENANCE.md + README.md to the target tag. Returns `{ targetTag,
 * bumped, skippedDrift }`.
 */
export async function bumpVendoredPhpTags({ tag, write = false } = {}) {
	const { hasDrift, results, targetTag } = await checkVendoredExtensions({ tag });

	console.log(`\nApplying bumps for target tag: ${targetTag}\n`);

	const bumped = [];
	const skippedDrift = results.filter((r) => r.status === 'drift').map((r) => r.name);

	for (const result of results) {
		if (result.status === 'drift') continue;
		if (result.status !== 'tag-bump-only' && result.status !== 'up-to-date') continue;

		const extDir = path.join(sourceDir, 'extensions', result.name);
		const packageReadme = path.join(repoRoot, 'packages', `phpext-${result.name}`, 'README.md');
		const changedFiles = [];

		for (const filePath of [path.join(extDir, 'PROVENANCE.md'), packageReadme]) {
			const bump = bumpFile(filePath, targetTag);
			if (!bump) continue;
			changedFiles.push(path.relative(repoRoot, filePath));
			if (write) writeFileSync(filePath, bump.text);
		}

		if (changedFiles.length) bumped.push({ name: result.name, changedFiles });
	}

	if (bumped.length === 0) {
		console.log('Nothing to bump (everything already at the target tag, or has no PHP-X.Y.Z mention).');
	} else {
		console.log(`${write ? 'Bumped' : 'Would bump'} ${bumped.length} extension(s):`);
		for (const { name, changedFiles } of bumped) {
			console.log(`  - ${name}: ${changedFiles.join(', ')}`);
		}
		if (!write) console.log('\nRe-run with --write to persist.');
	}
	if (skippedDrift.length) {
		console.log(
			`\nSkipped (real upstream drift, re-vendor first): ${skippedDrift.join(', ')} -- ` +
				'see each one\'s own PROVENANCE.md "Regenerating" line.'
		);
	}

	return { targetTag, bumped, skippedDrift, hasDrift };
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
	const args = process.argv.slice(2);
	const tagIndex = args.indexOf('--tag');
	const tag = tagIndex !== -1 ? args[tagIndex + 1] : undefined;
	const write = args.includes('--write');

	bumpVendoredPhpTags({ tag, write }).catch((err) => {
		console.error('FATAL:', err);
		process.exit(1);
	});
}
