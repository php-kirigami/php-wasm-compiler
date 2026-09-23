# Provenance

This directory vendors the `v0.5.2` tag of `crazy-goat/qrcode-ext`
(https://github.com/crazy-goat/qrcode-ext/tree/v0.5.2), published on
Packagist as `crazy-goat/qrcode-ext` for PIE. Its extension name is
`scanmeqr`, which is what `config.yaml`, matrix.json (`extensions.scanmeqr`)
and the npm package (`@kirigami/phpext-scanmeqr`) are keyed by. It's the
native encoder behind `crazy-goat/scanmephp`: it exposes
`CrazyGoat\ScanMePHP\NativeEncoderCore`.

Everything from the tag is vendored except `tests/` and `composer.json`:
`config.m4`, the PHP glue (`scanme_qr.c`, which `#include`s
`native_encoder.c`, and their headers), the C++ core (`encoder`,
`reed_solomon`, `matrix`, `mask`, the mask kernels, `tables.hpp`,
`simd/`), `LICENSE` (MIT) and `README.md`.

Everything is unmodified.

## Build

No external library: the C++20 core is compiled into the extension. The
core has no C++ runtime, so `vendorLib: libcxx` (`compile/libcxx`) links
one into `scanmeqr.so`, as for rar.

`config.m4` is a `PHP_ARG_ENABLE` defaulting to yes. Its
`--with-scanmeqr-clib` option is only for building inside a ScanMePHP
checkout and isn't used. The AVX2 and AVX-512 mask kernels are only
compiled when `host_cpu` is x86, which it isn't here (wasm32), so the
portable kernel is the only one and `mask.cpp`'s CPU dispatch reduces to
it. compile-extension defines `__x86_64__` for every extension, which
would make `simd/cpuid.hpp` include `<cpuid.h>`; `config.yaml` undefines
it for this extension (`-U__x86_64__`), as for rar.

## `smoke-test.php`

Adapted from the extension's own `tests/002-encode-raw.phpt`: `encodeRaw()`
at the four error-correction levels, then a check of the finder pattern.

Regenerating: re-download the tag's source archive and copy everything
but `tests/` and `composer.json`.
