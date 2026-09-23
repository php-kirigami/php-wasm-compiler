# Provenance

This directory vendors `ext/pdo_firebird` from the `PHP-8.5.11` tag of
`php/php-src` (https://github.com/php/php-src/tree/PHP-8.5.11/ext/pdo_firebird):
`config.m4`, `pdo_firebird.c`, `firebird_driver.c`, `firebird_statement.c`,
`pdo_firebird_utils.cpp`, `pdo_firebird_utils.h`, `php_pdo_firebird.h`,
`php_pdo_firebird_int.h`, `pdo_firebird.stub.php`, `pdo_firebird_arginfo.h`.
`CREDITS`, `config.w32` and `tests/` are left out, the same selection as
`compile/extensions/pdo_dblib/PROVENANCE.md`.

The C/C++ sources are unmodified. `config.m4` has one local patch (below).

Built as a `mode: shared` extension against Firebird 5's client library
(`libfbclient`), vendored as `firebird` (`compile/firebird/Dockerfile`).
It's the first C++ `mode: shared` extension in this repo:
`pdo_firebird_utils.cpp` is C++, and so is all of `libfbclient`.

## Configure flag: `--with-pdo-firebird=/build/vendor/firebird`

There's no `fb_config` in the build container, so `config.m4` takes its
directory branch, pointed at the vendorLib staging path (the same shape as
`pdo_dblib`). Its `PHP_CHECK_LIBRARY([fbclient], ...)` probe is a link test
that can't succeed against static archives without their dependencies, so
`config.yaml` answers it with `ac_cv_lib_fbclient_fb_get_master_interface=yes`
(same reason as `pgsql`'s cached answers).

## Local patch: `PHP_ADD_LIBRARY_WITH_PATH` removed from `config.m4`

The same libtool problem `tidy`, `bz2` and `pdo_dblib` hit. The macro adds a
bare `-L.../lib -lfbclient`, and libtool's shared-link mode can't resolve
it against a static-only archive with no `.la` sidecar. It silently
downgrades the build to a static module with no `.so`. The vendorLib
mechanism already passes every archive in the staged `lib/` to the final
link, so the call is commented out, with an explanation in place.

Re-apply this patch after re-vendoring.

## The C++ runtime is linked into the `.so`

The core `php.wasm` is a C-only `MAIN_MODULE=2` link: it has no libc++,
libc++abi or libunwind, and so no `__cpp_exception` Wasm EH tag (defined
by libc++abi). A side module importing that tag can't be instantiated at
all (`LinkError: ... "env" "__cpp_exception": tag import requires a
WebAssembly.Tag`). So `compile/firebird/Dockerfile` also stages PIC builds
of `libc++`, `libc++abi` and `libunwind` (the `-legacyexcept` variants,
matching `-fwasm-exceptions` with Emscripten's default legacy Wasm EH) next
to `libfbclient.a`, and they're linked into `pdo_firebird.so` itself. The
side module then defines `__cpp_exception` on its own. See the comment in
that Dockerfile for why the core doesn't export them instead.

## Firebird build notes

See `compile/firebird/Dockerfile`: client-only (`--enable-client-only`),
built with Firebird's own cross-compiling sequence rather than its
top-level make, bundled tommath/tomcrypt, no wire compression (no zlib),
no chacha wire-encryption plugin (a separate `dlopen()`ed plugin; the
built-in Arc4 stays available). ICU headers only: Firebird loads ICU with
`dlopen()` at runtime, only for charset/collation work. The
`firebird.msg` message file isn't installed yet, so errors carry their
codes but not the formatted message text.

## `smoke-test.php`

It connects to a closed local port (`127.0.0.1/1`), which runs
libfbclient's real attach path (yvalve's provider loop, the remote
client's INET connection code) before failing. That found the missing core
ABI exports listed in `compile/php/Dockerfile`'s `.JS_ABI_EXPORTS` (the
`pdo_firebird.so` block). Not yet tested against a real Firebird server.

Regenerating: re-download the ten files listed above from the matching PHP
tag, then re-apply the `config.m4` patch.
