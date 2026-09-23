# Provenance

This directory vendors `ext/pdo_pgsql` as-is from the `PHP-8.5.10` tag of
`php/php-src` (https://github.com/php/php-src/tree/PHP-8.5.10/ext/pdo_pgsql)
— unmodified source, no patches. Complete file listing verified against the
real git tree (`gh api repos/php/php-src/contents/ext/pdo_pgsql?
ref=PHP-8.5.10`) before vendoring — `CREDITS`, `config.w32` (Windows build)
and `tests/` are the only entries in the real extension not vendored here,
same selection as `compile/extensions/pdo_mysql/PROVENANCE.md`.

## No `mysqlnd`-style runtime dependency

Unlike `mysqli`/`pdo_mysql` (CLAUDE.md decision 42), `pdo_pgsql` does **not**
depend on another shared extension's Zend module being loaded first — its
`config.m4` has no `PHP_ADD_EXTENSION_DEP(pdo_pgsql, pgsql)`. Both `pgsql`
and `pdo_pgsql` link `libpq` directly and independently; `config.yaml` sets
no `bundleExtensions` here. `config.m4` does declare `PHP_ADD_EXTENSION_DEP
(pdo_pgsql, pdo)`, but `pdo` itself is `mode: static` in this repo's core
`php.wasm` (already loaded) — only its headers are a compile-time concern,
same as `pdo_mysql`'s own `pdo` dependency.

## Configure flag: `--with-pdo-pgsql`, not `--enable-pdo_pgsql`

`config.m4` declares itself via `PHP_ARG_WITH([pdo-pgsql], ...)`, defaulting
to "no" when unset — `@php-wasm/compile-extension`'s own default `--enable-
pdo_pgsql` is an unrecognized flag autoconf ignores. `config.yaml`'s
`pdo_pgsql` entry passes `configArgs: '... --with-pdo-pgsql'` to actually
turn it on.

## `libpq` detection: `pkgConfigVar: PGSQL`, plus `ac_cv_lib_pq_*` overrides

Same `PHP_SETUP_PGSQL` macro (`build/php.m4`) and `pkgConfigVar: PGSQL`
override as `compile/extensions/pgsql/PROVENANCE.md` — see that file for
the full mechanism, including why the two `AC_CHECK_LIB`-based probes this
extension's own `config.m4` runs (`PQresultMemorySize`, `PQclosePrepared`,
on top of `PHP_SETUP_PGSQL`'s own `PQencryptPasswordConn` floor check) are
also short-circuited via `ac_cv_lib_pq_*=yes` in `config.yaml`'s
`configArgs` rather than left to actually run.

## `pgsql_sql_parser.c` (pre-generated from `pgsql_sql_parser.re` via re2c)

`pgsql_sql_parser.re` is vendored unmodified, but `pgsql_sql_parser.c` is
**also** committed here, pre-generated — same real, concretely-observed
reason as `compile/extensions/pdo_mysql/PROVENANCE.md`'s own
`mysql_sql_parser.c`: a plain `phpize`-based standalone extension build (what
`@php-wasm/compile-extension`'s `build-in-docker.sh` runs) never sets the
`RE2C` make variable (`AC_PROG_RE2C`-style detection only happens in a full
`buildconf` of the whole php-src tree), so `Makefile.frag`'s `%.c: %.re`
rule would fail with a bare `-o pgsql_sql_parser.c pgsql_sql_parser.re`
(make error 127, "-o: command not found").

Regenerated with `re2c 3.1` (already installed in `kirigami-php-wasm:base`,
CLAUDE.md decision 46) via:

```
docker run --rm -v "<repo>/compile/extensions/pdo_pgsql:/src" -w /src \
  kirigami-php-wasm:base re2c -o pgsql_sql_parser.c pgsql_sql_parser.re
```

Regenerate the same way if `pgsql_sql_parser.re` is ever re-vendored from a
newer PHP tag.

## Missing header: `libpq/libpq-fs.h`

`php_pdo_pgsql_int.h` includes `<libpq/libpq-fs.h>` — see `compile/
extensions/pgsql/PROVENANCE.md`'s own note on this; `compile/libpq/
Dockerfile` installs it by hand.

## `libpq.a` isn't self-contained; `smoke-test.php` connects to a closed port

Both real, concretely-observed findings — see `compile/extensions/pgsql/
PROVENANCE.md`'s own notes on `libpgport_shlib.a`/`libpgcommon_shlib.a` and
on this extension's `smoke-test.php` connecting to `127.0.0.1:1` instead of
a real server. This extension's own smoke test additionally surfaced
`strchrnul` and `strcpy` as genuinely missing libc ABI exports beyond what
`pgsql`'s smoke test alone found (both added to `compile/php/Dockerfile`'s
`.JS_ABI_EXPORTS`).

Regenerating: re-download `config.m4`, `Makefile.frag`, `pdo_pgsql.c`,
`pdo_pgsql.stub.php`, `pdo_pgsql_arginfo.h`, `pgsql_driver.c`,
`pgsql_driver.stub.php`, `pgsql_driver_arginfo.h`, `pgsql_sql_parser.re`,
`pgsql_statement.c`, `php_pdo_pgsql.h`, `php_pdo_pgsql_int.h` from the
matching PHP tag, then regenerate `pgsql_sql_parser.c` as above.
