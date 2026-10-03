# Provenance

This directory vendors the `v2.0.1` tag of `mailmug/php-dlib`
(https://github.com/mailmug/php-dlib/tree/v2.0.1), published on Packagist as
`mailmug/php_dlib` for PIE (extension name `php_dlib`): face detection,
landmarks, recognition and clustering over the dlib C++ library.

Vendored: `config.m4`, `php_dlib.cpp`, `php_dlib.h`, `src/`, `LICENSE`
(MIT) and `CREDITS`. Left out: `tests/`, `config.w32`, `composer.json` and
`pdlib.php`.

Upstream sources are unmodified except `config.m4`, which loses its
`PHP_ADD_LIBRARY(stdc++, ...)` line (see "Build"). Local additions:
`README.md` (replaces upstream's, which documents a native `phpize` build
and Windows DLLs: this one follows the `@kirigami/phpext-*` README
template and is the same file as the package's, with the API and this
build's limits under Usage),
`wasm-pkgconfig/` and `smoke-test.php`.

## Build

`config.m4` is a `PHP_ARG_WITH` (`--with-php-dlib`), so `config.yaml` passes
it in `configArgs` (compile-extension's default `--enable-php_dlib` is
silently ignored). It finds dlib by running `pkg-config dlib-1` itself
(`--exists`, `--atleast-version`, `--cflags`, `--libs`), not through
`PKG_CHECK_MODULES`, so `pkgConfigVar` can't answer it. dlib's own
`dlib-1.pc` points at its build container's prefix, so `wasm-pkgconfig/`
holds a `dlib-1.pc` of our own whose `Cflags` point at the vendorLib staging
path (`/build/vendor/libdlib/include`) and whose `Libs` is empty (the
archive is linked by `vendorLibs`, which links each staged archive with
`--whole-archive`). `config.yaml` sets `PKG_CONFIG_PATH=/build/wasm-pkgconfig`.
Its `Version` matches the vendored dlib.

dlib is vendored as `libdlib` (`compile/libdlib/Dockerfile`): a static
archive built without GUI support, no
BLAS/LAPACK/CUDA, and JPEG/PNG support from dlib's own bundled
libjpeg/libpng/zlib. The extension is C++ and the core has no C++ runtime,
so `libcxx` is the second `vendorLibs` entry, as for rar. `-U__x86_64__`
for the same reason as rar.

`config.m4` adds `-lstdc++`, which doesn't exist under Emscripten (its C++
runtime is libc++): libtool warns "linker path does not have real file for
library -lstdc++" and falls back to a static module, so no `.so` is produced.
The `PHP_ADD_LIBRARY` line is removed; `libcxx` supplies the runtime.

## Models

The shape predictor, face-recognition ResNet and CNN detector are not
shipped: `FaceLandmarkDetection`, `FaceRecognition` and `CnnFaceDetection`
load them from paths the caller provides (dlib's `.dat` model files).

## `smoke-test.php`

Not from upstream (its `lenna.jpg` test image is not vendored). It runs the
clustering and vector functions, and `dlib_face_detection()` on a PNG and a
JPEG written with gd, which exercises the detector and both image decoders.

Regenerating: re-download the tag's source archive and copy everything
but `tests/`, `composer.json`, `config.w32`, `pdlib.php` and `README.md`.
