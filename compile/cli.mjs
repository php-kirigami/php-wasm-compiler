#!/usr/bin/env node
// Single entry point for the compiler. Reads config.yaml (repo root),
// validates the requested extensions, then drives build.js once per PHP
// version. See CLAUDE.md (decisions 9-12) for the context behind this file.
import {
	readFileSync,
	writeFileSync,
	mkdirSync,
	existsSync,
	rmSync,
	cpSync,
	readdirSync,
} from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { spawn, execFileSync } from 'node:child_process';
import { createHash } from 'node:crypto';

import { parse as parseYaml, stringify as stringifyYaml } from 'yaml';
import prompts from 'prompts';
import yargs from 'yargs';
import { hideBin } from 'yargs/helpers';

import { updatePHPVersions } from './update-php-versions.mjs';
import { updateLibVersions } from './update-lib-versions.mjs';
import { getMatrixVersion, getMatrixExtensionVersion } from './matrix-version.mjs';
import { phpVersions as SUPPORTED_PHP_VERSIONS } from '../supported-php-versions.mjs';

const sourceDir = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(sourceDir, '..');

// This project targets Node 24+ across the board (see CLAUDE.md).
const MIN_NODE_MAJOR = 24;

function checkNodeVersion() {
	const major = Number(process.versions.node.split('.')[0]);
	if (major < MIN_NODE_MAJOR) {
		console.error(
			`php-wasm-compiler requires Node.js >= ${MIN_NODE_MAJOR} (found ${process.version}).`
		);
		process.exit(1);
	}
}

function checkDocker() {
	try {
		execFileSync('docker', ['version', '--format', '{{.Server.Version}}'], {
			stdio: 'pipe',
		});
	} catch {
		console.error(
			'Docker does not seem to be installed or running. Install/start ' +
				'Docker Desktop (or Docker Engine) before building.'
		);
		process.exit(1);
	}
}

/**
 * `build.js` shells out to `make base-image` (see compile/build.js), and
 * `compile/Makefile` drives every third-party lib build. GNU Make ships by
 * default on Linux/macOS CI runners, but not on Windows — it has to be
 * installed separately there (e.g. via Chocolatey, Scoop, MSYS2, or inside
 * WSL).
 */
function checkMake() {
	try {
		execFileSync('make', ['--version'], { stdio: 'pipe' });
	} catch {
		console.error(
			'GNU Make does not seem to be installed or available on PATH. ' +
				(process.platform === 'win32'
					? 'On Windows, install it separately (e.g. "choco install make", ' +
						'via Scoop/MSYS2, or run this from inside WSL).'
					: 'Install it via your system package manager.')
		);
		process.exit(1);
	}
}

/**
 * `compile/Makefile`'s recipes use POSIX shell syntax and coreutils
 * (`mkdir -p`, `rm -rf`, `mv ... || true`, `$$(...)` command substitution,
 * plus this Makefile's own `CID=$$(docker create ...) && docker cp ... &&
 * docker rm $$CID` pattern). None of that is native to Windows (`cmd.exe`'s
 * `mkdir`/`rmdir` don't understand `-p`/`-rf`, and it has no `sh`): a POSIX
 * shell + coreutils has to be installed separately, e.g. via Git for
 * Windows (Git Bash), MSYS2, or by running everything from inside WSL.
 */
function checkPosixTools() {
	if (process.platform !== 'win32') {
		return;
	}
	try {
		execFileSync('sh', ['-c', 'command -v mkdir rm mv cp'], { stdio: 'pipe' });
	} catch {
		console.error(
			'A POSIX shell + coreutils (mkdir -p, rm -rf, mv, sh) does not seem ' +
				'to be on PATH. On Windows, install Git for Windows (provides Git ' +
				'Bash / sh.exe) or MSYS2, or run this from inside WSL.'
		);
		process.exit(1);
	}
}

const BASE_IMAGE_TAG = 'kirigami-php-wasm:base';

/**
 * `compile/Makefile`'s `base-image` target is keyed on base-image/.ready's
 * mtime (see CLAUDE.md decision 25's caching fix), not on whether the
 * Docker image it built actually still exists — a marker that survives an
 * external reset of Docker's own image store (`docker system prune`, a
 * WSL2 `.vhdx` wipe, a fresh Docker Desktop install) makes `make` think
 * base-image is already satisfied, and every subsequent lib build then
 * fails with "pull access denied" trying to pull it from a registry
 * instead of building it locally (CLAUDE.md decision 35 hit this exact
 * failure once already). Detect the mismatch here and delete the stale
 * marker so `make base-image` rebuilds it for real on the next run.
 */
function checkBaseImage() {
	const readyPath = path.join(repoRoot, 'compile', 'base-image', '.ready');
	if (!existsSync(readyPath)) {
		return;
	}
	let imageExists = true;
	try {
		execFileSync('docker', ['image', 'inspect', BASE_IMAGE_TAG], { stdio: 'pipe' });
	} catch {
		imageExists = false;
	}
	if (!imageExists) {
		console.log(
			`${BASE_IMAGE_TAG} not found in Docker despite compile/base-image/.ready ` +
				'existing — removing the stale marker so `make` rebuilds the base ' +
				'image for real instead of trying to pull it.'
		);
		rmSync(readyPath);
	}
}

/**
 * Windows-only: this project keeps local dev environments close to the
 * Linux CI runners it will eventually build on (see CLAUDE.md, decision 8),
 * so on Windows we require running through WSL with an Ubuntu distro that
 * has Node >= MIN_NODE_MAJOR, rather than the Windows-native Node build.
 */
function checkWindowsWslStack() {
	if (process.platform !== 'win32') {
		return;
	}

	let distros;
	try {
		distros = execFileSync('wsl', ['-l', '-q'], { encoding: 'utf16le' })
			.split('\n')
			.map((line) => line.replace(/\0/g, '').trim())
			.filter(Boolean);
	} catch {
		console.error(
			'WSL does not seem to be installed. Run "wsl --install", restart, ' +
				'then install an Ubuntu distribution.'
		);
		process.exit(1);
	}

	const ubuntuDistro = distros.find((name) => /ubuntu/i.test(name));
	if (!ubuntuDistro) {
		console.error(
			'WSL is installed but no Ubuntu distribution was found ' +
				`(found: ${distros.join(', ') || 'none'}). Install one with ` +
				'"wsl --install -d Ubuntu".'
		);
		process.exit(1);
	}

	// nvm only shadows the system Node on PATH via lines it appends to
	// .bashrc — and .bashrc itself starts with a guard clause that returns
	// immediately for non-interactive shells (the standard Debian/Ubuntu
	// skeleton), so neither a plain `wsl -- node -v` nor `bash -lc` picks it
	// up. Source nvm.sh explicitly instead, the same way `nvm`'s own install
	// instructions do for non-interactive use.
	let wslNodeVersion;
	try {
		wslNodeVersion = execFileSync(
			'wsl',
			[
				'-d',
				ubuntuDistro,
				'--',
				'bash',
				'-lc',
				'[ -s "$HOME/.nvm/nvm.sh" ] && . "$HOME/.nvm/nvm.sh"; node -v',
			],
			{ encoding: 'utf8' }
		).trim();
	} catch {
		wslNodeVersion = '';
	}
	const wslNodeMajor = Number(wslNodeVersion.replace(/^v/, '').split('.')[0]);
	if (!wslNodeMajor || wslNodeMajor < MIN_NODE_MAJOR) {
		console.error(
			`Node.js inside WSL (${ubuntuDistro}) is ` +
				`${wslNodeVersion || 'not installed'}, but >= ${MIN_NODE_MAJOR} is ` +
				`required. Install/update Node inside that WSL distro.`
		);
		process.exit(1);
	}
}

// Extensions already wired into compile/php/Dockerfile today (config.yaml
// key -> Dockerfile ARG). Any extension listed in config.yaml but missing
// from here can only be in mode "off" (see validateExtensions).
const IMPLEMENTED_EXTENSIONS = {
	cli_sapi: 'WITH_CLI_SAPI',
	libzip: 'WITH_LIBZIP',
	sqlite: 'WITH_SQLITE',
	curl: 'WITH_CURL',
	fileinfo: 'WITH_FILEINFO',
	iconv: 'WITH_ICONV',
	libxml: 'WITH_LIBXML',
	soap: 'WITH_SOAP',
	exif: 'WITH_EXIF',
	gd: 'WITH_GD',
	mbstring: 'WITH_MBSTRING',
	mbregex: 'WITH_MBREGEX',
	openssl: 'WITH_OPENSSL',
	ws_networking_proxy: 'WITH_WS_NETWORKING_PROXY',
	opcache: 'WITH_OPCACHE',
	imagick: 'WITH_IMAGICK',
	mysql: 'WITH_MYSQL',
	nodefs: 'WITH_NODEFS',
	yaml: 'WITH_YAML',
	mdhtml: 'WITH_MDHTML',
};

// These have no independent Dockerfile flag of their own: compile/php/Dockerfile
// always turns them on together with their "host" extension (config.yaml key
// -> host extension key). dom/simplexml/xmlreader/xmlwriter ride along with
// WITH_LIBXML=yes ("Add Libxml2 if needed" block); pdo/pdo_sqlite ride along
// with WITH_SQLITE=yes. Each bundled extension's mode must always match its
// host's mode (enforced in validateExtensions).
const BUNDLED_EXTENSIONS = {
	dom: 'libxml',
	simplexml: 'libxml',
	xmlreader: 'libxml',
	xmlwriter: 'libxml',
	pdo: 'sqlite',
	pdo_sqlite: 'sqlite',
};

/**
 * Third-party lib(s) a given extension needs, as compile/Makefile target
 * names — inspired by php-static-autobuilder's per-extension dep list
 * (CLAUDE.md decision 25): only build what the enabled extensions actually
 * need instead of always running the full `make all_jspi`. Only the direct
 * "leaf" target is listed; Makefile's own prerequisite chain (e.g.
 * libcurl_jspi -> libz_jspi, libopenssl_jspi, libssh2_jspi, nghttp2_jspi)
 * pulls in the rest automatically. Extensions with no entry here (cli_sapi,
 * fileinfo, soap, exif, mbstring, ws_networking_proxy, opcache, mysql,
 * nodefs) compile from php-src's own bundled sources and need no vendored
 * lib. `libzip` only needs the 1.9.2 build — php/Dockerfile's "Add Libzip"
 * block never reads the 1.2.0 one it also happens to build (a pre-existing,
 * harmless orphan in the full `make all_jspi`; see also libaom, unused by
 * libavif/libgd in this build).
 */
const LIB_TARGETS_BY_EXTENSION = {
	libzip: ['libzip-1.9.2_jspi'],
	sqlite: ['libsqlite3_jspi'],
	curl: ['libcurl_jspi'],
	iconv: ['libiconv_jspi'],
	libxml: ['libxml2_jspi'],
	gd: ['libgd_jspi'],
	mbregex: ['oniguruma_jspi'],
	openssl: ['libopenssl_jspi'],
	imagick: ['libImageMagick_jspi'],
	yaml: ['libyaml_jspi'],
	mdhtml: ['libcmark-gfm_jspi'],
};

/** Union of Makefile targets needed by every extension currently set to "static". */
function computeRequiredLibTargets(config) {
	const extensions = config.extensions ?? {};
	const targets = new Set();
	for (const [name, entry] of Object.entries(extensions)) {
		if ((entry?.mode ?? 'off') !== 'static') {
			continue;
		}
		for (const target of LIB_TARGETS_BY_EXTENSION[name] ?? []) {
			targets.add(target);
		}
	}
	return [...targets];
}

/**
 * Runs `make <targets...>` in compile/ so only the libs the enabled
 * extensions need get (re)built. Safe to call every run: compile/Makefile's
 * targets are keyed on the built .a file already existing, so an
 * already-built lib is a fast no-op (only `base-image` always re-runs, a
 * quick cached `docker build`).
 */
function runLibBuild(targets) {
	if (targets.length === 0) {
		console.log('\nNo third-party libraries needed for the enabled extensions.');
		return Promise.resolve();
	}
	console.log(`\n=== Building required libraries: ${targets.join(', ')} ===`);
	return new Promise((resolve, reject) => {
		const child = spawn('make', targets, {
			cwd: path.join(repoRoot, 'compile'),
			stdio: 'inherit',
		});
		child.on('close', (code) => {
			if (code === 0) {
				resolve();
			} else {
				reject(new Error(`make failed (code ${code}) building: ${targets.join(', ')}`));
			}
		});
	});
}

async function main() {
	checkNodeVersion();

	await yargs(hideBin(process.argv))
		.scriptName('php-wasm-compiler')
		.usage('Usage: $0 <command> [options]')
		.command(
			['build', '$0'],
			'Build PHP + enabled extensions from config.yaml (default command)',
			(y) =>
				y.options({
					config: {
						type: 'string',
						default: path.join(repoRoot, 'config.yaml'),
						describe: 'Path to the YAML configuration file',
					},
					quiet: {
						type: 'boolean',
						default: false,
						describe: 'Skip interactive prompts, use config.yaml as-is (CI usage)',
					},
					save: {
						type: 'boolean',
						default: true,
						describe:
							'In interactive mode, offer to save changes back to config.yaml',
					},
					'dry-run': {
						type: 'boolean',
						default: false,
						describe:
							'Print the commands that would run, without building anything',
					},
				}),
			runBuildCommand
		)
		.command(
			'update-versions',
			'Refresh supported-php-versions.mjs (PHP) and matrix.json (third-party ' +
				'libraries) from upstream — run this before a release (CLAUDE.md ' +
				'decision 11), not something a normal build needs',
			(y) =>
				y.options({
					write: {
						type: 'boolean',
						default: false,
						describe:
							'Persist matrix.json changes (otherwise a dry-run report). ' +
							'supported-php-versions.mjs is always written — it has no ' +
							'dry-run mode of its own.',
					},
				}),
			runUpdateVersionsCommand
		)
		.command(
			'compile-extension [name]',
			'Compile one (or, with no name, every) mode:shared extension from ' +
				'config.yaml into a standalone WASM side module via ' +
				'@php-wasm/compile-extension (CLAUDE.md decisions 5, 30, 31) — a ' +
				'separate command from "build" since these are not linked into ' +
				'the main php.wasm',
			(y) =>
				y
					.positional('name', {
						type: 'string',
						describe:
							'Extension name to build (default: every mode:shared extension)',
					})
					.options({
						config: {
							type: 'string',
							default: path.join(repoRoot, 'config.yaml'),
							describe: 'Path to the YAML configuration file',
						},
						'dry-run': {
							type: 'boolean',
							default: false,
							describe:
								'Print the commands that would run, without building anything',
						},
					}),
			runCompileExtensionCommand
		)
		.demandCommand(0, 1)
		.help()
		.parseAsync();
}

async function runBuildCommand(argv) {
	checkDocker();
	checkMake();
	checkPosixTools();
	checkWindowsWslStack();
	checkBaseImage();

	const config = loadConfig(argv.config);

	if (!argv.quiet) {
		const shouldSave = await interactiveReview(config);
		if (shouldSave && argv.save) {
			saveConfig(argv.config, config);
			console.log(`Configuration saved to ${argv.config}`);
		}
	}

	validateExtensions(config);

	const requiredLibTargets = computeRequiredLibTargets(config);
	if (argv['dry-run']) {
		console.log(
			`\n[dry-run] make ${requiredLibTargets.join(' ') || '(no lib targets needed)'}`
		);
	} else {
		await runLibBuild(requiredLibTargets);
	}

	const versions = config.php?.versions ?? [];
	if (versions.length === 0) {
		throw new Error('config.yaml: php.versions is empty.');
	}

	for (const phpVersion of versions) {
		const args = buildArgsForVersion(config, phpVersion);
		if (argv['dry-run']) {
			console.log(`\n[dry-run] node build.js ${args.join(' ')}`);
			continue;
		}
		await runBuild(phpVersion, args);
	}
}

async function runUpdateVersionsCommand(argv) {
	console.log('=== PHP versions (supported-php-versions.mjs) ===');
	await updatePHPVersions();

	console.log('\n=== Third-party libraries (matrix.json) ===');
	await updateLibVersions({ write: argv.write });
}

/**
 * A real (non-internal) shared extension's compiled output — manifest.json +
 * one .so per PHP version — is already everything an @kirigami/phpext-<name>
 * npm package needs to ship (CLAUDE.md decision 31): no extra JS glue, the
 * consumer loads it straight via @php-wasm/universal's
 * `{ format: 'manifest', manifestUrl: ... }`. This just adds the package.json
 * that output directory needs to actually be `npm publish`-able.
 *
 * Package name/output-dir convention: `@kirigami/phpext-<name>` (CLAUDE.md
 * decision 38) — the pattern a future `@kirigami/php-wasm` dependency scanner
 * (kirigami repo, not this one) globs for to auto-load installed extensions,
 * per the user's 2026-09-15 direction. manifest.json already carries the
 * ini/env directives that scanner needs (CLAUDE.md decision 5/30) — no
 * separate .ini file is shipped.
 *
 * `kirigami` field: same `type`-discriminated metadata section every
 * @kirigami/plugin-<name> package.json already carries (see the "kirigami"
 * key in ../kirigami/packages/plugin-embed/package.json, `type: "plugin"`)
 * — `type: "extension"` here, per CLAUDE.md decision 5's own terminology.
 */
function extensionPackageJson(name, version, kirigami) {
	return {
		name: `@kirigami/phpext-${name}`,
		version,
		description: `Kirigami PHP.wasm shared extension: ${name} (JSPI side module, see @kirigami/php-wasm)`,
		keywords: ['php-wasm', 'php', 'wasm', 'webassembly', 'kirigami', name],
		homepage: 'https://php-kirigami.github.io',
		bugs: { url: 'https://github.com/php-kirigami/php-wasm-compiler/issues' },
		repository: {
			type: 'git',
			url: 'git+https://github.com/php-kirigami/php-wasm-compiler.git',
		},
		publishConfig: { access: 'public' },
		license: 'GPL-2.0-or-later',
		author: 'Maxime Larrivée-Roy',
		files: ['manifest.json', '*.so', 'README.md'],
		kirigami,
	};
}

/**
 * Hash of the actual compiled output (manifest.json + every *.so, name and
 * bytes) — used to decide whether a rebuild actually changed anything worth
 * publishing (CLAUDE.md decision 38's answer to "don't publish packages for
 * nothing"). Deterministic: file names are sorted before hashing.
 */
function computeBuildHash(outDir) {
	const files = readdirSync(outDir)
		.filter((f) => f === 'manifest.json' || f.endsWith('.so'))
		.sort();
	const hash = createHash('sha256');
	for (const file of files) {
		hash.update(file);
		hash.update(readFileSync(path.join(outDir, file)));
	}
	return hash.digest('hex');
}

function bumpPatchVersion(version) {
	const match = /^(\d+)\.(\d+)\.(\d+)$/.exec(version);
	if (!match) return '0.1.0';
	const [, major, minor, patch] = match;
	return `${major}.${minor}.${Number(patch) + 1}`;
}

function compareSemver(a, b) {
	const pa = a.split('.').map(Number);
	const pb = b.split('.').map(Number);
	for (let i = 0; i < 3; i++) {
		if (pa[i] !== pb[i]) return pa[i] - pb[i];
	}
	return 0;
}

/**
 * The full PHP patch version (e.g. "8.5.10") each major.minor entry in
 * config.yaml's php.versions resolves to, per supported-php-versions.mjs —
 * the same source build.js itself uses to pick which PHP tag to download.
 */
function resolvePhpFullVersions(phpVersionList) {
	return phpVersionList.map((version) => {
		const entry = SUPPORTED_PHP_VERSIONS.find((p) => p.version === version);
		if (!entry) {
			throw new Error(`supported-php-versions.mjs has no entry for PHP ${version}.`);
		}
		return entry.lastRelease;
	});
}

/**
 * Builds the `kirigami` package.json section for a shared extension
 * (CLAUDE.md decision 38's follow-up, 2026-09-15): `phpVersions` (major.minor
 * list this build ships), `minVersion` (the oldest full PHP patch version
 * among them — not a @kirigami/php-wasm semver, which this repo has no
 * visibility into; the PHP version actually compiled/tested against is the
 * honest thing to record), the vendored third-party library + its
 * matrix.json-pinned version when the extension has one, and the same
 * build-output hash `.buildhash` tracks (exposed here too so a consumer can
 * read it straight off package.json without a second file).
 */
function buildKirigamiExtensionMetadata(ext, phpVersionList, buildHash) {
	const fullVersions = resolvePhpFullVersions(phpVersionList);
	const minVersion = fullVersions.reduce((min, v) => (compareSemver(v, min) < 0 ? v : min));

	const kirigami = {
		type: 'extension',
		phpVersions: phpVersionList,
		minVersion,
	};
	if (ext.vendorLib) {
		kirigami.vendorLib = { name: ext.vendorLib, version: getMatrixVersion(ext.vendorLib) };
	}
	kirigami.buildHash = buildHash;
	return kirigami;
}

/**
 * Writes/updates a shared extension's package.json, bumping its patch
 * version only when the freshly compiled output's hash differs from the one
 * recorded in `.buildhash` at the last write — i.e. only when there is
 * actually something new to publish. Both files are committed (mechanically
 * maintained, not hand-edited — see .gitignore and CLAUDE.md decision 38).
 */
function syncExtensionPackage(outDir, ext, phpVersionList) {
	const packageJsonPath = path.join(outDir, 'package.json');
	const buildHashPath = path.join(outDir, '.buildhash');

	const newHash = computeBuildHash(outDir);
	const previousHash = existsSync(buildHashPath)
		? readFileSync(buildHashPath, 'utf8').trim()
		: null;
	const previousVersion = existsSync(packageJsonPath)
		? JSON.parse(readFileSync(packageJsonPath, 'utf8')).version
		: '0.1.0';

	const changed = newHash !== previousHash;
	const version = changed && previousHash !== null ? bumpPatchVersion(previousVersion) : previousVersion;
	const kirigami = buildKirigamiExtensionMetadata(ext, phpVersionList, newHash);

	writeFileSync(
		packageJsonPath,
		JSON.stringify(extensionPackageJson(ext.name, version, kirigami), null, '\t') + '\n',
		'utf8'
	);
	writeFileSync(buildHashPath, newHash + '\n', 'utf8');

	return { version, changed };
}

function resolveCompileExtensionBin() {
	const binName =
		process.platform === 'win32'
			? 'php-wasm-compile-extension.cmd'
			: 'php-wasm-compile-extension';
	const binPath = path.join(repoRoot, 'compile', 'node_modules', '.bin', binName);
	if (!existsSync(binPath)) {
		throw new Error(
			'@php-wasm/compile-extension is not installed. Run "npm install" in ' +
				'compile/ first (see CLAUDE.md decision 30).'
		);
	}
	return binPath;
}

/**
 * `--extra-cflags`/`--extra-ldflags` paths for a vendored dependency lib
 * staged under <extension source>/vendor/<lib>/ (see `stageVendorLib()`) —
 * pure string computation so it can also be shown in `--dry-run`.
 */
function vendorLibFlags(vendorLib) {
	return {
		cflags: `-I/build/vendor/${vendorLib}/include`,
		ldflags: `/build/vendor/${vendorLib}/lib/${vendorLib}.a`,
	};
}

/**
 * Copies an already-built compile/<lib>/jspi/dist/ output into a shared
 * extension's own source directory, so `@php-wasm/compile-extension` (which
 * only ever sees the `--source` directory, copied to `/build` inside its
 * container) can see it too. Mirrors the README's own "build the dependency
 * yourself, pass /build/... paths" pattern (CLAUDE.md decision 32) — used
 * for extensions with an external C library dependency the extension
 * container doesn't provide (e.g. sodium -> libsodium).
 */
function stageVendorLib(extSourceDir, vendorLib) {
	const libDistDir = path.join(repoRoot, 'compile', vendorLib, 'jspi', 'dist', 'root', 'lib');
	if (!existsSync(libDistDir)) {
		throw new Error(
			`${vendorLib} hasn't been built yet — run "make ${vendorLib}_jspi" in compile/ first.`
		);
	}
	const vendorDir = path.join(extSourceDir, 'vendor', vendorLib);
	rmSync(vendorDir, { recursive: true, force: true });
	mkdirSync(vendorDir, { recursive: true });
	cpSync(libDistDir, vendorDir, { recursive: true });
}

async function runCompileExtensionCommand(argv) {
	const config = loadConfig(argv.config);
	validateExtensions(config);

	const shared = getSharedExtensions(config);
	const selected = argv.name ? shared.filter((ext) => ext.name === argv.name) : shared;

	if (argv.name && selected.length === 0) {
		throw new Error(`config.yaml has no mode:shared extension named "${argv.name}".`);
	}
	if (selected.length === 0) {
		console.log('No mode:shared extensions declared in config.yaml.');
		return;
	}

	const phpVersionList = config.php?.versions ?? [];
	const phpVersions = phpVersionList.join(',');
	if (!phpVersions) {
		throw new Error('config.yaml: php.versions is empty.');
	}

	if (!argv['dry-run']) {
		checkDocker();
	}
	const compileExtensionBin = resolveCompileExtensionBin();

	for (const ext of selected) {
		const extSourceDir = path.resolve(repoRoot, ext.source);
		const isInternal = Boolean(ext.internal);
		// A real extension's output dir IS its future @kirigami/phpext-<name>
		// package content — an internal fixture's output is just a throwaway
		// test artifact, so it stays under node-builds/ instead of packages/.
		const outDir = isInternal
			? path.join(repoRoot, config.output?.dir ?? 'node-builds', 'extensions', ext.name)
			: path.join(repoRoot, 'packages', `phpext-${ext.name}`);

		let extraCflags = ext.extraCflags ?? '';
		let extraLdflags = ext.extraLdflags ?? '';
		let configArgs = ext.configArgs ?? '';

		if (ext.vendorLib) {
			const { cflags, ldflags } = vendorLibFlags(ext.vendorLib);
			extraCflags = [cflags, extraCflags].filter(Boolean).join(' ');
			extraLdflags = [ldflags, extraLdflags].filter(Boolean).join(' ');
			if (ext.pkgConfigVar) {
				// ext.configArgs' config.m4 PKG_CHECK_MODULES([<var>], ...) skips
				// its own pkg-config probe when these are already set (there's no
				// real pkg-config/*.pc for a vendored lib inside the container).
				configArgs = [
					`${ext.pkgConfigVar}_CFLAGS=${cflags}`,
					`${ext.pkgConfigVar}_LIBS=${ldflags}`,
					configArgs,
				]
					.filter(Boolean)
					.join(' ');
			}
		}

		const args = [
			'--source',
			extSourceDir,
			'--name',
			ext.name,
			'--php-versions',
			phpVersions,
			'--out',
			outDir,
		];
		if (extraCflags) args.push('--extra-cflags', extraCflags);
		if (extraLdflags) args.push('--extra-ldflags', extraLdflags);
		if (configArgs) args.push('--config-args', configArgs);

		if (argv['dry-run']) {
			console.log(`\n[dry-run] ${compileExtensionBin} ${args.join(' ')}`);
			if (!isInternal) {
				console.log(`[dry-run] write ${path.join(outDir, 'package.json')}`);
			}
			continue;
		}

		console.log(`\n=== Compiling shared extension "${ext.name}" ===`);
		if (ext.vendorLib) {
			stageVendorLib(extSourceDir, ext.vendorLib);
		}
		mkdirSync(outDir, { recursive: true });
		await runCommand(compileExtensionBin, args);

		if (!isInternal) {
			const { version, changed } = syncExtensionPackage(outDir, ext, phpVersionList);
			console.log(
				changed
					? `@kirigami/phpext-${ext.name}@${version}: output changed, version bumped`
					: `@kirigami/phpext-${ext.name}@${version}: output unchanged, version kept`
			);
		}
	}
}

function runCommand(command, args) {
	return new Promise((resolve, reject) => {
		// npm-installed .bin wrappers are .cmd shims on Windows; Node's spawn()
		// needs shell:true to run those directly (EINVAL otherwise). Unlike the
		// non-shell Windows path, spawn() does NOT quote args itself when
		// shell:true — an arg containing a space (e.g. a multi-word
		// --config-args value) would otherwise reach cmd.exe unquoted and get
		// split into several arguments.
		const isWindows = process.platform === 'win32';
		const quotedArgs = isWindows
			? args.map((arg) => (/[\s"]/.test(arg) ? `"${arg.replace(/"/g, '\\"')}"` : arg))
			: args;
		const child = spawn(command, quotedArgs, {
			cwd: repoRoot,
			stdio: 'inherit',
			shell: isWindows,
		});
		child.on('close', (code) => {
			if (code === 0) {
				resolve();
			} else {
				reject(new Error(`${command} failed (code ${code})`));
			}
		});
	});
}

function loadConfig(configPath) {
	const raw = readFileSync(configPath, 'utf8');
	return parseYaml(raw);
}

function saveConfig(configPath, config) {
	writeFileSync(configPath, stringifyYaml(config), 'utf8');
}

/**
 * Reject any config that promises more than the Dockerfile can actually do
 * today, instead of failing silently partway through the Docker build.
 */
function validateExtensions(config) {
	const extensions = config.extensions ?? {};
	for (const [name, entry] of Object.entries(extensions)) {
		const mode = entry?.mode ?? 'off';

		if (name in BUNDLED_EXTENSIONS) {
			const host = BUNDLED_EXTENSIONS[name];
			const hostMode = extensions[host]?.mode ?? 'off';
			if (mode !== hostMode) {
				throw new Error(
					`config.yaml: extension "${name}" has mode "${mode}", but it has no ` +
						`Dockerfile flag of its own — compile/php/Dockerfile always turns ` +
						`it on together with "${host}" (currently mode "${hostMode}"). ` +
						`Set "${name}" to the same mode as "${host}".`
				);
			}
			continue;
		}

		if (mode === 'off') {
			continue;
		}

		if (mode === 'shared') {
			if (typeof entry?.source !== 'string' || entry.source.trim() === '') {
				throw new Error(
					`config.yaml: extension "${name}" has mode "shared" but no ` +
						`"source" path (relative to the repo root, pointing at its own ` +
						`config.m4). See CLAUDE.md decision 30/31, and build it with ` +
						`"node compile/cli.mjs compile-extension ${name}".`
				);
			}
			continue;
		}

		if (mode !== 'static') {
			throw new Error(
				`config.yaml: extension "${name}" has an unknown mode "${mode}" ` +
					`(expected static | shared | off).`
			);
		}
		if (!(name in IMPLEMENTED_EXTENSIONS)) {
			throw new Error(
				`config.yaml: extension "${name}" has mode "${mode}" but is not ` +
					`wired into compile/php/Dockerfile yet (see CLAUDE.md, decision 9). ` +
					`Set it to mode: off.`
			);
		}
	}
}

/** Every config.yaml extension entry with mode: shared, name included. */
function getSharedExtensions(config) {
	const extensions = config.extensions ?? {};
	return Object.entries(extensions)
		.filter(([, entry]) => (entry?.mode ?? 'off') === 'shared')
		.map(([name, entry]) => ({ name, ...entry }));
}

async function interactiveReview(config) {
	console.log(`Configuration loaded. Interactive review (Ctrl+C to cancel).\n`);

	const versionsAnswer = await prompts({
		type: 'text',
		name: 'versions',
		message: 'PHP versions to build (comma-separated)',
		initial: (config.php?.versions ?? []).join(', '),
	});
	if (versionsAnswer.versions === undefined) {
		process.exit(1);
	}
	config.php ??= {};
	config.php.versions = versionsAnswer.versions
		.split(',')
		.map((v) => v.trim())
		.filter(Boolean);

	const choices = Object.keys(IMPLEMENTED_EXTENSIONS).map((name) => ({
		title: name,
		value: name,
		selected: (config.extensions?.[name]?.mode ?? 'off') === 'static',
	}));
	const extAnswer = await prompts({
		type: 'multiselect',
		name: 'enabled',
		message: 'Extensions to compile statically',
		hint: '- Space to select, Enter to confirm',
		instructions: false,
		choices,
	});
	if (extAnswer.enabled === undefined) {
		process.exit(1);
	}
	config.extensions ??= {};
	for (const name of Object.keys(IMPLEMENTED_EXTENSIONS)) {
		if (extAnswer.enabled.includes(name)) {
			config.extensions[name] = { mode: 'static' };
		} else if ((config.extensions[name]?.mode ?? 'off') !== 'shared') {
			// Extensions that also have a static Dockerfile flag but are
			// currently configured as mode: shared were never a
			// selectable choice here (see the `selected` check above, which
			// only lights up on mode: static) — leave their shared config
			// (source/vendorLib/configArgs) untouched instead of clobbering
			// it to `{ mode: 'off' }`.
			config.extensions[name] = { mode: 'off' };
		}
	}

	const confirmAnswer = await prompts({
		type: 'confirm',
		name: 'save',
		message: 'Save these values to config.yaml?',
		initial: true,
	});
	return Boolean(confirmAnswer.save);
}

function buildArgsForVersion(config, phpVersion) {
	const args = [`--PHP_VERSION=${phpVersion}`];

	for (const [name, flag] of Object.entries(IMPLEMENTED_EXTENSIONS)) {
		const mode = config.extensions?.[name]?.mode ?? 'off';
		args.push(`--${flag}=${mode === 'static' ? 'yes' : 'no'}`);
	}

	const build = config.build ?? {};
	args.push(
		`--WITH_OPENSSL_VERSION=${build.openssl_version ?? getMatrixVersion('libopenssl')}`
	);
	args.push(`--STACK_SIZE=${build.stack_size ?? '1MB'}`);
	args.push(`--WITH_DEBUG=${build.debug ? 'yes' : 'no'}`);
	args.push(`--WITH_SOURCEMAPS=${build.sourcemaps ? 'yes' : 'no'}`);
	// Sourced from their own GitHub repos, not pecl.php.net's package
	// archive (CLAUDE.md decision 34).
	args.push(`--YAML_EXT_VERSION=${getMatrixExtensionVersion('yaml')}`);
	// php-kirigami/php-mdhtml — our own extension, replacing cmark as the
	// core static-mode markdown renderer (CLAUDE.md decision 37).
	args.push(`--MDHTML_EXT_VERSION=${getMatrixExtensionVersion('mdhtml')}`);

	const versionDir = phpVersion.replace(/\./g, '-');
	const outputDir = path.join(
		repoRoot,
		config.output?.dir ?? 'node-builds',
		versionDir
	);
	args.push(`--output-dir=${outputDir}`);

	return args;
}

function runBuild(phpVersion, args) {
	console.log(`\n=== Building PHP ${phpVersion} ===`);
	return new Promise((resolve, reject) => {
		const child = spawn('node', [path.join(sourceDir, 'build.js'), ...args], {
			cwd: sourceDir,
			stdio: 'inherit',
		});
		child.on('close', (code) => {
			if (code === 0) {
				resolve();
			} else {
				reject(new Error(`build.js failed (code ${code}) for PHP ${phpVersion}`));
			}
		});
	});
}

main().catch((error) => {
	console.error(error instanceof Error ? error.message : error);
	process.exit(1);
});
