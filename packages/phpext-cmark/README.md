<div align="center">

<img src="https://zmotrin.github.io/assets/kirigami/kirigami-logo-universal.svg" alt="Kirigami" width="400" />

---

# @kirigami/phpext-cmark

Full CommonMark with a real, traversable node tree for Kirigami PHP.wasm — as a standalone, on-demand WASM side module.  
Built for the **[Kirigami](https://github.com/php-kirigami)** static site generator's PHP runtime, [`@kirigami/php-wasm`](https://github.com/php-kirigami/kirigami).

[![npm version](https://img.shields.io/npm/v/@kirigami/phpext-cmark)](https://www.npmjs.com/package/@kirigami/phpext-cmark)
[![License: GPL-2.0-or-later](https://img.shields.io/badge/license-GPL--2.0--or--later-yellow)](./LICENSE)
[![Node.js >=24.0.0](https://img.shields.io/badge/node-%3E%3D24.0.0-brightgreen)](https://nodejs.org)
[![Website](https://img.shields.io/badge/website-php--kirigami.github.io-1f6b4a)](https://php-kirigami.github.io)

</div>

---

## Overview

`@kirigami/phpext-cmark` is [`krakjoe/cmark`](https://github.com/krakjoe/cmark),
compiled independently of the core `php.wasm` into a **JSPI WASM side
module** (Kirigami's `mode: shared` extension model). Install it and
`@kirigami/php-wasm` picks it up automatically at startup — no manual wiring,
no core rebuild.

Kirigami's core build already ships its own, lighter `mdhtml` extension
(CommonMark+GFM → HTML string, via `cmark-gfm`, no object model). Reach for
`@kirigami/phpext-cmark` instead when you need the actual parsed
**document tree**: `instanceof Traversable`, `foreach` over nodes, per-node
inspection/visitor logic — not just an HTML string out.

- ✅ Real `CommonMark\Node\*` object tree — `Document`, `Heading`,
  `Paragraph`, `Text`, `Link`, `CodeBlock`, etc. — each `instanceof
  Traversable`, walkable with a plain `foreach`
- ✅ `\CommonMark\Parse($markdown)` — the extension's top-level entry point
- ✅ Five real PHP 7→8 Zend-API bugs found and patched in unmodified upstream
  `krakjoe/cmark` v1.2.0 (last touched by upstream in 2019, pre-PHP 8) — see
  [Extension details](#extension-details)
- ✅ JSPI only, Node.js only — matches `@kirigami/php-wasm`'s own target
- ✅ Verified end-to-end against `@kirigami/php-wasm`'s patched `php.wasm`:
  parse → tree walk → object destruction at request shutdown, no crash

Built by [`php-wasm-compiler`](https://github.com/php-kirigami/php-wasm-compiler),
which also documents the full build pipeline and architecture decisions.

Part of the **Kirigami** project ecosystem.

---

## Table of contents

- [@kirigami/phpext-cmark](#kirigamiphpext-cmark)
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
npm install @kirigami/phpext-cmark
```

That's it — `@kirigami/php-wasm` scans its own dependencies for
`@kirigami/phpext-*` packages at startup and loads whatever it finds.

---

## Usage

```php
<?php
$doc = \CommonMark\Parse("# Hello\n\nWorld");

var_dump($doc instanceof Traversable); // true

foreach ($doc as $node) {
    echo get_class($node), "\n";
}
// CommonMark\Node\Heading
// CommonMark\Node\Text
// CommonMark\Node\Paragraph
// CommonMark\Node\Text
```

---

## Extension details

| | |
| --- | --- |
| PHP extension | [`krakjoe/cmark`](https://github.com/krakjoe/cmark) `v1.2.0` |
| PHP version(s) shipped | 8.5 |
| Native library | [commonmark/cmark](https://github.com/commonmark/cmark) `0.31.2` |
| Build mode | `mode: shared` (JSPI side module) |
| Patches (`patches/cmark/`) | 5 — all fixing real, independently-diagnosed PHP 7→8 (and PHP-version-drift) incompatibilities in unmodified upstream code: (1) `Node`'s own Traversable/abstract-class registration order, (2) Zend object-handler signatures (`zval*` → `zend_object*`), (3) the same Traversable/abstract ordering bug on all 21 concrete subclass registrations, (4) object-properties-table initialization, (5) a missing `extra_flags` field in the extension's hand-rolled `zend_object` mimic struct — the actual root cause of a request-shutdown crash |

---

## Contents

- `*.so` — one JSPI side module per PHP version listed above.
- `manifest.json` — extension metadata (name, per-version artifact paths,
  ini/env directives) consumed by `@php-wasm/universal`'s
  `resolvePHPExtension()` / `{ format: 'manifest' }` loader — this is what
  `@kirigami/php-wasm`'s auto-loader reads.
- `package.json`'s `kirigami` field — `{ type: "extension", phpVersions,
  minVersion, vendorLib: { name, version }, buildHash }`, mirroring the same
  `kirigami` metadata convention every `@kirigami/plugin-<name>` package
  carries. `minVersion` is the exact PHP patch version this build was
  compiled/tested against (not a `@kirigami/php-wasm` semver).

Compiled by [`php-wasm-compiler`](https://github.com/php-kirigami/php-wasm-compiler)'s
`compile/cli.mjs compile-extension` — see that repo's `CLAUDE.md`
(decisions 34-35) for the full five-patch cmark investigation and the
build/versioning story.

---

## License

`GPL-2.0-or-later`.

---

## Author

Maxime Larrivée-Roy, 2026
