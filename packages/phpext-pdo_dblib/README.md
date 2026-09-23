<div align="center">

<img src="https://zmotrin.github.io/assets/kirigami/kirigami-logo-universal.svg" alt="Kirigami" width="400" />

---

# @kirigami/phpext-pdo_dblib

PDO `dblib:` driver (Microsoft SQL Server / Sybase) for Kirigami PHP.wasm — as a standalone, on-demand WASM side module.  
Built for the **[Kirigami](https://github.com/php-kirigami)** static site generator's PHP runtime, [`@kirigami/php-wasm`](https://github.com/php-kirigami/kirigami).

[![npm version](https://img.shields.io/npm/v/@kirigami/phpext-pdo_dblib)](https://www.npmjs.com/package/@kirigami/phpext-pdo_dblib)
[![License: GPL-2.0-or-later](https://img.shields.io/badge/license-GPL--2.0--or--later-yellow)](./LICENSE)
[![Node.js >=24.0.0](https://img.shields.io/badge/node-%3E%3D24.0.0-brightgreen)](https://nodejs.org)
[![Website](https://img.shields.io/badge/website-php--kirigami.github.io-1f6b4a)](https://php-kirigami.github.io)

</div>

---

## Overview

`@kirigami/phpext-pdo_dblib` is PHP's own `ext/pdo_dblib` — upstream C
source, straight from the `PHP-8.5.11` tag — compiled independently of the
core `php.wasm` into a **JSPI WASM side module** (Kirigami's `mode: shared`
extension model). Install it and `@kirigami/php-wasm` picks it up
automatically at startup — no manual wiring, no core rebuild.

- ✅ Real `ext/pdo_dblib`: the `dblib` PDO driver (`new PDO('dblib:...')`) — not a
  reimplementation
- ✅ Linked against the vendored, Emscripten-cross-compiled **FreeTDS 1.5.19**
  db-lib (`libsybdb`), speaking TDS to Microsoft SQL Server and Sybase ASE
- ⚠️ No TLS yet, so no encrypted connections (Azure SQL requires them); no
  Kerberos. Not yet tested against a real SQL Server
- ✅ JSPI only, Node.js only — matches `@kirigami/php-wasm`'s own target

Built by [`php-wasm-compiler`](https://github.com/php-kirigami/php-wasm-compiler),
which also documents the full build pipeline and architecture decisions.

Part of the **Kirigami** project ecosystem.

---

## Table of contents

- [@kirigami/phpext-pdo_dblib](#kirigamiphpext-pdo_dblib)
  - [Overview](#overview)
  - [Table of contents](#table-of-contents)
  - [Requirements](#requirements)
  - [Installation](#installation)
  - [Usage](#usage)
  - [Extension details](#extension-details)
  - [Contents](#contents)
  - [License](#license)
  - [Author](#author)

---

## Requirements

- Node.js `>= 24.0.0`
- [`@kirigami/php-wasm`](https://github.com/php-kirigami/kirigami) (peer — this package has nothing to load it into on its own)

---

## Installation

```bash
npm install @kirigami/phpext-pdo_dblib
```

That's it — `@kirigami/php-wasm` scans its own dependencies for
`@kirigami/phpext-*` packages at startup and loads whatever it finds.

---

## Usage

No JS-side API of its own — once installed, the extension is just *there* in
PHP:

```php
<?php
$pdo = new PDO('dblib:host=sql.example.com:1433;dbname=mydb', 'user', 'password');
$rows = $pdo->query('SELECT @@VERSION')->fetchAll();
```

---

## Extension details

| | |
| --- | --- |
| PHP extension | `ext/pdo_dblib` from the `PHP-8.5.11` tag (C sources unmodified, one-line `config.m4` patch) |
| PHP version(s) shipped | 8.5 |
| Native library | [FreeTDS](https://www.freetds.org) `1.5.19` (db-lib only) |
| Build mode | `mode: shared` (JSPI side module) — see `@kirigami/php-wasm`'s `mode: static` core for the always-on extension set instead |
| Patches | One line in `config.m4` (a libtool-incompatible `PHP_ADD_LIBRARY_WITH_PATH` call removed, see `compile/extensions/pdo_dblib/PROVENANCE.md`); C sources unmodified |
| Build options | No TLS, no Kerberos, no threads (and so no MARS) in the vendored FreeTDS build |

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

`GPL-2.0-or-later`, the license of php-wasm-compiler's build pipeline, which comes from WordPress Playground (see its [NOTICE.md](https://github.com/php-kirigami/php-wasm-compiler/blob/main/NOTICE.md)). The extension's own source, php-src's `ext/pdo_dblib`, is under the PHP License 3.01.

---

## Author

Maxime Larrivée-Roy, 2026
