# Provenance

This directory vendors `ext/enchant` as-is from the `PHP-8.5.11` tag of
`php/php-src` (https://github.com/php/php-src/tree/PHP-8.5.11/ext/enchant)
— only `config.m4`, `enchant.c`, `enchant.stub.php`, `enchant_arginfo.h`,
`php_enchant.h` vendored (`CREDITS`, `config.w32`, `tests/` left out, same
selection as `compile/extensions/gmp/PROVENANCE.md`). Completely unmodified
— no patch needed, same as `ldap`.

Built as a `mode: shared` extension against a newly-vendored Enchant 2
(`libenchant2`, `compile/libenchant2/Dockerfile`), itself built on top of a
newly-vendored GLib (`compile/libglib/Dockerfile`) + libffi
(`compile/libffi/Dockerfile`) — the first GLib-based dependency chain in
this pipeline. See `matrix.json`'s own `libenchant2`/`libglib` notes for
the full "why this version, why these build flags" story for each of the
three vendored libraries.

## `libenchant-2` detection: `pkgConfigVar: ENCHANT2`, no config.m4 patch needed

`config.m4` tries `PKG_CHECK_MODULES([ENCHANT2], [enchant-2], ...)` first
(falling back to the legacy `enchant` module only if that fails — never
reached here). `pkgConfigVar: ENCHANT2`'s env-override short-circuit (same
mechanism as gmp/pgsql/ldap) steers it down the `PHP_EVAL_LIBLINE`-based
success branch directly — no config.m4 patch needed, same shape as `ldap`.

`config.yaml`'s `enchant` entry also re-overrides `ENCHANT2_CFLAGS` (same
"later configArgs wins" ordering as pgsql's own `PGSQL_LIBS` override) to
point one level deeper than the `vendorLib` mechanism's default:
`enchant.c` does `#include <enchant.h>` (bare, matching real
`enchant-2.pc`'s own `Cflags: -I.../enchant-2`), but the vendored headers
are staged at `include/enchant-2/enchant.h` — the default `-I.../include`
alone isn't enough.

## No spell-checking provider vendored yet

`libenchant2` builds and loads fine with zero providers (its own configure
only warns, "No spell-checking provider selected!", never fails) — but
`enchant_broker_request_dict()` will never find an actual dictionary until
a real provider (hunspell, most likely) is vendored as a follow-up. See
`compile/libenchant2/Dockerfile`'s own comment.

Without a provider, only personal word lists work
(`enchant_broker_request_pwl_dict()`): `enchant_dict_check()` gives the
right answers, but `enchant_dict_suggest()` always returns an empty array.
That's upstream behavior since libenchant 2.5.0, not a WASM bug: its NEWS
says "Enchant's mechanism for generating suggestions from personal
wordlists is removed". Suggestions now come only from the provider, which
can fold in the personal word list's words (Hunspell and Aspell do). A
provider is also a separate module that enchant loads with
`g_module_open()` (`dlopen()`), the same obstacle as an ODBC driver for
`ext/odbc`.

## Real, concretely-observed runtime bug (fixed at the toolchain level, not here)

Loading `enchant.so` at runtime failed with `SyntaxError: Unexpected token
'*'` — traced to a genuine upstream Emscripten bug in its dynamic-linker JS
generation (`src/lib/libdylink.js`'s `addEmJs()`), which strips pointer
stars from an `EM_JS` function's C-style argument types via a **non-global**
`jsArg.replace('*', '')`. For a double-pointer argument (`void **avalue`,
in libffi's own wasm32 `ffi_call_js` trampoline — legitimate C, needed for
its FFI marshalling), only the first `*` gets stripped, leaving a literal
`*` in the generated JS parameter name — invalid syntax. Fixed once, for
every future build, in `compile/base-image/Dockerfile` (patches the
Emscripten SDK's own `libdylink.js` to use a global regex replace) — not
specific to `ext/enchant` or any vendored library here, so not patched
per-extension.

Regenerating: re-download `config.m4`, `enchant.c`, `enchant.stub.php`,
`enchant_arginfo.h`, `php_enchant.h` from the matching PHP tag.
