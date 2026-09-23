# Provenance

This directory vendors `ext/gmp` as-is from the `PHP-8.5.11` tag of
`php/php-src` (https://github.com/php/php-src/tree/PHP-8.5.11/ext/gmp) —
unmodified, no patches. Complete file listing verified against the real git
tree before vendoring (same process as `compile/extensions/ftp/PROVENANCE.md`).

Built as a `mode: shared` extension needing a vendored external C library
(GNU MP, `compile/libgmp/Dockerfile`, same "vendorLib" mechanism as sodium ->
libsodium) -- `config.m4`'s `PHP_ARG_WITH([gmp], ...)` only runs its
`PKG_CHECK_MODULES([GMP], [gmp >= 4.2], ...)` probe when `$PHP_GMP` is
literally `yes` (bare `--with-gmp`, not `--with-gmp=DIR`), so `configArgs:
'--with-gmp'` in config.yaml is required -- @php-wasm/compile-extension's own
default `--enable-gmp` wouldn't be recognized at all (`PHP_ARG_WITH`, not
`PHP_ARG_ENABLE`), same gap decision 47 already found for pdo_mysql.
`pkgConfigVar: GMP` supplies `GMP_CFLAGS`/`GMP_LIBS` env overrides so that
probe finds our vendored `libgmp.a` without a real `gmp.pc`/pkg-config
present in the container, the same mechanism sodium's own `LIBSODIUM`
pkgConfigVar already relies on.

Regenerating: re-download the same 6 files (`config.m4`, `gmp.c`,
`gmp.stub.php`, `gmp_arginfo.h`, `php_gmp.h`, `php_gmp_int.h`) from the
matching PHP tag -- `CREDITS` and `config.w32` (Windows build) are the only
files in the real extension not vendored here, neither is relevant to this
pipeline.
