<div align="center">

<img src="https://zmotrin.github.io/assets/kirigami/kirigami-logo-universal.svg" alt="Kirigami" width="400" />

---

# @kirigami/phpext-fastchart

Chart rendering (38 chart families, QR codes, barcodes) to SVG/PNG/JPEG/WebP for Kirigami PHP.wasm — as a standalone, on-demand WASM side module.  
Built for the **[Kirigami](https://github.com/php-kirigami)** static site generator's PHP runtime, [`@kirigami/php-wasm`](https://github.com/php-kirigami/kirigami).

[![npm version](https://img.shields.io/npm/v/@kirigami/phpext-fastchart)](https://www.npmjs.com/package/@kirigami/phpext-fastchart)
[![License: BSD-3-Clause AND MIT](https://img.shields.io/badge/license-BSD--3--Clause%20AND%20MIT-yellow)](./LICENSE)
[![Node.js >=24.0.0](https://img.shields.io/badge/node-%3E%3D24.0.0-brightgreen)](https://nodejs.org)
[![Website](https://img.shields.io/badge/website-php--kirigami.github.io-1f6b4a)](https://php-kirigami.github.io)

</div>

---

## Overview

`@kirigami/phpext-fastchart` is the `fastchart` extension, [`iliaal/fastchart`](https://github.com/iliaal/fastchart), from its `1.7.4` tag, compiled independently of the
core `php.wasm` into a **JSPI WASM side module** (Kirigami's `mode: shared`
extension model). Install it and `@kirigami/php-wasm` picks it up
automatically at startup: no manual wiring, no core rebuild.

- ✅ The real fastchart: line, bar, pie, stock, heatmap, sankey, treemap and 30 more chart families, plus QR codes and Code128
- ✅ SVG, PNG, JPEG and WebP output through the vendored freetype, libpng, libjpeg-turbo and libwebp
- ✅ A default font built in (DejaVu Sans), so charts render text with no system fonts; `setFontPath()` still overrides it
- ⚠️ No PDF output (`renderPdf()` needs pdfio, not vendored)

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
npm install @kirigami/phpext-fastchart
```

That's it: `@kirigami/php-wasm` scans its own dependencies for
`@kirigami/phpext-*` packages at startup and loads whatever it finds.

---

## Usage

No JS-side API of its own: once installed, the extension is just *there* in
PHP:

```php
<?php
$chart = (new FastChartLineChart())->setSize(600, 300)->setTitle('Sales')->setSeries([3, 1, 4, 1, 5, 9]);
file_put_contents('chart.png', $chart->renderPng());
```

---

## Extension details

| | |
| --- | --- |
| PHP extension | `iliaal/fastchart` `1.7.4` (one line added to `config.m4`: the embedded default font) |
| Native library | plutovg, plutosvg, qrcodegen (bundled); freetype, libpng, libjpeg-turbo, libwebp, zlib |
| Patches | `config.m4` lists one extra source file (`wasm_default_font.c`) |
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

`BSD-3-Clause AND MIT`, fastchart's own licenses (with its bundled plutovg/plutosvg/qrcodegen). The embedded DejaVu Sans font is under the Bitstream Vera license, with public-domain DejaVu changes.

---

## Author

Maxime Larrivée-Roy, 2026
