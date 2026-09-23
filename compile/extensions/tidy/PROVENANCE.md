# Provenance

This directory vendors `ext/tidy` as-is from the `PHP-8.5.11` tag of
`php/php-src` (https://github.com/php/php-src/tree/PHP-8.5.11/ext/tidy) —
only `config.m4`, `tidy.c`, `tidy.stub.php`, `tidy_arginfo.h`, `php_tidy.h`
vendored (`CREDITS`, `config.w32`, `tests/` left out, same selection as
`compile/extensions/gmp/PROVENANCE.md`).

Built as a `mode: shared` extension against `libtidy` (HTML Tidy,
`htacg/tidy-html5`, `compile/libtidy/Dockerfile`, CMake-based).

## `config.m4`: manual directory search, not `PKG_CHECK_MODULES`

Unlike gmp/sodium/pgsql, `config.m4` doesn't use pkg-config at all — it
manually probes `$TIDY_SEARCH_DIRS` for `include/tidy/tidy.h` or
`include/tidy.h` (our vendored `libtidy`'s headers are flat, at
`include/tidy.h`, matching the second form). `config.yaml`'s `configArgs:
'--with-tidy=/build/vendor/libtidy'` points `TIDY_SEARCH_DIRS` directly at
the `vendorLib`-staged path — no `pkgConfigVar` override needed.

## One patch to the vendored `config.m4`: `PHP_ADD_LIBRARY_WITH_PATH` removed

Real, concretely-observed build failure, same shape and same root cause as
`bz2`'s own experience (CLAUDE.md decision 53, before it moved to
`mode: static`): `PHP_ADD_LIBRARY_WITH_PATH([$TIDY_LIB_NAME], [$TIDY_LIBDIR],
[TIDY_SHARED_LIBADD])` unconditionally generates a bare
`-L$TIDY_LIBDIR -ltidy` link flag. Libtool's own shared-link bookkeeping
can't resolve that against a static-only archive with no `.la` sidecar
(`*** Warning: linker path does not have real file for library -ltidy`),
silently downgrading the build to a static-only module — which then fails
`@php-wasm/compile-extension`'s own "Could not find a built .so" check.
`config.yaml`'s `vendorLib: libtidy` mechanism already supplies the real
archive directly to the final link command via `--extra-ldflags`, so the
macro call is both redundant and harmful — removed (see the `dnl`-commented
block in `config.m4` for the inline explanation). Everything else in
`config.m4` is unmodified.

## `extraCflags: -DHAS_FUTIME=0`

Real build failure: `tidyplatform.h`'s own platform auto-detection
(`HAS_FUTIME`, gated on `CYGWIN_OS`/`LINUX_OS`/`__GLIBC__`/etc., none of
which Emscripten defines) defaults to `1` for an unrecognized platform and
`#include`s `<sys/utime.h>`, which doesn't exist in Emscripten's sysroot
(only the portable `<utime.h>`, the `HAS_FUTIME=0` branch). Forcing the
macro via `config.yaml`'s `extraCflags` (the header's own `#ifndef
HAS_FUTIME` guard allows this) needs no patch to the vendored header
itself.

## `smoke-test.php`

A real `tidy_parse_string()`/`cleanRepair()` round-trip on malformed HTML,
not just loading the module — found one real missing ABI export
(`vsnprintf`, libtidy's own message-formatting code) that a bare
`function_exists()`/load-only check would have missed.

Regenerating: re-download `config.m4` (then re-apply the
`PHP_ADD_LIBRARY_WITH_PATH` removal above), `tidy.c`, `tidy.stub.php`,
`tidy_arginfo.h`, `php_tidy.h` from the matching PHP tag.
