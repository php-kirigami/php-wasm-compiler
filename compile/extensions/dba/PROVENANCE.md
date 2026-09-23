# Provenance

This directory vendors `ext/dba` as-is from the `PHP-8.5.11` tag of
`php/php-src` (https://github.com/php/php-src/tree/PHP-8.5.11/ext/dba) —
unmodified source, no patches. Complete file listing verified against the
real git tree (`gh api "repos/php/php-src/git/trees/PHP-8.5.11?recursive=1"`,
filtered to `ext/dba/`) before vendoring — `CREDITS`, `config.w32` (Windows
build) and `tests/` are the only entries in the real extension not vendored
here.

## No external library, no `vendorLib`, no `configArgs`

`dba` is a multi-backend extension (QDBM, GDBM, NDBM, Berkeley DB 1-4, DBM,
Tokyo Cabinet, LMDB, CDB, flat-file, ini-file) — but every backend that
needs an external C library (`qdbm`/`gdbm`/`ndbm`/`db1`-`db4`/`dbm`/
`tcadb`/`lmdb`) defaults to `[no]` in `config.m4`'s own `PHP_ARG_WITH(...,
[no], [no])` calls, and this repo doesn't vendor any of those libraries.
Only the three **bundled** backends — `cdb`, `inifile`, `flatfile` (their
own C sources ship inside `ext/dba/libcdb`/`libflatfile`/`libinifile`, no
external dependency at all) — default to **on**, via `config.m4`'s own
"enabled by default if any other option is enabled" logic:
`php_dba_enable = ($PHP_DBA != "no") ? yes : no`, and `--with-cdb`/
`--enable-inifile`/`--enable-flatfile` each default to `$php_dba_enable`.
Since `@php-wasm/compile-extension`'s build script always passes
`--enable-${EXTENSION_NAME}` (here, `--enable-dba`), that alone is already
enough to turn on exactly the three bundled backends and nothing else — no
`configArgs` override needed, unlike `pdo_mysql`'s `--with-pdo-mysql`
(CLAUDE.md decision 47) which needed one specifically because its own
`PHP_ARG_WITH` call has no default at all.

Confirmed from `config.m4`'s own `PHP_NEW_EXTENSION([dba], ...)` call: every
`dba_*.c` backend file (including the ones for libraries this build never
links) is compiled unconditionally — each file internally `#ifdef`s its own
body on the matching `DBA_*` macro, so `dba_gdbm.c` etc. compile to
essentially nothing when that backend's macro is undefined. This is why the
complete file set is vendored (including backends this build doesn't
actually use) rather than cherry-picked — matches decision 40's "always get
the real, complete file list" lesson, and the build would likely fail to
link `dba.c`'s own handler table against a partial file set anyway.

No `Makefile.frag` exists for this extension (unlike `pdo_mysql`'s
`mysql_sql_parser.re`/re2c) — no codegen step needed.
