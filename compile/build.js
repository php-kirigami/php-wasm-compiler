import path from 'path';
import util from 'util';
import fs from 'fs';
import { fileURLToPath } from 'url';
const rmAsync = util.promisify(fs.rm);
import { spawn } from 'child_process';
import { phpVersions, lastRefreshed } from '../supported-php-versions.mjs';

// Refresh PHP versions if they need updating (if last refreshed more than 24 hours ago)
const twentyFourHoursAgo = new Date(Date.now() - 24 * 60 * 60 * 1000);
const lastRefreshedDate = new Date(lastRefreshed);

if (lastRefreshedDate < twentyFourHoursAgo) {
	console.log(
		'📅 PHP versions data is older than 24 hours, checking for updates...'
	);
	try {
		const { updatePHPVersions } = await import('./update-php-versions.mjs');
		await updatePHPVersions();

		// Reload the supported-php-versions.mjs module to get the updated versions
		const { phpVersions: updatedPhpVersions } = await import(
			'../supported-php-versions.mjs?' + Date.now()
		);
		// Replace the original phpVersions with the updated ones
		phpVersions.length = 0;
		phpVersions.push(...updatedPhpVersions);
	} catch (error) {
		console.warn('⚠️  Failed to update PHP versions:', error.message);
		process.exit(1);
	}
}

// yargs parse
import yargs from 'yargs';
const argParser = yargs(process.argv.slice(2))
	.usage('Usage: $0 [options]')
	.options({
		JSPI: {
			type: 'boolean',
			default: false,
			description: 'Build with JSPI support',
		},
		DEBUG: {
			type: 'boolean',
			default: false,
			description: 'Build with debug symbols',
		},
		WITH_FILEINFO: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with fileinfo support',
		},
		WITH_LIBXML: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with libxml support',
		},
		WITH_SOAP: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with SOAP support',
		},
		WITH_LIBZIP: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with libzip support',
		},
		WITH_EXIF: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with exif support',
		},
		WITH_GD: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with libpng support',
		},
		WITH_ICONV: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with iconv support',
		},
		WITH_MBSTRING: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with mbstring support',
		},
		WITH_MBREGEX: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with mbregex support',
		},
		WITH_CLI_SAPI: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with CLI SAPI',
		},
		WITH_OPENSSL: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with OpenSSL support',
		},
		WITH_NODEFS: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with Node.js FS support',
		},
		WITH_CURL: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with cURL support',
		},
		WITH_SQLITE: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with SQLite support',
		},
		WITH_SOURCEMAPS: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with source maps',
		},
		WITH_DEBUG: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with DWARF debug information.',
		},
		WITH_ICONV: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with source maps',
		},
		WITH_MYSQL: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with MySQL support',
		},
		WITH_WS_NETWORKING_PROXY: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with WebSocket networking proxy support',
		},
		WITH_IMAGICK: {
			type: 'string',
			choices: ['yes', 'no'],
			description: 'Build with imagick support',
		},
		PHP_VERSION: {
			type: 'string',
			description: 'The PHP version to build',
			required: true,
		},
		PHP_REF: {
			type: 'string',
			description:
				'The php-src git ref to clone. Defaults to the php-$PHP_VERSION tag.',
		},
		['output-dir']: {
			type: 'string',
			description:
				'The output directory. If not provided, it will be computed from the PHP version and platform.',
		},
		WITH_OPENSSL_VERSION: {
			type: 'string',
			description: 'OpenSSL version to use (must match compile/Makefile\'s OPENSSL_VERSION)',
			default: '3.6.4',
		},
		WITH_OPCACHE: {
			type: 'string',
			description: 'Build with OPCache support',
		},
		STACK_SIZE: {
			type: 'string',
			description: 'The emscripten stack size to use for the build',
			default: '1MB',
		},
	});

const args = argParser.argv;

const platformDefaults = {
	all: {
		PHP_VERSION: '8.0.24',
		WITH_CLI_SAPI: 'yes',
		WITH_LIBZIP: 'yes',
		WITH_SQLITE: 'yes',
		WITH_JSPI: 'yes',
		WITH_CURL: 'yes',
		WITH_FILEINFO: 'yes',
		WITH_ICONV: 'yes',
		WITH_LIBXML: 'yes',
		WITH_SOAP: 'yes',
		WITH_EXIF: 'yes',
		WITH_GD: 'yes',
		WITH_MBSTRING: 'yes',
		WITH_MBREGEX: 'yes',
		WITH_OPENSSL: 'yes',
		WITH_WS_NETWORKING_PROXY: 'yes',
		WITH_OPCACHE: 'yes',
		WITH_IMAGICK: 'no',
		STACK_SIZE: '1MB',
	},
	node: {
		WITH_NODEFS: 'yes',
		WITH_MYSQL: 'yes',
		WITH_IMAGICK: 'yes',
	},
};
// This project only ships a Node.js runtime for Kirigami (no browser target).
const platform = 'node';

/* eslint-disable prettier/prettier */
const getArg = (name) => {
	let value =
		name in args
			? args[name]
			: name in platformDefaults[platform]
				? platformDefaults[platform][name]
				: name in platformDefaults.all
					? platformDefaults.all[name]
					: 'no';
	if (name === 'PHP_VERSION') {
		value = fullyQualifiedPHPVersion(value);
	}
	return `${name}=${value}`;
};

const requestedVersion = getArg('PHP_VERSION');
if (!requestedVersion || requestedVersion === 'undefined') {
	process.stdout.write(`PHP version ${requestedVersion} is not supported\n`);
	process.stdout.write(await argParser.getHelp());
	process.exit(1);
}

// fileURLToPath (not raw `new URL(...).pathname`) is required on Windows:
// the raw pathname keeps a leading "/" before the drive letter
// (e.g. "/C:/…"), which silently breaks `cwd` for spawn() below.
const sourceDir = path.dirname(fileURLToPath(import.meta.url));

// Compute output directory if not provided
function computeOutputDir() {
	if (args.outputDir) {
		return path.resolve(process.cwd(), args.outputDir);
	}
	// Extract major.minor from the PHP version (e.g., "8.4.16" -> "8-4")
	const phpVersion = args.PHP_VERSION || '8.3';
	const [major, minor] = phpVersion.split('.');
	const versionDir = `${major}-${minor}`;
	// Check both --JSPI (boolean) and --WITH_JSPI=yes (string from legacy format)
	const isJspi = args.JSPI || args.WITH_JSPI === 'yes';
	const jspiOrAsyncify = isJspi ? 'jspi' : 'asyncify';
	return path.resolve(
		process.cwd(),
		`node-builds/${versionDir}/${jspiOrAsyncify}`
	);
}

const outputDir = computeOutputDir();

// Clean up outdated minor versions in the output directory to avoid shipping
// multiple binaries for the same major.minor PHP version.
async function cleanupOldMinorVersions() {
	if (!fs.existsSync(outputDir)) {
		return;
	}
	const phpVersion = args.PHP_VERSION || '8.3';
	const [major, minor] = phpVersion.split('.');
	const versionPrefix = `${major}_${minor}`;

	const entries = fs.readdirSync(outputDir);
	for (const entry of entries) {
		// Match files and directories like "8_4_15", "php_8_4.js", etc.
		// that belong to the same major.minor version
		if (
			entry.startsWith(versionPrefix) ||
			entry.startsWith(`php_${major}_${minor}`)
		) {
			const fullPath = path.join(outputDir, entry);
			console.log(`Removing outdated: ${fullPath}`);
			await rmAsync(fullPath, { recursive: true, force: true });
		}
	}
}

await cleanupOldMinorVersions();

// compile/php/Dockerfile unconditionally COPYs both of these (to scan for
// already-built loadable extensions under node-builds/ — see CLAUDE.md
// decision 5 — and to pull in Playground's optional shared-extension
// metadata under compile/shared/, which this repo doesn't vendor). Docker's
// COPY fails outright if the source directory doesn't exist at all, so make
// sure both are present (possibly empty) before every build — node-builds/
// is gitignored and won't exist on a fresh checkout or CI runner.
fs.mkdirSync(path.resolve(sourceDir, '..', 'node-builds'), { recursive: true });
fs.mkdirSync(path.resolve(sourceDir, 'shared'), { recursive: true });

// Build the base image
await asyncSpawn('make', ['base-image'], { cwd: sourceDir, stdio: 'inherit' });

const phpVersionForDockerfile = getArg('PHP_VERSION').replace(
	'PHP_VERSION=',
	''
);
const phpRef = args.PHP_REF || `php-${phpVersionForDockerfile}`;
const dockerfile = 'php/Dockerfile';

await asyncSpawn(
	'docker',
	[
		'build',
		'-f',
		dockerfile,
		'..',
		'--tag=kirigami-php-wasm',
		'--progress=plain',
		'--build-arg',
		getArg('PHP_VERSION'),
		'--build-arg',
		`PHP_REF=${phpRef}`,
		'--build-arg',
		`OPENSSL_VERSION=${args.WITH_OPENSSL_VERSION || '3.6.4'}`,
		'--build-arg',
		getArg('WITH_FILEINFO'),
		'--build-arg',
		getArg('WITH_LIBXML'),
		'--build-arg',
		getArg('WITH_SOAP'),
		'--build-arg',
		getArg('WITH_LIBZIP'),
		'--build-arg',
		getArg('WITH_EXIF'),
		'--build-arg',
		getArg('WITH_GD'),
		'--build-arg',
		getArg('WITH_MBSTRING'),
		'--build-arg',
		getArg('WITH_MBREGEX'),
		'--build-arg',
		getArg('WITH_SOCKETS'),
		'--build-arg',
		getArg('WITH_CLI_SAPI'),
		'--build-arg',
		getArg('WITH_OPENSSL'),
		'--build-arg',
		getArg('WITH_NODEFS'),
		'--build-arg',
		getArg('WITH_CURL'),
		'--build-arg',
		getArg('WITH_SQLITE'),
		'--build-arg',
		getArg('WITH_SOURCEMAPS'),
		'--build-arg',
		// Relay output directory so we can create source maps and DWARF debug
		// info containing correct paths.
		`OUTPUT_DIR_ON_HOST=${outputDir}`,
		'--build-arg',
		getArg('WITH_DEBUG'),
		// This directory path allows us to set what the DWARF file references
		// are relative to so step debugging source files works correctly.
		'--build-arg',
		`DEBUG_DWARF_COMPILATION_DIR=${path.resolve(
			import.meta.dirname,
			'..'
		)}`,
		'--build-arg',
		getArg('WITH_ICONV'),
		'--build-arg',
		getArg('WITH_MYSQL'),
		'--build-arg',
		getArg('WITH_WS_NETWORKING_PROXY'),
		'--build-arg',
		getArg('WITH_IMAGICK'),
		'--build-arg',
		getArg('WITH_YAML'),
		'--build-arg',
		getArg('WITH_MDHTML'),
		'--build-arg',
		getArg('WITH_JSONK'),
		'--build-arg',
		getArg('WITH_APCU'),
		'--build-arg',
		getArg('WITH_NAVICAT'),
		'--build-arg',
		getArg('WITH_IGBINARY'),
		'--build-arg',
		getArg('WITH_NORM'),
		'--build-arg',
		// Sourced from their own GitHub repos, not pecl.php.net (CLAUDE.md
		// decision 34). Same "hardcoded last-resort fallback for standalone
		// build.js use" shape as OPENSSL_VERSION above -- cli.mjs always
		// passes these explicitly via matrix.json.
		`YAML_EXT_VERSION=${args.YAML_EXT_VERSION || '2.3.0'}`,
		'--build-arg',
		`MDHTML_EXT_VERSION=${args.MDHTML_EXT_VERSION || 'v0.1.0'}`,
		'--build-arg',
		`JSONK_EXT_VERSION=${args.JSONK_EXT_VERSION || 'v0.1.0'}`,
		'--build-arg',
		`SIMDJSON_VERSION=${args.SIMDJSON_VERSION || '4.6.11'}`,
		'--build-arg',
		`YYJSON_VERSION=${args.YYJSON_VERSION || '0.13.0'}`,
		'--build-arg',
		`APCU_EXT_VERSION=${args.APCU_EXT_VERSION || 'v5.1.28'}`,
		'--build-arg',
		`NAVICAT_EXT_VERSION=${args.NAVICAT_EXT_VERSION || 'v0.1.0'}`,
		'--build-arg',
		`IGBINARY_EXT_VERSION=${args.IGBINARY_EXT_VERSION || '3.2.15'}`,
		'--build-arg',
		`NORM_EXT_VERSION=${args.NORM_EXT_VERSION || 'v0.1.0'}`,
		'--build-arg',
		`UTF8PROC_VERSION=${args.UTF8PROC_VERSION || '2.11.3'}`,
		'--build-arg',
		`IMAGICK_EXT_VERSION=${args.IMAGICK_EXT_VERSION || '3.8.1'}`,
		'--build-arg',
		`EMSCRIPTEN_ENVIRONMENT=${platform}`,
		'--build-arg',
		getArg('WITH_JSPI'),
		'--build-arg',
		getArg('WITH_OPCACHE'),
		'--build-arg',
		getArg('STACK_SIZE'),
	],
	{ cwd: sourceDir, stdio: 'inherit' }
);
/* eslint-enable prettier/prettier */

const copyTerminfoCommand =
	getArg('WITH_CLI_SAPI') === 'yes'
		? ' && cp /root/lib/share/terminfo/x/xterm /output/terminfo/x'
		: '';
const restoreOutputOwnershipCommand = getRestoreOutputOwnershipCommand();

// Extract the PHP WASM module
await asyncSpawn(
	'docker',
	[
		'run',
		'--name',
		'kirigami-php-wasm-tmp',
		'--rm',
		'-v',
		`${outputDir}:/output`,
		'kirigami-php-wasm',
		// Use sh -c because wildcards are a shell feature and
		// they don't work without running cp through shell.
		'sh',
		'-c',
		`cp -rf /root/output/* /output && ` +
			`mkdir -p /output/terminfo/x` +
			copyTerminfoCommand +
			restoreOutputOwnershipCommand,
	],
	{ cwd: sourceDir, stdio: 'inherit' }
);

/**
 * build.js copies artifacts from a root Docker container. On Linux, those
 * bind-mounted files become root-owned on the host. Restore them to the host
 * owner so later Node steps and local cleanups can edit generated artifacts.
 */
function getRestoreOutputOwnershipCommand() {
	if (
		typeof process.getuid !== 'function' ||
		typeof process.getgid !== 'function'
	) {
		return '';
	}
	const uid = process.getuid();
	const gid = process.getgid();
	if (uid === 0) {
		return '';
	}
	return ` && chown -R ${uid}:${gid} /output`;
}

function asyncSpawn(...args) {
	console.log('Running', args[0], args[1].join(' '), '...');
	return new Promise((resolve, reject) => {
		const child = spawn(...args);
		child.on('close', (code) => {
			if (code === 0) resolve(code);
			else reject(new Error(`Process exited with code ${code}`));
		});
	});
}

function fullyQualifiedPHPVersion(requestedVersion) {
	for (const { version, lastRelease } of phpVersions) {
		if (requestedVersion === version) {
			return lastRelease;
		}
	}
	return requestedVersion;
}
