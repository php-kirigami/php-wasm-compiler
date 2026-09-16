<div align="center">

<img src="https://zmotrin.github.io/assets/kirigami/kirigami-logo-universal.svg" alt="Kirigami" width="400" />

---

# @kirigami/phpext-ftp

`ftp_*()` for Kirigami PHP.wasm — as a standalone, on-demand WASM side module.  
Built for the **[Kirigami](https://github.com/php-kirigami)** static site generator's PHP runtime, [`@kirigami/php-wasm`](https://github.com/php-kirigami/kirigami).

[![npm version](https://img.shields.io/npm/v/@kirigami/phpext-ftp)](https://www.npmjs.com/package/@kirigami/phpext-ftp)
[![License: GPL-2.0-or-later](https://img.shields.io/badge/license-GPL--2.0--or--later-yellow)](./LICENSE)
[![Node.js >=24.0.0](https://img.shields.io/badge/node-%3E%3D24.0.0-brightgreen)](https://nodejs.org)
[![Website](https://img.shields.io/badge/website-php--kirigami.github.io-1f6b4a)](https://php-kirigami.github.io)

</div>

---

## Overview

`@kirigami/phpext-ftp` is PHP's own `ext/ftp` — unmodified upstream source,
straight from the `PHP-8.5.10` tag — compiled independently of the core
`php.wasm` into a **JSPI WASM side module** (Kirigami's `mode: shared`
extension model). Install it and `@kirigami/php-wasm` picks it up
automatically at startup — no manual wiring, no core rebuild.

- ✅ Real `ext/ftp`: `ftp_connect`, `ftp_login`, `ftp_get`/`ftp_put`, the full
  documented PHP API — not a reimplementation
- ✅ No external C library — compiles straight from php-src's own bundled
  sources (FTP-over-SSL support is not built, since that would need a
  vendored OpenSSL; plain FTP works fully)
- ✅ JSPI only, Node.js only — matches `@kirigami/php-wasm`'s own target

Built by [`php-wasm-compiler`](https://github.com/php-kirigami/php-wasm-compiler),
which also documents the full build pipeline and architecture decisions.

Part of the **Kirigami** project ecosystem.

---

## Table of contents

- [@kirigami/phpext-ftp](#kirigamiphpext-ftp)
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
npm install @kirigami/phpext-ftp
```

That's it — `@kirigami/php-wasm` scans its own dependencies for
`@kirigami/phpext-*` packages at startup and loads whatever it finds.

---

## Usage

No JS-side API of its own — once installed, the extension is just *there* in
PHP:

```php
<?php
$conn = ftp_connect('ftp.example.com');
ftp_login($conn, 'user', 'pass');
ftp_get($conn, 'local.txt', 'remote.txt', FTP_ASCII);
ftp_close($conn);
```

---

## Extension details

| | |
| --- | --- |
| PHP extension | `ext/ftp`, unmodified, from the `PHP-8.5.10` tag |
| PHP version(s) shipped | 8.5 |
| Native library | None — pure php-src bundled code |
| Build mode | `mode: shared` (JSPI side module) — see `@kirigami/php-wasm`'s `mode: static` core for the always-on extension set instead |
| Patches | None |

---

## Contents

- `*.so` — one JSPI side module per PHP version listed above.
- `manifest.json` — extension metadata (name, per-version artifact paths,
  ini/env directives) consumed by `@php-wasm/universal`'s
  `resolvePHPExtension()` / `{ format: 'manifest' }` loader.
- `index.js` (+ `index.d.ts`) — default-exports `register(phpVersion)`, resolving this package's
  artifact(s) ready to feed into `@php-wasm/universal`'s
  `withResolvedPHPExtensions()`. This is what `@kirigami/php-wasm`'s
  auto-loader calls.
- `package.json`'s `kirigami` field — `{ type: "extension", phpVersions,
  minVersion, buildHash }`, mirroring the same `kirigami` metadata
  convention every `@kirigami/plugin-<name>` package carries.

Compiled by [`php-wasm-compiler`](https://github.com/php-kirigami/php-wasm-compiler)'s
`compile/cli.mjs compile-extension` — see that repo's `CLAUDE.md` for the
full build/versioning story.

---

## License

`GPL-2.0-or-later` — inherited from PHP itself (php-src's own `ext/ftp`).

---

## Author

Maxime Larrivée-Roy, 2026
