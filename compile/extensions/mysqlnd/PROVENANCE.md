# Provenance

This directory vendors `ext/mysqlnd` as-is from the `PHP-8.5.10` tag of
`php/php-src` (https://github.com/php/php-src/tree/PHP-8.5.10/ext/mysqlnd) —
unmodified source, no patches. One rename: php-src's own file is
`config9.m4`, not `config.m4` — a php-src-tree-only convention (numbered
config fragments control macro-processing order when `buildconf` combines
every `ext/*/config*.m4` into one big `./configure` for the whole tree).
`phpize`-based standalone builds (what `@php-wasm/compile-extension` does)
only ever look for a plain `config.m4`, so this vendored copy renames it.

Complete file listing verified against the real git tree (`gh api repos/
php/php-src/git/trees/<ext/mysqlnd sha>`) before vendoring — `CREDITS`,
`config-win.h`, `config.w32` (Windows build) are the only files in the real
extension not vendored here.

**Compression support and extended SSL are both enabled** (CLAUDE.md
decision 45's follow-up) — `config.yaml`'s `vendorLibs` stages the already
built-for-the-static-core `libz`/`libopenssl` into `source/vendor/`, and
`--with-mysqlnd-ssl` overrides the default-off SSL flag (which otherwise
depends on `$PHP_OPENSSL`, always unset in an isolated
`@php-wasm/compile-extension` build). Both were disabled in an earlier
pilot pass to prove the core mysqlnd/mysqli mechanism without a vendored
dependency first — the pattern used here is the exact same `vendorLib`
mechanism `sodium`/`compile/extensions/sodium` already validated.

This is the companion module `mysqli` (`compile/extensions/mysqli/`) depends
on — see that extension's own `PROVENANCE.md` for the two-shared-extensions
dependency this pilot is actually testing (CLAUDE.md decision 42).
`mysqlnd` must be loaded before `mysqli` at runtime.
