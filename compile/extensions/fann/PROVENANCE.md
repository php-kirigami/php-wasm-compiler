# Provenance

This directory vendors the `v1.0.0` tag of `coral-media/ext-fann`
(https://github.com/coral-media/ext-fann/tree/v1.0.0), published on
Packagist as `coral-media/ext-fann` for PIE (extension name `fann`):
bindings for FANN, the Fast Artificial Neural Network library (network
creation, training, cascade training, activation functions, weights,
scaling).

Vendored: `config.m4`, `fann.c`, `php_fann.h`, `fann_arginfo.h`,
`fann.stub.php`, `lib/libfann/` (FANN's own sources, `COPYING.txt` and
`README.txt`) and `LICENSE.md`. Left out: `tests/`, `docs/`,
`phpdoc.dist.xml`, `config.w32`, `composer.json` and `README.md`.

Upstream sources are unmodified except `config.m4`, which loses its
`PHP_ADD_LIBRARY(m, ...)` line (see "Build"). Local additions: `README.md`
(follows the `@kirigami/phpext-*` README template and is the same file as
the package's; upstream's documents a native PIE/`phpize` install) and
`smoke-test.php`.

## Licenses

The extension is MIT (`LICENSE.md`). The vendored FANN sources are LGPL
2.1 or later (`lib/libfann/COPYING.txt`) and stay under it: the compiled
module links them statically, so `config.yaml` declares the license as
`MIT AND LGPL-2.1-or-later`. The FANN source is in this repository.

## Build

No external library: FANN is compiled into the extension from
`lib/libfann/src/floatfann.c` (the float build), so there is no
`vendorLib`, `pkgConfigVar` or `libcxx`. `config.m4` is a `PHP_ARG_WITH`
whose default is `yes`, so compile-extension's own `--enable-fann` is
ignored but harmless; `config.yaml` passes `--with-fann` anyway to not
depend on that default.

`config.m4` adds `-lm`, which has no file under Emscripten (libm is part of
its libc): libtool warns "linker path does not have real file for library
-lm" and falls back to a static module, so no `.so` is produced
(compile-extension: "Could not find a built .so"). The `PHP_ADD_LIBRARY`
line is removed.

## `smoke-test.php`

Not from upstream. Trains the XOR network with incremental training until
the error drops, checks the error went down, then saves the network and
loads it back (libfann's file I/O) and compares both networks' output.

Regenerating: re-download the tag's source archive and copy everything
but `tests/`, `docs/`, `phpdoc.dist.xml`, `config.w32`, `composer.json`
and `README.md`.
