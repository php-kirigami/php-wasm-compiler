# Provenance

This directory vendors `ext/ftp` as-is from the `PHP-8.5.11` tag of
`php/php-src` (https://github.com/php/php-src/tree/PHP-8.5.11/ext/ftp) —
unmodified, no patches. Complete file listing verified against the real git
tree (`gh api repos/php/php-src/git/trees/<ext/ftp sha>`) before vendoring,
after `compile/extensions/cmark/` (removed, see CLAUDE.md decision 40) turned
out to be silently incomplete.

Built as a `mode: shared` extension (CLAUDE.md decision 42) since it needs no
external C library (`config.m4`'s only optional dependency, `--with-ftp-ssl`,
defaults to off when `$PHP_OPENSSL` is unset, which it always is in an
isolated `@php-wasm/compile-extension` build) — plain `PHP_ARG_ENABLE`,
compiles straight from these bundled sources.

Regenerating: re-download the same 7 files (`config.m4`, `ftp.c`, `ftp.h`,
`ftp.stub.php`, `ftp_arginfo.h`, `php_ftp.c`, `php_ftp.h`) from the matching
PHP tag — `CREDITS` and `config.w32` (Windows build) are the only files in
the real extension not vendored here, neither is relevant to this pipeline.
