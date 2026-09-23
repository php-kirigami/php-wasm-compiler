<div align="center">

<img src="https://zmotrin.github.io/assets/kirigami/kirigami-logo-universal.svg" alt="Kirigami" width="400" />

---

# @kirigami/phpext-rar

RAR archive reading (`RarArchive`, `rar://`) for Kirigami PHP.wasm — as a standalone, on-demand WASM side module.  
Built for the **[Kirigami](https://github.com/php-kirigami)** static site generator's PHP runtime, [`@kirigami/php-wasm`](https://github.com/php-kirigami/kirigami).

[![npm version](https://img.shields.io/npm/v/@kirigami/phpext-rar)](https://www.npmjs.com/package/@kirigami/phpext-rar)
[![License: PHP-3.01 AND LicenseRef-UnRAR](https://img.shields.io/badge/license-PHP--3.01%20AND%20LicenseRef--UnRAR-yellow)](./LICENSE)
[![Node.js >=24.0.0](https://img.shields.io/badge/node-%3E%3D24.0.0-brightgreen)](https://nodejs.org)
[![Website](https://img.shields.io/badge/website-php--kirigami.github.io-1f6b4a)](https://php-kirigami.github.io)

</div>

---

## Overview

`@kirigami/phpext-rar` is the PECL rar extension, [`cataphract/php-rar`](https://github.com/cataphract/php-rar) (`cataphract/rar` on Packagist), from its `v4.3.1` tag, compiled independently of the
core `php.wasm` into a **JSPI WASM side module** (Kirigami's `mode: shared`
extension model). Install it and `@kirigami/php-wasm` picks it up
automatically at startup: no manual wiring, no core rebuild.

- ✅ Read-only RAR support, RAR 1.5 through RAR5: listing, extraction, streams, the `rar://` wrapper, encrypted and multi-volume archives
- ✅ Built with the UnRAR source bundled in the extension (no external library)
- ⚠️ Reading only: RAR archives can't be created (UnRAR's license forbids re-creating the compressor)

Built by [`php-wasm-compiler`](https://github.com/php-kirigami/php-wasm-compiler),
which also documents the full build pipeline and architecture decisions.

Part of the **Kirigami** project ecosystem.

---

## Requirements

- Node.js `>= 24.0.0`
- [`@kirigami/php-wasm`](https://github.com/php-kirigami/kirigami) (peer — this package has nothing to load it into on its own)

---

## Installation

```bash
npm install @kirigami/phpext-rar
```

That's it: `@kirigami/php-wasm` scans its own dependencies for
`@kirigami/phpext-*` packages at startup and loads whatever it finds.

---

## Usage

No JS-side API of its own: once installed, the extension is just *there* in
PHP:

```php
<?php
$rar = RarArchive::open('archive.rar');
foreach ($rar->getEntries() as $entry) {
    echo $entry->getName(), "\n";
}
```

---

## Extension details

| | |
| --- | --- |
| PHP extension | `cataphract/php-rar` `v4.3.1` (unmodified) |
| Native library | RARLAB UnRAR (bundled in the extension source) |
| Patches | None |
| PHP version(s) shipped | 8.5 |
| Build mode | `mode: shared` (JSPI side module), see `@kirigami/php-wasm`'s `mode: static` core for the always-on extension set instead |

---

## Contents

- `*.so` — one JSPI side module per PHP version listed above.
- `manifest.json` — extension metadata (name, per-version artifact paths,
  ini/env directives) consumed by `@php-wasm/universal`'s
  `resolvePHPExtension()`.
- `index.js` (+ `index.d.ts`) — default-exports `register(phpVersion)`,
  resolving this package's artifact ready to feed into `@php-wasm/universal`'s
  `withResolvedPHPExtensions()`. This is what `@kirigami/php-wasm`'s
  auto-loader calls — install the package and it's picked up automatically.
- `package.json`'s `kirigami` field — `{ type: "extension", phpVersions,
  minVersion, vendorLib: { name, version }, buildHash }`, mirroring the same
  `kirigami` metadata convention every `@kirigami/plugin-<name>` package
  carries. `minVersion` is the exact PHP patch version this build was
  compiled/tested against (not a `@kirigami/php-wasm` semver).

Compiled by [`php-wasm-compiler`](https://github.com/php-kirigami/php-wasm-compiler)'s
`compile/cli.mjs compile-extension` — see that repo's `CLAUDE.md` for the
full build/versioning story.

---

## License

`PHP-3.01 AND LicenseRef-UnRAR`: php-rar is under the PHP License 3.01, and the bundled UnRAR source under RARLAB's own freeware license (free to use and redistribute; it may not be used to re-create the RAR compression algorithm).

---

## Author

Maxime Larrivée-Roy, 2026
