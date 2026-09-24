// ---------------------------------------------------------------------------
// Release script — rebuilds every mode:shared extension, then packs and
// publishes to npm only the @kirigami/phpext-<name> packages whose exact
// version isn't already on the registry (CLAUDE.md decision 38). Adapted
// from ../kirigami/scripts/publish.js's own "already published?" idiom —
// same ecosystem convention, no jsDelivr purge here (these packages ship no
// JSON schemas).
//
//   node scripts/publish.js [options]
//
//     --dry-run     rebuild + pack + `npm publish --dry-run`; no registry write
//     --skip-build  skip the "node compile/cli.mjs compile-extension" rebuild
//                   step (e.g. artifacts were already rebuilt this session)
//     --only <name> act on a single package ("sodium" or "@kirigami/phpext-sodium")
//     --otp <code>  seed the first `npm publish` with this OTP
//     --yes         skip the confirmation prompt (for CI / scripted runs)
//
// Versioning is NOT decided here: `compile/cli.mjs compile-extension` only
// bumps a package's patch version when the freshly compiled output's hash
// actually differs from the last recorded one (see CLAUDE.md decision 38) —
// this script just publishes whatever that left behind, skipping anything
// already on the registry. Interactive by design, same reasoning as the
// kirigami script: prints the plan and waits for confirmation, and each
// `npm publish` inherits the terminal so npm's own 2FA flow works in place.
// ---------------------------------------------------------------------------

import { execSync } from 'node:child_process';
import { readFileSync, readdirSync, existsSync, mkdirSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { createInterface } from 'node:readline/promises';
import path from 'node:path';
import { parseArgs } from 'node:util';

const ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const PKG_DIR = path.join(ROOT, 'packages');
const PACKS = path.join(ROOT, 'packs');

const { values: opt } = parseArgs({
	options: {
		'dry-run': { type: 'boolean', default: false },
		'skip-build': { type: 'boolean', default: false },
		only: { type: 'string' },
		otp: { type: 'string' },
		yes: { type: 'boolean', default: false },
	},
});

const dim = (s) => `\x1b[2m${s}\x1b[0m`;
const yellow = (s) => `\x1b[33m${s}\x1b[0m`;
const green = (s) => `\x1b[32m${s}\x1b[0m`;

const q = (s) => (/[\s"]/.test(s) ? `"${String(s).replace(/"/g, '\\"')}"` : String(s));

function capture(line, cwd = ROOT) {
	return execSync(line, { cwd, encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe'] }).trim();
}
function inherit(line, cwd = ROOT) {
	execSync(line, { cwd, stdio: 'inherit' });
}

async function confirm(question) {
	if (opt.yes) return true;
	if (!process.stdin.isTTY) {
		console.log(yellow('\nnot a TTY — cannot prompt. Re-run in a terminal, or pass --yes.'));
		return false;
	}
	const rl = createInterface({ input: process.stdin, output: process.stdout });
	const answer = (await rl.question(`${question} ${dim('(y/N)')} `)).trim().toLowerCase();
	rl.close();
	return answer === 'y' || answer === 'yes';
}

// ── discover the publishable packages ─────────────────────────────────────
function readPackages() {
	const pkgs = new Map();
	if (!existsSync(PKG_DIR)) return pkgs;
	for (const dir of readdirSync(PKG_DIR)) {
		const file = path.join(PKG_DIR, dir, 'package.json');
		if (!existsSync(file)) continue;
		const json = JSON.parse(readFileSync(file, 'utf8'));
		if (json.private) continue;
		pkgs.set(json.name, { name: json.name, dir: path.join(PKG_DIR, dir), json });
	}
	return pkgs;
}

// ── is this exact name@version already on the registry? ───────────────────
function alreadyPublished(name, version) {
	try {
		return capture(`npm view ${name}@${version} version`) === version;
	} catch {
		return false; // never published, or offline — let the publish surface it
	}
}

function packAndPublish(pkg) {
	const { name, dir, json } = pkg;
	const { version } = json;

	mkdirSync(PACKS, { recursive: true });
	const packed = capture(`npm pack --json --pack-destination ${q(PACKS)}`, dir);
	// npm <= 11 prints an array; npm 12 an object keyed by package name.
	const [pack] = Object.values(JSON.parse(packed));
	const tarball = path.join(PACKS, pack.filename);
	console.log(`  ${dim('·')} ${name}: packed ${dim(path.relative(ROOT, tarball))}`);

	let cmd = `npm publish ${q(tarball)} --access public`;
	if (opt['dry-run']) cmd += ' --dry-run';
	if (opt.otp) cmd += ` --otp ${q(opt.otp)}`;
	inherit(cmd, dir);
	console.log(`  ${green('✓')} ${name}@${version} published`);
}

// ── preflight ────────────────────────────────────────────────────────────
function preflightWarnings() {
	const warnings = [];
	try {
		if (capture('git status --porcelain', ROOT)) warnings.push('working tree has uncommitted changes');
	} catch {
		/* not a git checkout — ignore */
	}
	return warnings;
}

// ── main ─────────────────────────────────────────────────────────────────
async function main() {
	if (!opt['skip-build']) {
		console.log('=== Rebuilding mode:shared extensions ===');
		console.log(dim('(node compile/cli.mjs compile-extension — needs Docker)'));
		inherit('node compile/cli.mjs compile-extension');
	}

	const allPkgs = readPackages();
	let pkgs = allPkgs;

	if (opt.only) {
		const want = opt.only.startsWith('@') ? opt.only : `@kirigami/phpext-${opt.only}`;
		if (!allPkgs.has(want)) {
			console.error(`Unknown package "${opt.only}". Known: ${[...allPkgs.keys()].join(', ')}`);
			process.exit(1);
		}
		pkgs = new Map([[want, allPkgs.get(want)]]);
	}

	const ordered = [...pkgs.values()];
	const toPublish = ordered.filter((p) => !alreadyPublished(p.name, p.json.version));

	console.log('\nRelease plan:');
	for (const p of ordered) {
		const fresh = toPublish.includes(p);
		console.log(
			`  ${fresh ? green('publish') : dim('skip   ')} ${p.name}@${p.json.version}${
				fresh ? '' : dim(' (already on npm)')
			}`
		);
	}
	if (ordered.length === 0) {
		console.log(dim('  (no packages under packages/*)'));
	}

	const warnings = preflightWarnings();
	if (warnings.length) {
		console.log(yellow('\nHeads up:'));
		for (const w of warnings) console.log(`  ${yellow('⚠')} ${w}`);
	}

	if (toPublish.length === 0) {
		console.log('\nNothing to publish.');
		return;
	}

	if (opt['dry-run']) {
		console.log(yellow('\n--dry-run: publishing with --dry-run.\n'));
	} else if (!(await confirm('\nProceed?'))) {
		console.log('Aborted.');
		process.exit(1);
	}

	const summary = { published: [], skipped: [] };
	for (const pkg of ordered) {
		const id = `${pkg.name}@${pkg.json.version}`;
		if (!toPublish.includes(pkg)) {
			console.log(`  ${dim('=')} ${id} ${dim('already on npm, skipped')}`);
			summary.skipped.push(id);
			continue;
		}
		packAndPublish(pkg);
		summary.published.push(id);
	}

	console.log('\n─────────────────────────────');
	if (summary.published.length) console.log(`published: ${summary.published.join(', ')}`);
	if (summary.skipped.length) console.log(dim(`skipped:   ${summary.skipped.join(', ')}`));
	if (opt['dry-run']) console.log(yellow('dry run — nothing was actually published'));
}

main().catch((err) => {
	console.error(`\n${yellow('release failed:')} ${err.message}`);
	process.exit(1);
});
