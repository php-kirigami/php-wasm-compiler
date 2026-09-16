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

**Built with compression support and SSL both disabled**
(`config.yaml`'s `configArgs: '--disable-mysqlnd-compression-support'` — SSL
is off by default already, since `config.m4`'s `$PHP_OPENSSL` check is
unset in an isolated `@php-wasm/compile-extension` build) — both would
otherwise need a vendored dependency (`zlib` for compression, `libopenssl`
for SSL) neither of which this pilot build needs to prove the core
mechanism. Add them back later (as `vendorLib`s, same pattern as
`sodium`/`compile/extensions/sodium`) once basic connectivity works.

This is the companion module `mysqli` (`compile/extensions/mysqli/`) depends
on — see that extension's own `PROVENANCE.md` for the two-shared-extensions
dependency this pilot is actually testing (CLAUDE.md decision 42).
`mysqlnd` must be loaded before `mysqli` at runtime.
