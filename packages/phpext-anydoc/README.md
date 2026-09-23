<div align="center">

<img src="https://zmotrin.github.io/assets/kirigami/kirigami-logo-universal.svg" alt="Kirigami" width="400" />

---

# @kirigami/phpext-anydoc

Documents to Markdown (Word, PowerPoint, Excel, OpenDocument, RTF, EPUB, CSV, PDF) for Kirigami PHP.wasm — as a standalone, on-demand WASM side module.  
Built for the **[Kirigami](https://github.com/php-kirigami)** static site generator's PHP runtime, [`@kirigami/php-wasm`](https://github.com/php-kirigami/kirigami).

[![npm version](https://img.shields.io/npm/v/@kirigami/phpext-anydoc)](https://www.npmjs.com/package/@kirigami/phpext-anydoc)
[![License: MIT](https://img.shields.io/badge/license-MIT-yellow)](./LICENSE)
[![Node.js >=24.0.0](https://img.shields.io/badge/node-%3E%3D24.0.0-brightgreen)](https://nodejs.org)
[![Website](https://img.shields.io/badge/website-php--kirigami.github.io-1f6b4a)](https://php-kirigami.github.io)

</div>

---

## Overview

`@kirigami/phpext-anydoc` is [`hosmelq/ext-anydoc`](https://github.com/hosmelq/ext-anydoc) `v0.2.4`, the PHP bindings for Firecrawl's anydoc, written in Rust (ext-php-rs), compiled independently of the
core `php.wasm` into a **JSPI WASM side module** (Kirigami's `mode: shared`
extension model). Install it and `@kirigami/php-wasm` picks it up
automatically at startup: no manual wiring, no core rebuild.

- ✅ `anydoc_to_markdown()`, `anydoc_to_markdown_bytes()`, `anydoc_to_document()` and the format helpers, not a reimplementation
- ✅ The Rust code compiled for WebAssembly (the first Rust extension in this ecosystem)
- ⚠️ Single-threaded (the VM has no threads); a Rust panic aborts the PHP runtime (`panic=abort`)

Built by [`php-wasm-compiler`](https://github.com/php-kirigami/php-wasm-compiler),
which also documents the full build pipeline and architecture decisions.

Part of the **Kirigami** project ecosystem.

---

## Table of contents

- [@kirigami/phpext-anydoc](#kirigamiphpext-anydoc)
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
npm install @kirigami/phpext-anydoc
```

That's it: `@kirigami/php-wasm` scans its own dependencies for
`@kirigami/phpext-*` packages at startup and loads whatever it finds.

---

## Usage

No JS-side API of its own: once installed, the extension is just *there* in
PHP:

```php
<?php
$markdown = anydoc_to_markdown('report.docx');
$fromBytes = anydoc_to_markdown_bytes(file_get_contents('slides.pptx'));
```

---

## Extension details

| | |
| --- | --- |
| PHP extension | `hosmelq/ext-anydoc` `v0.2.4` (Rust, built as a static archive and wrapped by a thin phpize build) |
| Native library | None (Rust crate graph: anydoc, zip, quick-xml, lopdf, ...; zstd in C) |
| Patches | Crate type switched to `staticlib`; ext-php-rs's 64-bit-sized `PropertyDescriptor` size guard relaxed for wasm32 |
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

`MIT`, ext-anydoc's own license (anydoc itself is MIT too).

---

## Author

Maxime Larrivée-Roy, 2026
