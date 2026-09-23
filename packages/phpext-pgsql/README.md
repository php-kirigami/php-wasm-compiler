<div align="center">

<img src="https://zmotrin.github.io/assets/kirigami/kirigami-logo-universal.svg" alt="Kirigami" width="400" />

---

# @kirigami/phpext-pgsql

`pg_*()` for Kirigami PHP.wasm — as a standalone, on-demand WASM side module.  
Built for the **[Kirigami](https://github.com/php-kirigami)** static site generator's PHP runtime, [`@kirigami/php-wasm`](https://github.com/php-kirigami/kirigami).

[![npm version](https://img.shields.io/npm/v/@kirigami/phpext-pgsql)](https://www.npmjs.com/package/@kirigami/phpext-pgsql)
[![License: GPL-2.0-or-later](https://img.shields.io/badge/license-GPL--2.0--or--later-yellow)](./LICENSE)
[![Node.js >=24.0.0](https://img.shields.io/badge/node-%3E%3D24.0.0-brightgreen)](https://nodejs.org)
[![Website](https://img.shields.io/badge/website-php--kirigami.github.io-1f6b4a)](https://php-kirigami.github.io)

</div>

---

## Overview

`@kirigami/phpext-pgsql` is PHP's own `ext/pgsql` — unmodified upstream
source, straight from the `PHP-8.5.11` tag — compiled independently of the
core `php.wasm` into a **JSPI WASM side module** (Kirigami's `mode: shared`
extension model). Install it and `@kirigami/php-wasm` picks it up
automatically at startup — no manual wiring, no core rebuild.

- ✅ Real `ext/pgsql`: `pg_connect`, `pg_query`, `pg_fetch_assoc`, the full
  documented PHP API — not a reimplementation
- ✅ Linked against a vendored, Emscripten-cross-compiled **libpq 18.6**
  (PostgreSQL's own client library, static build)
- ⚠️ No SSL/TLS support yet in this build (plain connections work fully)
- ✅ JSPI only, Node.js only — matches `@kirigami/php-wasm`'s own target
- ✅ Verified end-to-end: a real connection attempt exercises libpq's actual
  network code path (not just a successful `dlopen`)

Built by [`php-wasm-compiler`](https://github.com/php-kirigami/php-wasm-compiler),
which also documents the full build pipeline and architecture decisions.

Part of the **Kirigami** project ecosystem.

---

## Table of contents

- [@kirigami/phpext-pgsql](#kirigamiphpext-pgsql)
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
npm install @kirigami/phpext-pgsql
```

That's it — `@kirigami/php-wasm` scans its own dependencies for
`@kirigami/phpext-*` packages at startup and loads whatever it finds.

---

## Usage

No JS-side API of its own — once installed, the extension is just *there* in
PHP:

```php
<?php
$conn = pg_connect('host=localhost dbname=mydb user=postgres');
$result = pg_query($conn, 'SELECT 1');
var_dump(pg_fetch_assoc($result));
```

---

## Extension details

| | |
| --- | --- |
| PHP extension | `ext/pgsql`, unmodified, from the `PHP-8.5.11` tag |
| PHP version(s) shipped | 8.5 |
| Native library | [libpq](https://www.postgresql.org/docs/current/libpq.html) `18.6` (static, bundled with `libpgport`/`libpgcommon`) |
| Build mode | `mode: shared` (JSPI side module) — see `@kirigami/php-wasm`'s `mode: static` core for the always-on extension set instead |
| Patches | None — `ext/pgsql`'s own PHP-facing API needed no changes; only the *build configuration* was adapted (a few `libpq` feature-detection checks pre-seeded to work around an unreliable cross-compile probe) |
| Build options | No SSL/TLS support yet (`--without-openssl` on the vendored `libpq`) |

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

`GPL-2.0-or-later` — inherited from PHP itself (php-src's own `ext/pgsql`).

---

## Author

Maxime Larrivée-Roy, 2026
