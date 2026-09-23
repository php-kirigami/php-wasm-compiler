<div align="center">

<img src="https://zmotrin.github.io/assets/kirigami/kirigami-logo-universal.svg" alt="Kirigami" width="400" />

---

# @kirigami/phpext-snmp

SNMP client (`SNMP` class, `snmp*_get()` & co) for Kirigami PHP.wasm — as a standalone, on-demand WASM side module.  
Built for the **[Kirigami](https://github.com/php-kirigami)** static site generator's PHP runtime, [`@kirigami/php-wasm`](https://github.com/php-kirigami/kirigami).

[![npm version](https://img.shields.io/npm/v/@kirigami/phpext-snmp)](https://www.npmjs.com/package/@kirigami/phpext-snmp)
[![License: GPL-2.0-or-later](https://img.shields.io/badge/license-GPL--2.0--or--later-yellow)](./LICENSE)
[![Node.js >=24.0.0](https://img.shields.io/badge/node-%3E%3D24.0.0-brightgreen)](https://nodejs.org)
[![Website](https://img.shields.io/badge/website-php--kirigami.github.io-1f6b4a)](https://php-kirigami.github.io)

</div>

---

## Overview

`@kirigami/phpext-snmp` is PHP's own `ext/snmp`, upstream source straight from the `PHP-8.5.11` tag, compiled independently of the
core `php.wasm` into a **JSPI WASM side module** (Kirigami's `mode: shared`
extension model). Install it and `@kirigami/php-wasm` picks it up
automatically at startup: no manual wiring, no core rebuild.

- ✅ Real `ext/snmp`: SNMP v1, v2c and v3 (with authentication and privacy through OpenSSL)
- ✅ Linked against the vendored, Emscripten-cross-compiled **net-snmp 5.9.5.2** client library
- ⚠️ No MIB files: OIDs must be numeric. IPv4 only. UDP only (ext/snmp can't select another transport), through `@kirigami/php-wasm`'s UDP relay

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
npm install @kirigami/phpext-snmp
```

That's it: `@kirigami/php-wasm` scans its own dependencies for
`@kirigami/phpext-*` packages at startup and loads whatever it finds.

---

## Usage

No JS-side API of its own: once installed, the extension is just *there* in
PHP:

```php
<?php
$snmp = new SNMP(SNMP::VERSION_2c, 'router.example', 'public');
echo $snmp->get('.1.3.6.1.2.1.1.1.0');
```

---

## Extension details

| | |
| --- | --- |
| PHP extension | `ext/snmp` from the `PHP-8.5.11` tag (unmodified) |
| Native library | [net-snmp](https://github.com/net-snmp/net-snmp) `5.9.5.2` (libnetsnmp only), OpenSSL for SNMPv3 |
| Patches | None |
| Build options | No agent, no apps, no MIB files, no IPv6 |
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

`GPL-2.0-or-later`, inherited from PHP itself (php-src's own `ext/snmp`). The statically linked net-snmp is under its own BSD-style licenses, and OpenSSL under Apache-2.0.

---

## Author

Maxime Larrivée-Roy, 2026
