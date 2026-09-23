# Provenance

This directory vendors `ext/pdo_mysql` as-is from the `PHP-8.5.11` tag of
`php/php-src` (https://github.com/php/php-src/tree/PHP-8.5.11/ext/pdo_mysql)
— unmodified source, no patches. Complete file listing verified against the
real git tree (`gh api repos/php/php-src/contents/ext/pdo_mysql?ref=PHP-8.5.11`)
before vendoring — `CREDITS`, `config.w32` (Windows build) and `tests/` are
the only entries in the real extension not vendored here.

## Configure flag: `--with-pdo-mysql`, not `--enable-pdo_mysql`

`@php-wasm/compile-extension`'s own `build-in-docker.sh` always passes
`--enable-${EXTENSION_NAME}` — but `pdo_mysql`'s own `config.m4` declares
itself via `PHP_ARG_WITH([pdo-mysql], ...)`, not `PHP_ARG_ENABLE`, with no
default value (defaults to "no" when unset). The hardcoded `--enable-
pdo_mysql` is simply an unrecognized flag autoconf ignores — it does NOT
turn the extension on. `config.yaml`'s `pdo_mysql` entry passes
`configArgs: '--with-pdo-mysql=mysqlnd'` to actually enable it (targeting
the mysqlnd driver explicitly, per the extension's own `AS_HELP_STRING`:
"If no value or mysqlnd is passed as DIR, the MySQL native driver will be
used").

## `mysqlnd` dependency (compile-time headers + runtime bundling)

Same shape as `mysqli` (CLAUDE.md decision 42) — confirmed directly from
`config.m4`: `PHP_ADD_EXTENSION_DEP(pdo_mysql, mysqlnd)` (only reached when
`$PHP_PDO_MYSQL` is `yes`/`mysqlnd`, which `configArgs` above guarantees),
and `php_pdo_mysql_int.h` `#include`s `"ext/mysqlnd/mysqlnd_debug.h"`
directly. `./ext/mysqlnd/` under this directory is the same header-only copy
(`cp compile/extensions/mysqli/ext/mysqlnd/*.h compile/extensions/pdo_mysql/
ext/mysqlnd/`) mysqli already vendors — regenerate the same way if mysqlnd's
version ever changes. `config.yaml`'s `pdo_mysql` entry sets
`bundleExtensions: [mysqlnd]`, reusing the same already-built, internal
`mysqlnd` package mysqli bundles — no second copy of mysqlnd is built or
published, `cli.mjs` just builds/stages it into this package's own output
directory too (same mechanism as mysqli, CLAUDE.md decision 42).

## `ext/pdo` dependency (compile-time headers only, no bundling needed)

`config.m4` also declares `PHP_ADD_EXTENSION_DEP(pdo_mysql, pdo)`, and
`pdo_mysql.c`/`mysql_driver.c`/`mysql_statement.c` all `#include
"ext/pdo/php_pdo.h"` and `"ext/pdo/php_pdo_driver.h"`. Unlike mysqlnd, this
needs **no** header staging here: `pdo`'s own `config.m4` defaults to
enabled (`PHP_ARG_ENABLE([pdo], ..., [yes])`) and `@php-wasm/compile-
extension`'s `Dockerfile.ext` configure invocation never disables it, so its
`PHP_INSTALL_HEADERS([ext/pdo], [pdo_sql_parser.h php_pdo_driver.h
php_pdo_error.h php_pdo.h])` already lands those headers under
`/usr/local/include/php/ext/pdo/` during the compile-extension build's own
base-image setup — the compile-extension build's standard `-I` search path
finds them there. And unlike mysqlnd, `pdo` itself is `mode: static` in this
repo's own core `php.wasm` (already loaded, no bundling needed) — only its
*headers* are a compile-time concern, never a *runtime* one.

## `mysql_sql_parser.c` (pre-generated from `mysql_sql_parser.re` via re2c)

`mysql_sql_parser.re` is vendored unmodified, but `mysql_sql_parser.c` is
**also** committed here, pre-generated — unlike every other vendored
extension in this repo, which relies on the compile-extension build's own
`phpize`/`make` to run any needed codegen step itself.

Real, concretely-observed reason (not a guess, found from an actual failed
build): `config.m4`'s `PHP_ADD_MAKEFILE_FRAGMENT` does pull in this
directory's `Makefile.frag`, whose `%.c: %.re` rule does invoke `$(RE2C)` —
but a plain `phpize`-based standalone extension build (what `@php-wasm/
compile-extension`'s `build-in-docker.sh` runs) never sets the `RE2C` make
variable at all (`AC_PROG_RE2C`-style detection only happens in a full
`buildconf` of the whole php-src tree, not in `phpize`'s minimal generated
`configure`). The rule still fires (since `mysql_sql_parser.c` doesn't
exist yet and the `.re` does), but with `$(RE2C)` empty it expands to a
bare ` -o mysql_sql_parser.c mysql_sql_parser.re` — the shell then tries to
run `-o` as a command and fails with "command not found" (make error 127).
This matches how official PHP release tarballs actually handle this exact
extension: they ship `mysql_sql_parser.c` pre-generated precisely because a
release tarball is built via `phpize`, not `buildconf`, same constraint.

Regenerated with `re2c 3.1` (already installed in `kirigami-compile-
extension:base`/`kirigami-php-wasm:base`, CLAUDE.md decision 46) via:

```
docker run --rm -v "<repo>/compile/extensions/pdo_mysql:/src" -w /src \
  kirigami-compile-extension:base re2c -o mysql_sql_parser.c mysql_sql_parser.re
```

Regenerate the same way if `mysql_sql_parser.re` is ever re-vendored from a
newer PHP tag (run from `/src` with relative filenames so the generated
`#line` directive stays a clean relative path, not a container-specific
absolute one).
