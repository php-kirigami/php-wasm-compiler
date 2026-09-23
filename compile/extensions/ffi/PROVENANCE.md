# Provenance

This directory vendors `ext/ffi` from the `PHP-8.5.11` tag of `php/php-src`
(https://github.com/php/php-src/tree/PHP-8.5.11/ext/ffi): `config.m4`,
`ffi.c`, `ffi.g`, `ffi_parser.c`, `php_ffi.h`, `ffi.stub.php`,
`ffi_arginfo.h`. `CREDITS`, `config.w32` and `tests/` are left out, the same
selection as `compile/extensions/pdo_dblib/PROVENANCE.md`. `ffi.g` is the
LLK grammar `ffi_parser.c` is generated from, kept for reference only (the
generated parser is committed upstream too, nothing regenerates it here).

The sources and `config.m4` are all unmodified.

Built as a `mode: shared` extension against libffi 3.8.0, vendored as
`libffi` (`compile/libffi/Dockerfile`, already there for enchant's GLib).

## Configure flags

`config.m4` is a single `PHP_ARG_WITH([ffi], ...)` with one
`PKG_CHECK_MODULES([FFI], [libffi >= 3.0.11])` probe, followed by
`PHP_EVAL_LIBLINE`. So `config.yaml`'s `pkgConfigVar: FFI` override is all
it needs (the same shape as gmp), plus an explicit `--with-ffi`, since the
compile-extension default `--enable-ffi` is ignored by a `PHP_ARG_WITH`.

## `ffi.enable=1` (manifest `iniEntries`)

`ffi.enable` defaults to `preload`: the FFI API is then only allowed under
the `cli` SAPI, or from a preloaded script. The core's SAPI is named `wasm`
(`compile/php/php_wasm.c`), and neither `@kirigami/php-wasm` nor
`check-shared-extension-symbols.mjs` renames it, so every `FFI::cdef()`
would throw `FFI API is restricted by "ffi.enable" configuration
directive`. `ffi.enable` is `PHP_INI_SYSTEM`, so `ini_set()` can't fix it
at runtime either. `config.yaml`'s `iniEntries` puts `ffi.enable=1` into
the package's `manifest.json`, and `register()` hands it to the loader,
which writes it after the `extension=` line.

The usual reason for `preload` (limiting native calls from web requests)
doesn't carry over the same way here: FFI in this VM can only reach code
already loaded into it (the core's exports and other side modules), not
arbitrary host libraries.

## libffi's wasm32 port

libffi implements `ffi_call` and closures for wasm32 with EM_JS
trampolines (`src/wasm/ffi.c`). EM_JS in a side module is wired up by the
main module's dynamic linker at load time, and its code runs in the main
module's JS scope. Its `EM_JS_DEPS` (`getWasmTableEntry`,
`setWasmTableEntry`, `getEmptyTableSlot`, `convertJsFunctionToWasm`) only
take effect in a main-module link, so they must already be in
`php_8_5.js`: they are, along with `stackSave`/`stackRestore`/`stackAlloc`
and the `HEAP*` views (dynamic linking pulls them in). The double-pointer
EM_JS signature (`void **avalue`) needs the fix in
`compile/base-image/patch-libdylink-emjs-multiptr.mjs`, found with enchant.

Known limit: `ffi_call_js` invokes the target through JS
(`getWasmTableEntry(fn).apply(...)`). A C function called through FFI that
suspends under JSPI (async I/O) would have to suspend through that JS
frame, which JSPI doesn't allow.

## `smoke-test.php`

It uses `FFI::cdef()` with no library, so symbols come from the core
(`dlsym` on `RTLD_DEFAULT`): `strlen()` for a plain `ffi_call`, and
`qsort()` with a PHP callback for a closure. Both need the called function
exported by the core (`.JS_ABI_EXPORTS` in `compile/php/Dockerfile`).

Regenerating: re-download the seven files listed above from the matching
PHP tag.
