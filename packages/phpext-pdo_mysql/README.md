<div align="center">

<img src="https://zmotrin.github.io/assets/kirigami/kirigami-logo-universal.svg" alt="Kirigami" width="400" />

---

# @kirigami/phpext-pdo_mysql

The PDO MySQL driver for Kirigami PHP.wasm — as two standalone, on-demand WASM side modules bundled into one install.  
Built for the **[Kirigami](https://github.com/php-kirigami)** static site generator's PHP runtime, [`@kirigami/php-wasm`](https://github.com/php-kirigami/kirigami).

[![npm version](https://img.shields.io/npm/v/@kirigami/phpext-pdo_mysql)](https://www.npmjs.com/package/@kirigami/phpext-pdo_mysql)
[![License: GPL-2.0-or-later](https://img.shields.io/badge/license-GPL--2.0--or--later-yellow)](./LICENSE)
[![Node.js >=24.0.0](https://img.shields.io/badge/node-%3E%3D24.0.0-brightgreen)](https://nodejs.org)
[![Website](https://img.shields.io/badge/website-php--kirigami.github.io-1f6b4a)](https://php-kirigami.github.io)

</div>

---

## Overview

`@kirigami/phpext-pdo_mysql` is PHP's own `ext/pdo_mysql` — unmodified
upstream source, straight from the `PHP-8.5.11` tag — compiled independently
of the core `php.wasm` into a **JSPI WASM side module** (Kirigami's
`mode: shared` extension model). Install it and `@kirigami/php-wasm` picks
it up automatically at startup — no manual wiring, no core rebuild.

Like `mysqli`, `pdo_mysql` genuinely needs PHP's native MySQL driver,
`ext/mysqlnd`, loaded first — it's a real, separate Zend module, not just a
linkable C library (`pdo_mysql`'s own `config.m4` declares
`PHP_ADD_EXTENSION_DEP(pdo_mysql, mysqlnd)`). Rather than ship two separate
npm packages with an implicit install-order requirement, **this one package
bundles both**: `mysqlnd`'s `.so` ships alongside `pdo_mysql`'s own, and
`index.js`'s `register()` resolves them in the correct order automatically.

- ✅ Real `ext/pdo_mysql`: `new PDO('mysql:...')`, the full documented PDO
  driver surface — not a reimplementation
- ✅ `ext/mysqlnd` (PHP's native MySQL driver) bundled and loaded first,
  automatically — no separate install, no manual load-order wiring. If
  [`@kirigami/phpext-mysqli`](https://www.npmjs.com/package/@kirigami/phpext-mysqli)
  is also installed, both packages' own `mysqlnd` copies are functionally
  identical (built from the same source) — no conflict
- ✅ No external C library — compiles straight from php-src's own bundled
  sources (compression enabled; SSL support not built)
- ✅ JSPI only, Node.js only — matches `@kirigami/php-wasm`'s own target

Built by [`php-wasm-compiler`](https://github.com/php-kirigami/php-wasm-compiler),
which also documents the full build pipeline and architecture decisions.

Part of the **Kirigami** project ecosystem.

---

## Table of contents

- [@kirigami/phpext-pdo_mysql](#kirigamiphpext-pdo_mysql)
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
- PHP's `pdo` extension (already part of `@kirigami/php-wasm`'s always-on core — no separate install)

---

## Installation

```bash
npm install @kirigami/phpext-pdo_mysql
```

That's it — `@kirigami/php-wasm` scans its own dependencies for
`@kirigami/phpext-*` packages at startup and loads whatever it finds, in the
right order (`mysqlnd` before `pdo_mysql`, handled internally).

---

## Usage

No JS-side API of its own — once installed, the extension is just *there* in
PHP:

```php
<?php
$pdo = new PDO('mysql:host=localhost;dbname=mydb', 'user', 'pass');
$stmt = $pdo->query('SELECT 1');
var_dump($stmt->fetch());
```

---

## Extension details

| | |
| --- | --- |
| PHP extensions | `ext/pdo_mysql` + `ext/mysqlnd`, both unmodified, from the `PHP-8.5.11` tag |
| PHP version(s) shipped | 8.5 |
| Native library | None for either — pure php-src bundled code |
| Build mode | `mode: shared` (two JSPI side modules, one package) — see `@kirigami/php-wasm`'s `mode: static` core for the always-on extension set instead |
| Patches | None — both extensions vendored as-is; only the *build configuration* was adapted (`pdo_mysql`'s own `config.m4` needs an explicit `--with-pdo-mysql=mysqlnd` flag, since its default is off) |
| Build options | `mysqlnd` built with compression and extended SSL support both enabled (`--with-mysqlnd-ssl`); `pdo_mysql` itself has no SSL toggle of its own — it rides on `mysqlnd`'s |

---

## Contents

- `pdo_mysql-*.so` / `mysqlnd-*.so` — one JSPI side module per PHP version
  listed above, for each of the two bundled extensions.
- `manifest.json` (pdo_mysql) / `manifest-mysqlnd.json` (mysqlnd) —
  extension metadata (name, per-version artifact paths, ini/env directives)
  consumed by `@php-wasm/universal`'s `resolvePHPExtension()`.
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
(decisions 42 and 47) for the full build/versioning story and the
bundled-extension mechanism.

---

## License

`GPL-2.0-or-later`, the license of php-wasm-compiler's build pipeline, which comes from WordPress Playground (see its [NOTICE.md](https://github.com/php-kirigami/php-wasm-compiler/blob/main/NOTICE.md)). The extension's own source, php-src's `ext/pdo_mysql`/`ext/mysqlnd`, is under the PHP License 3.01.

---

## Author

Maxime Larrivée-Roy, 2026
