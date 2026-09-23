#!/usr/bin/env node
/**
 * Refreshes matrix.json's "versions" arrays by querying each entry's
 * upstream host directly — GitHub (Releases API, tags API fallback),
 * GitLab (tags API), Gitiles/googlesource (+refs?format=JSON), SourceForge
 * (RSS feed), and a generic regex scan of a plain download/listing page
 * (GNU ftp directory indexes, zlib.net's homepage, sqlite.org's download
 * page) for everything else with a `checkUrl`. Mirrors
 * update-php-versions.mjs's approach (see CLAUDE.md decision 11).
 *
 * Also covers `matrix.extensions` entries (PECL/Kirigami-own extensions
 * sourced straight from a GitHub repo's tags, e.g. yaml/mdhtml/jsonk/
 * navicat/norm/igbinary/apcu — CLAUDE.md decision 34) — these don't set an
 * explicit `sourceType` the way `libraries` entries do, so any entry with a
 * `repo` is assumed GitHub-hosted (release first, tag fallback).
 *
 * An entry can opt out entirely with `"autoUpdate": false` (e.g. libzip,
 * whose two parallel versions are hardcoded in compile/Makefile and not
 * read from this array at all; libopenssl, whose "pick the newest 3.x, not
 * the newest major" policy isn't something a generic resolver should
 * guess at — see their notes in matrix.json).
 *
 * Every resolved candidate is verified with a live HEAD/GET against its
 * rendered `sourceTemplate` URL before being written — a wrong guess (a
 * stale year segment, a template that doesn't match the real filename)
 * fails loud as a warning instead of silently recording a version whose
 * download link 404s at build time.
 *
 * Unauthenticated GitHub API calls are capped at 60/hour — easy to exhaust
 * across ~15 GitHub-hosted entries (up to 2 calls each) plus repeated dry
 * runs. If `GITHUB_TOKEN` is set, or `gh` (GitHub CLI) is installed and
 * logged in, its token is sent as a Bearer header, raising that to
 * 5000/hour. Falls back to unauthenticated silently if neither is
 * available.
 *
 * Usage: node compile/update-lib-versions.mjs [--write]
 *   --write   Persist changes to matrix.json. Without it, just prints a
 *             report of what would change (dry run).
 */
import { readFileSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { execFileSync } from 'node:child_process';

const sourceDir = path.dirname(fileURLToPath(import.meta.url));
const matrixPath = path.resolve(sourceDir, '..', 'matrix.json');

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

async function fetchText(url) {
	const headers = { 'User-Agent': 'php-wasm-compiler' };
	if (new URL(url).hostname === 'api.github.com') {
		const token = getGithubToken();
		if (token) headers.Authorization = `Bearer ${token}`;
	}
	const response = await fetch(url, { headers });
	if (!response.ok) {
		throw new Error(`HTTP ${response.status} for ${url}`);
	}
	return response.text();
}

async function fetchJSON(url) {
	return JSON.parse(await fetchText(url));
}

/** Fetches googlesource's Gitiles JSON, stripping its XSSI `)]}'` prefix line. */
async function fetchGitilesJSON(url) {
	const text = await fetchText(url);
	return JSON.parse(text.replace(/^\)\]\}'\n?/, ''));
}

/** true if `url` resolves (HEAD, falling back to a ranged GET for hosts that reject HEAD). */
async function urlResolves(url) {
	try {
		const head = await fetch(url, { method: 'HEAD', headers: { 'User-Agent': 'php-wasm-compiler' } });
		if (head.ok) return true;
		if (head.status !== 405 && head.status !== 501) return false;
	} catch {
		/* fall through to GET */
	}
	try {
		const get = await fetch(url, {
			headers: { 'User-Agent': 'php-wasm-compiler', Range: 'bytes=0-0' },
		});
		return get.ok || get.status === 206;
	} catch {
		return false;
	}
}

/**
 * Strip a leading "v" and any non-numeric prefix/suffix noise from a tag.
 * Some projects (e.g. curl's "curl-8_22_0") separate version components with
 * underscores instead of dots — normalize those first. ImageMagick tags
 * carry a trailing "-NN" patch counter ("7.1.2-31") that's significant (not
 * a prerelease suffix to discard) — keep it when present. cmark-gfm tags
 * carry a trailing ".gfm.N" patch counter ("0.29.0.gfm.13") that's equally
 * significant — without this, the plain-numeric regex below stops at the
 * first ".gfm" and silently truncates every cmark-gfm version down to its
 * upstream cmark base ("0.29.0"), which isn't a real cmark-gfm release on
 * its own and would regress matrix.json's pinned version if written.
 */
function normalizeVersion(tag) {
	const match = tag
		.replace(/_/g, '.')
		.match(/(\d+(?:\.\d+){1,3}(?:-\d+)?(?:\.gfm\.\d+)?)/);
	return match ? match[1] : tag;
}

/** Numeric, component-wise comparison ("7.1.2-31" → [7,1,2,31]); highest last. */
function highestVersion(versions) {
	if (versions.length === 0) return null;
	const toParts = (v) => v.replace('-', '.').split('.').map(Number);
	return [...versions].sort((a, b) => {
		const pa = toParts(a);
		const pb = toParts(b);
		for (let i = 0; i < Math.max(pa.length, pb.length); i++) {
			const diff = (pa[i] || 0) - (pb[i] || 0);
			if (diff !== 0) return diff;
		}
		return 0;
	})[versions.length - 1];
}

async function latestGithubRelease(repo) {
	try {
		const release = await fetchJSON(`https://api.github.com/repos/${repo}/releases/latest`);
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
	let tags;
	try {
		tags = await fetchJSON(`https://api.github.com/repos/${repo}/tags?per_page=100`);
	} catch {
		return null;
	}
	// ImageMagick tags this way ("7.1.2-31" — a trailing "-NN" patch counter,
	// not a prerelease suffix), so allow it alongside plain dotted versions.
	// freetype tags this way ("VER-2-14-3" — dashes instead of dots); convert
	// to the dotted form matrix.json actually stores (the Dockerfile does the
	// reverse substitution itself) before applying the same numeric filter.
	const versions = tags
		.map((t) => t.name)
		.map((name) => {
			const verDashed = name.match(/^VER-(\d+(?:-\d+)+)$/i);
			return verDashed ? verDashed[1].replace(/-/g, '.') : name;
		})
		.filter((name) => /^v?\d+(\.\d+){1,3}(-\d+)?$/.test(name))
		.map(normalizeVersion);
	return highestVersion(versions);
}

async function resolveGithub(entry, sourceType) {
	if (!entry.repo || entry.repo.includes('/') === false) return null;
	const repo = entry.repo.replace(/^https?:\/\/github\.com\//, '').replace(/\.git$/, '');
	return sourceType === 'github-release'
		? (await latestGithubRelease(repo)) ?? (await latestGithubTag(repo))
		: await latestGithubTag(repo);
}

/**
 * GitLab-hosted `git-tag` entries (e.g. libxml2 on gitlab.gnome.org) — the
 * REST tags API. Any other git host for this sourceType is left
 * unsupported (returns null), same as before this function existed.
 */
async function resolveGitTag(entry) {
	const match = (entry.repo ?? '').match(/^https?:\/\/([^/]+)\/(.+?)(?:\.git)?$/);
	if (!match || !match[1].includes('gitlab')) return null;
	const [, host, projectPath] = match;
	const tags = await fetchJSON(
		`https://${host}/api/v4/projects/${encodeURIComponent(projectPath)}/repository/tags?per_page=100`
	);
	const versions = tags.map((t) => t.name).filter((name) => /^v?\d+(\.\d+){1,3}$/.test(name));
	const best = highestVersion(versions.map(normalizeVersion));
	if (best === null) return null;
	// Re-apply whichever tags actually carried it, so `v2.15.4` doesn't get
	// silently rewritten to a bare `2.15.4` matrix.json never used.
	return versions.find((v) => normalizeVersion(v) === best) ?? best;
}

/**
 * Gitiles-hosted repos (*.googlesource.com) — `+refs?format=JSON` lists
 * every branch and tag in one call, no scraping needed. Filters out
 * pre-release/errata/rc suffixes before comparing.
 */
async function resolveGooglesource(entry) {
	if (!entry.repo) return null;
	const refs = await fetchGitilesJSON(`https://${entry.repo}/+refs?format=JSON`);
	const tagNames = Object.keys(refs)
		.filter((ref) => ref.startsWith('refs/tags/'))
		.map((ref) => ref.replace('refs/tags/', ''));
	const versions = tagNames
		.map((name) => name.replace(/^v/, ''))
		.filter((name) => /^\d+(\.\d+){1,3}$/.test(name));
	return highestVersion(versions);
}

/**
 * SourceForge project — no clean JSON API, but its RSS feed lists real
 * filenames straight out of the files browser, which is enough to regex
 * against the entry's own `sourceTemplate` filename shape. `rssPath`
 * (optional) scopes it to one subfolder (e.g. libpng's `/libpng16`, so a
 * 1.7.x beta or a 1.5.x file elsewhere in the project doesn't win); without
 * it, the whole project's feed is searched.
 */
async function resolveSourceforge(entry, projectSlug) {
	const path = entry.rssPath ? `?path=${encodeURIComponent(entry.rssPath)}` : '';
	const xml = await fetchText(`https://sourceforge.net/projects/${projectSlug}/rss${path}`);
	const pattern = templateToRegExp(entry.sourceTemplate.split('/').pop());
	const versions = [...xml.matchAll(pattern)].map((m) => m[1]);
	return highestVersion(versions);
}

/**
 * Turns a template filename like "libiconv-{version}.tar.gz" into a RegExp
 * matching real filenames and capturing the version — used for plain
 * listing/download pages and feeds with no real API (GNU ftp directory
 * indexes, zlib.net's homepage, sqlite.org's download page, SourceForge's
 * RSS). Always matches against a filename, not a full URL — none of these
 * sources render the complete sourceTemplate URL verbatim.
 */
function templateToRegExp(filenamePart) {
	const escaped = filenamePart.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
	const pattern = escaped
		.replace('\\{version\\}', '([\\d.]+)')
		.replace('\\{versionCompact\\}', '(\\d+)');
	return new RegExp(pattern, 'g');
}

/**
 * Generic "scrape a plain page for filenames matching the template" —
 * covers GNU ftp's directory index (libiconv), zlib.net's homepage (a
 * direct `zlib-X.Y.Z.tar.gz` link), and sqlite.org's download page (whose
 * `sqlite-autoconf-NNNNNNN.tar.gz` links use `{versionCompact}`, decoded
 * back to a dotted version below). An entry's own `checkPattern` overrides
 * the template-derived pattern.
 */
async function resolveTarball(entry) {
	if (!entry.checkUrl) return null;
	const html = await fetchText(entry.checkUrl);
	// `checkPattern` (optional): a regex with one capture group for pages
	// that list versions without the template's filename at all — e.g.
	// postgresql.org/ftp/source/ only links per-version directories
	// ("v18.6/"), the tarball itself lives one level down.
	if (entry.checkPattern) {
		const matches = [...html.matchAll(new RegExp(entry.checkPattern, 'g'))].map((m) => m[1]);
		return highestVersion(matches);
	}
	const usesCompact = entry.sourceTemplate.includes('{versionCompact}');
	const basename = entry.sourceTemplate.split('/').pop();
	// A plain listing/download page (GNU ftp, zlib.net, sqlite.org) almost
	// never shows the full sourceTemplate URL verbatim — just the filename,
	// sometimes with its own path segment sqlite.org's own download links
	// prefix with a release-year directory ("2026/sqlite-autoconf-...")
	// that isn't part of {version}/{versionCompact} at all. Tolerate an
	// optional leading "NNNN/" so this stays generic instead of
	// sqlite-specific.
	const pattern = new RegExp('(?:\\d{4}/)?' + templateToRegExp(basename).source, 'g');
	const matches = [...html.matchAll(pattern)].map((m) => m[1]);
	if (matches.length === 0) return null;

	if (!usesCompact) {
		return highestVersion(matches);
	}
	// sqlite's compact scheme: MAJOR*1000000 + MINOR*10000 + PATCH*100 + BUILD.
	const compactToDotted = (n) => {
		const major = Math.floor(n / 1000000);
		const minor = Math.floor(n / 10000) % 100;
		const patch = Math.floor(n / 100) % 100;
		return `${major}.${minor}.${patch}`;
	};
	const dotted = matches.map((n) => compactToDotted(Number(n)));
	return highestVersion(dotted);
}

const RESOLVERS = {
	'github-release': resolveGithub,
	'github-tag': resolveGithub,
	'git-tag': resolveGitTag,
	'googlesource-archive': resolveGooglesource,
	'googlesource-archive-commit': resolveGooglesource,
	tarball: resolveTarball,
	sourceforge: async (entry) => {
		// The only sourceforge entry today (libpng) doesn't record its own
		// project slug — derive it from sourceTemplate's host-relative path
		// (".../libpng/libpng-{version}.tar.gz" → "libpng").
		const slugMatch = entry.sourceTemplate.match(/\/([^/]+)\/[^/]*\{version\}/);
		if (!slugMatch) return null;
		return resolveSourceforge(entry, slugMatch[1]);
	},
};

/**
 * Renders a `{version}`/`{versionCompact}` sourceTemplate for a live check.
 * versionCompact is only ever needed for sqlite today; computed the same
 * way compile/Makefile's SQLITE_VERSION_COMPACT is.
 */
function renderTemplate(sourceTemplate, version) {
	const [maj, min = 0, pat = 0] = version.split('.').map(Number);
	const versionCompact = String(maj * 1000000 + min * 10000 + pat * 100);
	// replaceAll: most templates carry {version} twice (tag + filename, e.g.
	// ".../v{version}/enchant-{version}.tar.gz") — a plain replace() left the
	// second one unrendered and every such URL failed the live check.
	return sourceTemplate.replaceAll('{version}', version).replaceAll('{versionCompact}', versionCompact);
}

/**
 * Walks one matrix.json collection (`libraries` or `extensions`), refreshing
 * each entry's `versions` array in place. `resolveSourceType` decides, per
 * entry, which resolver in `RESOLVERS` to use — `libraries` entries opt in
 * via an explicit `sourceType`; `extensions` entries don't carry one, so
 * any entry with a `repo` is assumed GitHub-hosted.
 */
async function updateCollection(collection, resolveSourceType) {
	let changed = 0;

	for (const [name, entry] of Object.entries(collection)) {
		if (entry.autoUpdate === false) {
			continue;
		}
		const sourceType = resolveSourceType(entry);
		const resolver = RESOLVERS[sourceType];
		if (!resolver) {
			continue;
		}
		if (!Array.isArray(entry.versions)) {
			// e.g. matrix.json's "haru" entry — a repo is recorded but no
			// version has been pinned/tried yet. Nothing to compare against.
			continue;
		}

		let latest;
		try {
			latest = await resolver(entry, sourceType);
		} catch (error) {
			console.warn(`⚠️  ${name}: ${error.message}`);
			continue;
		}
		if (!latest) {
			console.warn(`⚠️  ${name}: could not resolve a version`);
			continue;
		}

		const current = entry.versions[entry.versions.length - 1];
		// normalizeVersion() always strips a leading "v". Some entries — our
		// own GitHub-tag-sourced extensions (mdhtml, jsonk, navicat, norm,
		// apcu) — deliberately keep it, since their `sourceTemplate` doesn't
		// add its own "v" the way e.g. libavif's does. Detect that
		// convention from what's already recorded and re-apply it, or every
		// check reports a spurious "vX.Y.Z → X.Y.Z" no-op update.
		const keepsVPrefix = /^v\d/i.test(current ?? '');
		const resolved = keepsVPrefix && !/^v/i.test(latest) ? `v${latest}` : latest;

		if (resolved === current || entry.versions.includes(resolved)) {
			// Already current, or already recorded elsewhere in the array
			// (typically a version that was deliberately reordered away from
			// "latest" after failing to build — see e.g. matrix.json's own
			// libjpeg note, CLAUDE.md decision 26/33). Don't report a
			// misleading "current → latest" bump for either case.
			continue;
		}

		if (entry.sourceTemplate) {
			// Check the URL the build will actually download: the build
			// substitutes the recorded version, "v" included for entries
			// that keep it (their sourceTemplate adds no "v" of its own).
			const renderedUrl = renderTemplate(entry.sourceTemplate, resolved);
			if (!(await urlResolves(renderedUrl))) {
				console.warn(`⚠️  ${name}: resolved ${resolved} but its download URL doesn't work (${renderedUrl}) — not recording it. Check manually.`);
				continue;
			}
		}

		console.log(`${name}: ${current ?? '(none)'} → ${resolved}`);
		entry.versions.push(resolved);
		changed++;
	}

	return changed;
}

export async function updateLibVersions({ write = false } = {}) {
	const matrix = JSON.parse(readFileSync(matrixPath, 'utf8'));
	let changed = 0;

	changed += await updateCollection(matrix.libraries, (lib) => lib.sourceType);
	changed += await updateCollection(matrix.extensions, (ext) => (ext.repo ? 'github-release' : undefined));

	if (write) {
		// Refresh lastChecked on every real (non-dry-run) check, not just
		// ones that changed something -- it records when the matrix was
		// last actually verified against upstream, not just when it was
		// last edited.
		matrix.lastChecked = new Date().toISOString();
		writeFileSync(matrixPath, JSON.stringify(matrix, null, '\t') + '\n');
	}

	if (changed === 0) {
		console.log('✨ All auto-checkable libraries and extensions are already up to date!');
	} else if (write) {
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
