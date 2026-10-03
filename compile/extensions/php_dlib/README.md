<div align="center">

<img src="https://zmotrin.github.io/assets/kirigami/kirigami-logo-universal.svg" alt="Kirigami" width="400" />

---

# @kirigami/phpext-php_dlib

Face detection, landmarks, recognition and clustering (the `php_dlib` extension, over [dlib](http://dlib.net)) for Kirigami PHP.wasm — as a standalone, on-demand WASM side module.  
Built for the **[Kirigami](https://github.com/php-kirigami)** static site generator's PHP runtime, [`@kirigami/php-wasm`](https://github.com/php-kirigami/kirigami).

[![npm version](https://img.shields.io/npm/v/@kirigami/phpext-php_dlib)](https://www.npmjs.com/package/@kirigami/phpext-php_dlib)
[![License: MIT](https://img.shields.io/badge/license-MIT-yellow)](./LICENSE)
[![Node.js >=24.0.0](https://img.shields.io/badge/node-%3E%3D24.0.0-brightgreen)](https://nodejs.org)
[![Website](https://img.shields.io/badge/website-php--kirigami.github.io-1f6b4a)](https://php-kirigami.github.io)

</div>

---

## Overview

`@kirigami/phpext-php_dlib` is the `php_dlib` extension from [`mailmug/php-dlib`](https://github.com/mailmug/php-dlib), from its `v2.0.1` tag, compiled independently of the
core `php.wasm` into a **JSPI WASM side module** (Kirigami's `mode: shared`
extension model). Install it and `@kirigami/php-wasm` picks it up
automatically at startup: no manual wiring, no core rebuild.

- ✅ HOG frontal face detector, clustering (`dlib_chinese_whispers`), vector helpers
- ✅ JPEG and PNG loading, from dlib's own bundled decoders
- ✅ dlib 20.0.1 and the C++ runtime are linked into the module
- ⚠️ No BLAS/LAPACK and no threads: plain C++ matrix code, so the neural models (`CnnFaceDetection`, `FaceRecognition`) are slow
- ⚠️ The model files (`.dat`) are not shipped: pass the paths of your own

Built by [`php-wasm-compiler`](https://github.com/php-kirigami/php-wasm-compiler),
which also documents the full build pipeline and architecture decisions.

Part of the **Kirigami** project ecosystem.

---

## Table of contents

- [@kirigami/phpext-php_dlib](#kirigamiphpext-php_dlib)
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
npm install @kirigami/phpext-php_dlib
```

That's it: `@kirigami/php-wasm` scans its own dependencies for
`@kirigami/phpext-*` packages at startup and loads whatever it finds.

---

## Usage

No JS-side API of its own: once installed, the extension is just *there* in
PHP. Images are read from the PHP filesystem, as JPEG or PNG.

### Models

Only the HOG frontal face detector is built into dlib. The other features
load a trained model from a path you pass in (dlib's `.dat` files, from
[dlib.net/files](http://dlib.net/files/), distributed as `.bz2`: decompress
them first). None is shipped with the package.

| Used by | Model |
| --- | --- |
| `dlib_face_landmark_detection()`, `FaceLandmarkDetection` | `shape_predictor_5_face_landmarks.dat` or `shape_predictor_68_face_landmarks.dat` |
| `FaceRecognition` | `dlib_face_recognition_resnet_model_v1.dat` |
| `CnnFaceDetection` | `mmod_human_face_detector.dat` |

### Functions

| Function | Returns |
| --- | --- |
| `dlib_face_detection(string $imagePath, int $upsample = 0)` | Faces found by the HOG detector: a list of `['left', 'top', 'right', 'bottom']` arrays (`false` on error). |
| `dlib_face_landmark_detection(string $predictorPath, string $imagePath)` | One entry per detected face, each a list of `[x, y]` landmark points. |
| `dlib_chinese_whispers(array $edges)` | A cluster label per node, from a list of `[a, b]` node-index pairs (graph clustering, e.g. to group face descriptors). |
| `dlib_vector_length(array $a, array $b)` | The Euclidean distance between two equal-length float vectors. Throws if their sizes differ. |

`$upsample` enlarges the image that many times before detecting, to find
smaller faces at the cost of time.

### Classes

Each constructor takes the path of its model file and throws an
`Exception` if it cannot be loaded.

| Class | Methods |
| --- | --- |
| `CnnFaceDetection` | `__construct(string $modelPath)`, `detect(string $imagePath, int $upsample = 0): array` (same shape as `dlib_face_detection()`). |
| `FaceLandmarkDetection` | `__construct(string $modelPath)`, `detect(string $imagePath, array $rect): array`, with `$rect` a `['left', 'top', 'right', 'bottom']` array; returns `['rect' => [...], 'parts' => [['x' => .., 'y' => ..], ...]]`. |
| `FaceRecognition` | `__construct(string $modelPath)`, `computeDescriptor(string $imagePath, array $shape, int $numJitters = 1): array`, with `$shape` the `['rect', 'parts']` array above; returns the 128 floats of the face descriptor. |

### Example: image to face descriptor

```php
<?php
$image = 'photo.jpg';

$detector = new CnnFaceDetection('mmod_human_face_detector.dat');
$landmarks = new FaceLandmarkDetection('shape_predictor_5_face_landmarks.dat');
$recognizer = new FaceRecognition('dlib_face_recognition_resnet_model_v1.dat');

foreach ($detector->detect($image) as $rect) {
    $shape = $landmarks->detect($image, $rect);
    $descriptor = $recognizer->computeDescriptor($image, $shape);
    // Compare two descriptors with dlib_vector_length(): below about 0.6
    // is usually the same person.
}
```

### Limits in this build

- dlib is built without BLAS/LAPACK and without threads, so the neural
  models (`CnnFaceDetection`, `FaceRecognition`) are much slower than a
  native build. The HOG detector is the practical choice.
- No GPU.

---

## Extension details

| | |
| --- | --- |
| PHP extension | `mailmug/php_dlib` `v2.0.1` |
| Native library | dlib 20.0.1 (static, JPEG/PNG built in, no GUI/BLAS) |
| Patches | `config.m4` drops `-lstdc++` (not available under Emscripten) |
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
  minVersion, cxxRuntime: { name, version }, vendorLib: { name, version },
  buildHash }` (`cxxRuntime`: the C++ runtime linked in, versioned by its
  emsdk; `vendorLib`: dlib), mirroring the same
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
