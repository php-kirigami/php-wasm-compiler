# Provenance

This directory vendors `ext/mysqli` as-is from the `PHP-8.5.11` tag of
`php/php-src` (https://github.com/php/php-src/tree/PHP-8.5.11/ext/mysqli) —
unmodified source, no patches. Complete file listing verified against the
real git tree (`gh api repos/php/php-src/git/trees/<ext/mysqli sha>`) before
vendoring — `CREDITS` and `config.w32` (Windows build) are the only files in
the real extension not vendored here.

**The first `mode: shared` extension that depends on another `mode: shared`
extension being loaded first** (CLAUDE.md decision 42) — `mysqli`'s own
`config.m4` declares `PHP_ADD_EXTENSION_DEP(mysqli, mysqlnd)`, and its C
sources genuinely call into `mysqlnd`'s API (`mysqlnd_reverse_api.h` and
others), not just link a static C library the way `sodium`'s `vendorLib`
does. `mysqlnd` is a real, separate Zend module (its own `MINIT`), built and
published as its own package, `compile/extensions/mysqlnd/`.

`@php-wasm/compile-extension`'s own build environment installs only a
*minimal* matching PHP source tree before running `phpize` (confirmed in its
README's troubleshooting section: "If an extension includes headers from
optional PHP extensions, copy or generate those headers... or include them
in the extension source") — `ext/mysqlnd/` is exactly such an optional
extension, so its headers are NOT present by default. `./ext/mysqlnd/` under
this directory is a **copy of every header** (not the `.c` sources — those
are never referenced by `mysqli`'s own `PHP_NEW_EXTENSION` file list, so
they're not needed here) from `compile/extensions/mysqlnd/`, staged at the
exact relative path (`ext/mysqlnd/<file>.h`) `mysqli`'s own sources
`#include` (quoted includes resolve relative to the including file's own
directory before falling back to `-I` search paths — since `mysqli.c` etc.
sit at this directory's root, `ext/mysqlnd/mysqlnd.h` resolves to
`<this dir>/ext/mysqlnd/mysqlnd.h`). This only satisfies the *compile-time*
header dependency; the actual `mysqlnd_*` **symbols** `mysqli.so` calls are
left as unresolved imports (standard for an Emscripten `SIDE_MODULE`) and
must be satisfied *at runtime* by `mysqlnd.so` having already been
instantiated first — unverified until an actual `@php-wasm/universal` load
test confirms it (see CLAUDE.md decision 42 for the outcome).

Regenerating the `ext/mysqlnd/` header copy: `cp compile/extensions/mysqlnd/
*.h compile/extensions/mysqli/ext/mysqlnd/` (after re-vendoring `mysqlnd`
itself, if its version ever changes).
