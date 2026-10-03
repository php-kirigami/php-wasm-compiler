<div align="center">

<img src="https://zmotrin.github.io/assets/kirigami/kirigami-logo-universal.svg" alt="Kirigami" width="400" />

---

# @kirigami/phpext-fann

FANN (Fast Artificial Neural Network) bindings for Kirigami PHP.wasm — as a standalone, on-demand WASM side module.  
Built for the **[Kirigami](https://github.com/php-kirigami)** static site generator's PHP runtime, [`@kirigami/php-wasm`](https://github.com/php-kirigami/kirigami).

[![npm version](https://img.shields.io/npm/v/@kirigami/phpext-fann)](https://www.npmjs.com/package/@kirigami/phpext-fann)
[![License: MIT AND LGPL-2.1-or-later](https://img.shields.io/badge/license-MIT%20AND%20LGPL--2.1--or--later-yellow)](./LICENSE)
[![Node.js >=24.0.0](https://img.shields.io/badge/node-%3E%3D24.0.0-brightgreen)](https://nodejs.org)
[![Website](https://img.shields.io/badge/website-php--kirigami.github.io-1f6b4a)](https://php-kirigami.github.io)

</div>

---

## Overview

`@kirigami/phpext-fann` is the `fann` extension from [`coral-media/ext-fann`](https://github.com/coral-media/ext-fann), from its `v1.0.0` tag, compiled independently of the
core `php.wasm` into a **JSPI WASM side module** (Kirigami's `mode: shared`
extension model). Install it and `@kirigami/php-wasm` picks it up
automatically at startup: no manual wiring, no core rebuild.

- ✅ Create, train, run, save and load feed-forward neural networks (standard, sparse and shortcut/cascade topologies)
- ✅ Every training algorithm and activation function of FANN, plus training-data utilities and input/output scaling
- ✅ Self-contained: FANN's own sources are compiled into the module
- ⚠️ The float build only (no fixed-point `fann_run` variant), and training runs on a single thread

Built by [`php-wasm-compiler`](https://github.com/php-kirigami/php-wasm-compiler),
which also documents the full build pipeline and architecture decisions.

Part of the **Kirigami** project ecosystem.

---

## Table of contents

- [@kirigami/phpext-fann](#kirigamiphpext-fann)
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
npm install @kirigami/phpext-fann
```

That's it: `@kirigami/php-wasm` scans its own dependencies for
`@kirigami/phpext-*` packages at startup and loads whatever it finds.

---

## Usage

No JS-side API of its own: once installed, the extension is just *there* in
PHP. The functions mirror the C library's `fann_*` API
([FANN reference](https://github.com/libfann/fann)); the constants
(`FANN_TRAIN_*`, `FANN_SIGMOID_SYMMETRIC`, ...) are defined too.

```php
<?php
$ann = fann_create_standard(3, 2, 3, 1);
fann_set_training_algorithm($ann, FANN_TRAIN_INCREMENTAL);
fann_set_learning_rate($ann, 0.7);
fann_set_activation_function_hidden($ann, FANN_SIGMOID_SYMMETRIC);
fann_set_activation_function_output($ann, FANN_SIGMOID_SYMMETRIC);

for ($epoch = 0; $epoch < 1000; $epoch++) {
    fann_train($ann, [-1.0, -1.0], [-1.0]);
    fann_train($ann, [-1.0, 1.0], [1.0]);
    fann_train($ann, [1.0, -1.0], [1.0]);
    fann_train($ann, [1.0, 1.0], [-1.0]);
}

var_dump(fann_run($ann, [-1.0, 1.0])); // close to [1.0]

fann_save($ann, '/tmp/xor.net');
fann_destroy($ann);
```

Function groups: network creation and persistence, running and training
(including cascade training), training-data I/O and utilities, error and
metric handling, training hyperparameters, activation functions and
steepness, topology and weights, input/output scaling. The upstream
extension lists every function in its
[API reference](https://github.com/coral-media/ext-fann/tree/v1.0.0/docs/api).

---

## Extension details

| | |
| --- | --- |
| PHP extension | `coral-media/ext-fann` `v1.0.0` |
| Native library | FANN (vendored in the extension, compiled in) |
| Patches | `config.m4` drops `-lm` (libm is part of Emscripten's libc) |
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
  minVersion, buildHash }`, mirroring the same
  `kirigami` metadata convention every `@kirigami/plugin-<name>` package
  carries. `minVersion` is the exact PHP patch version this build was
  compiled/tested against (not a `@kirigami/php-wasm` semver).

Compiled by [`php-wasm-compiler`](https://github.com/php-kirigami/php-wasm-compiler)'s
`compile/cli.mjs compile-extension` — see that repo's `CLAUDE.md` for the
full build/versioning story.

---

## License

`MIT` for the extension; the FANN library compiled into it is
`LGPL-2.1-or-later` (its source is in
[`php-wasm-compiler`](https://github.com/php-kirigami/php-wasm-compiler),
under `compile/extensions/fann/lib/libfann`). The package declares
`MIT AND LGPL-2.1-or-later`.

---

## Author

Maxime Larrivée-Roy, 2026
