# Provenance

This directory vendors `ext/pdo_odbc` as-is from the `PHP-8.5.11` tag of
`php/php-src` (https://github.com/php/php-src/tree/PHP-8.5.11/ext/pdo_odbc):
`config.m4`, `pdo_odbc.c`, `odbc_driver.c`, `odbc_stmt.c`,
`php_pdo_odbc.h`, `php_pdo_odbc_int.h`, `pdo_odbc.stub.php`,
`pdo_odbc_arginfo.h`. `CREDITS`, `config.w32` and `tests/` are left out, the
same selection as `compile/extensions/pdo_pgsql/PROVENANCE.md`. The vendored
files are unmodified, with no patches.

Built as a `mode: shared` extension against the same unixODBC driver
manager as `ext/odbc` (`vendorLib: unixodbc`, `compile/unixodbc/Dockerfile`).

## No runtime dependency on `ext/odbc`

`config.m4` declares only `PHP_ADD_EXTENSION_DEP(pdo_odbc, pdo)`. `pdo` is
`mode: static` in the core `php.wasm`, so only its headers matter at compile
time, as with `pdo_mysql`/`pdo_pgsql`. `pdo_odbc` links `libodbc`
independently, so `config.yaml` sets no `bundleExtensions`.

## Configure flag: `--with-pdo-odbc=unixODBC`

`config.m4` is a single `PHP_ARG_WITH([pdo-odbc], ...)`. The compile-extension
default `--enable-pdo_odbc` is ignored, so the flag must be passed
explicitly. Its value is a "flavour": `unixODBC` selects the
`PKG_CHECK_MODULES([PDO_ODBC], [odbc])` branch. `pkgConfigVar: PDO_ODBC`
short-circuits that probe with `PDO_ODBC_CFLAGS`/`PDO_ODBC_LIBS`, the same
mechanism as `ldap`/`gmp`.

With only one `PHP_ARG_*`, this extension doesn't hit `ext/odbc`'s phpize
backend-selection problem (see `compile/extensions/odbc/PROVENANCE.md`).

## No driver shipped

The limitation is the same as `ext/odbc`'s: a real connection needs a
database-specific ODBC driver loaded with `dlopen()`, and none is vendored.
`smoke-test.php` checks that the `odbc` PDO driver is registered, and that
`new PDO('odbc:...')` fails cleanly: SQLSTATE `IM002` for an unknown DSN,
and `01000` ("Can't open lib") for a `DRIVER=` path that can't be loaded.

Regenerating: re-download the eight files listed above from the matching
PHP tag.
