<div align="center">

<img src="https://zmotrin.github.io/assets/kirigami/kirigami-logo-universal.svg" alt="Kirigami" width="400" />

---

# @kirigami/phpext-scanmeqr

Native QR code encoder (`CrazyGoat\ScanMePHP\NativeEncoderCore`) for Kirigami PHP.wasm — as a standalone, on-demand WASM side module.  
Built for the **[Kirigami](https://github.com/php-kirigami)** static site generator's PHP runtime, [`@kirigami/php-wasm`](https://github.com/php-kirigami/kirigami).

[![npm version](https://img.shields.io/npm/v/@kirigami/phpext-scanmeqr)](https://www.npmjs.com/package/@kirigami/phpext-scanmeqr)
[![License: MIT](https://img.shields.io/badge/license-MIT-yellow)](./LICENSE)
[![Node.js >=24.0.0](https://img.shields.io/badge/node-%3E%3D24.0.0-brightgreen)](https://nodejs.org)
[![Website](https://img.shields.io/badge/website-php--kirigami.github.io-1f6b4a)](https://php-kirigami.github.io)

</div>

---

## Overview

`@kirigami/phpext-scanmeqr` is the `scanmeqr` extension from [`crazy-goat/qrcode-ext`](https://github.com/crazy-goat/qrcode-ext) (the native encoder behind `crazy-goat/scanmephp`), from its `v0.5.2` tag, compiled independently of the
core `php.wasm` into a **JSPI WASM side module** (Kirigami's `mode: shared`
extension model). Install it and `@kirigami/php-wasm` picks it up
automatically at startup: no manual wiring, no core rebuild.

- ✅ The real C++ encoder that `crazy-goat/scanmephp`'s `NativeEncoder` uses
- ✅ Self-contained: the C++20 core is compiled into the module
- ⚠️ Portable mask kernel only (the AVX2/AVX-512 kernels are x86-only)

Built by [`php-wasm-compiler`](https://github.com/php-kirigami/php-wasm-compiler),
which also documents the full build pipeline and architecture decisions.

Part of the **Kirigami** project ecosystem.

---

## Table of contents

- [@kirigami/phpext-scanmeqr](#kirigamiphpext-scanmeqr)
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
npm install @kirigami/phpext-scanmeqr
```

That's it: `@kirigami/php-wasm` scans its own dependencies for
`@kirigami/phpext-*` packages at startup and loads whatever it finds.

---

## Usage

No JS-side API of its own: once installed, the extension is just *there* in
PHP:

```php
<?php
// With crazy-goat/scanmephp installed, its NativeEncoder uses this automatically.
$core = new CrazyGoat\ScanMePHP\NativeEncoderCore();
$raw = $core->encodeRaw('https://example.com', $errorCorrectionLevel);
```

---

## Extension details

| | |
| --- | --- |
| PHP extension | `crazy-goat/qrcode-ext` `v0.5.2` (unmodified) |
| Native library | None (C++20 core bundled) |
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
  minVersion, cxxRuntime: { name, version }, buildHash }` (`cxxRuntime`: the
  C++ runtime linked in, versioned by its emsdk), mirroring the same
  `kirigami` metadata convention every `@kirigami/plugin-<name>` package
  carries. `minVersion` is the exact PHP patch version this build was
  compiled/tested against (not a `@kirigami/php-wasm` semver).

Compiled by [`php-wasm-compiler`](https://github.com/php-kirigami/php-wasm-compiler)'s
`compile/cli.mjs compile-extension` — see that repo's `CLAUDE.md` for the
full build/versioning story.

---

## License

`MIT`, the extension's own license.

---

## Author

Maxime Larrivée-Roy, 2026
