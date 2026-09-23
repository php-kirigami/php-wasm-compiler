<div align="center">

<img src="https://zmotrin.github.io/assets/kirigami/kirigami-logo-universal.svg" alt="Kirigami" width="400" />

---

# @kirigami/phpext-mysqli

MySQLi for Kirigami PHP.wasm — as two standalone, on-demand WASM side modules bundled into one install.  
Built for the **[Kirigami](https://github.com/php-kirigami)** static site generator's PHP runtime, [`@kirigami/php-wasm`](https://github.com/php-kirigami/kirigami).

[![npm version](https://img.shields.io/npm/v/@kirigami/phpext-mysqli)](https://www.npmjs.com/package/@kirigami/phpext-mysqli)
[![License: GPL-2.0-or-later](https://img.shields.io/badge/license-GPL--2.0--or--later-yellow)](./LICENSE)
[![Node.js >=24.0.0](https://img.shields.io/badge/node-%3E%3D24.0.0-brightgreen)](https://nodejs.org)
[![Website](https://img.shields.io/badge/website-php--kirigami.github.io-1f6b4a)](https://php-kirigami.github.io)

</div>

---

## Overview

`@kirigami/phpext-mysqli` is PHP's own `ext/mysqli` — unmodified upstream
source, straight from the `PHP-8.5.11` tag — compiled independently of the
core `php.wasm` into a **JSPI WASM side module** (Kirigami's `mode: shared`
extension model). Install it and `@kirigami/php-wasm` picks it up
automatically at startup — no manual wiring, no core rebuild.

MySQLi genuinely needs PHP's native MySQL driver, `ext/mysqlnd`, loaded
first — it's a real, separate Zend module, not just a linkable C library
(`mysqli`'s own `config.m4` declares `PHP_ADD_EXTENSION_DEP(mysqli,
mysqlnd)`). Rather than ship two separate npm packages with an implicit
install-order requirement, **this one package bundles both**: `mysqlnd`'s
`.so` ships alongside `mysqli`'s own, and `index.js`'s `register()` resolves
them in the correct order automatically.

- ✅ Real `ext/mysqli`: `mysqli_connect`, the `mysqli` class, the full
  documented PHP API — not a reimplementation
- ✅ `ext/mysqlnd` (PHP's native MySQL driver) bundled and loaded first,
  automatically — no separate install, no manual load-order wiring
- ✅ No external C library for either — both compile straight from
  php-src's own bundled sources (compression support disabled to avoid a
  vendored zlib dependency for this build; SSL support likewise not built)
- ✅ JSPI only, Node.js only — matches `@kirigami/php-wasm`'s own target

Built by [`php-wasm-compiler`](https://github.com/php-kirigami/php-wasm-compiler),
which also documents the full build pipeline and architecture decisions.

Part of the **Kirigami** project ecosystem.

---

## Table of contents

- [@kirigami/phpext-mysqli](#kirigamiphpext-mysqli)
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
npm install @kirigami/phpext-mysqli
```

That's it — `@kirigami/php-wasm` scans its own dependencies for
`@kirigami/phpext-*` packages at startup and loads whatever it finds, in the
right order (`mysqlnd` before `mysqli`, handled internally).

---

## Usage

No JS-side API of its own — once installed, the extension is just *there* in
PHP:

```php
<?php
$mysqli = new mysqli('localhost', 'user', 'pass', 'database');
$result = $mysqli->query('SELECT 1');
```

---

## Extension details

| | |
| --- | --- |
| PHP extensions | `ext/mysqli` + `ext/mysqlnd`, both unmodified, from the `PHP-8.5.11` tag |
| PHP version(s) shipped | 8.5 |
| Native library | None for either — pure php-src bundled code |
| Build mode | `mode: shared` (two JSPI side modules, one package) — see `@kirigami/php-wasm`'s `mode: static` core for the always-on extension set instead |
| Patches | None — both extensions vendored as-is |
| Build options | `mysqlnd` built with `--disable-mysqlnd-compression-support` (avoids needing a vendored zlib for this build); SSL support is off by default in both |

---

## Contents

- `mysqli-*.so` / `mysqlnd-*.so` — one JSPI side module per PHP version
  listed above, for each of the two bundled extensions.
- `manifest.json` (mysqli) / `manifest-mysqlnd.json` (mysqlnd) — extension
  metadata (name, per-version artifact paths, ini/env directives) consumed
  by `@php-wasm/universal`'s `resolvePHPExtension()`.
- `index.js` (+ `index.d.ts`) — default-exports `register(phpVersion)`, resolving **both** artifacts
  in the correct load order (`mysqlnd` first) ready to feed into
  `@php-wasm/universal`'s `withResolvedPHPExtensions()`. This is what
  `@kirigami/php-wasm`'s auto-loader calls — it never needs to know this
  package bundles two modules.
- `package.json`'s `kirigami` field — `{ type: "extension", phpVersions,
  minVersion, bundles: ["mysqlnd"], buildHash }`, mirroring the same
  `kirigami` metadata convention every `@kirigami/plugin-<name>` package
  carries.

Compiled by [`php-wasm-compiler`](https://github.com/php-kirigami/php-wasm-compiler)'s
`compile/cli.mjs compile-extension` — see that repo's `CLAUDE.md`
(decision 42) for the full build/versioning story and the bundled-extension
mechanism.

---

## License

`GPL-2.0-or-later`, the license of php-wasm-compiler's build pipeline, which comes from WordPress Playground (see its [NOTICE.md](https://github.com/php-kirigami/php-wasm-compiler/blob/main/NOTICE.md)). The extension's own source, php-src's `ext/mysqli`/`ext/mysqlnd`, is under the PHP License 3.01.

---

## Author

Maxime Larrivée-Roy, 2026
