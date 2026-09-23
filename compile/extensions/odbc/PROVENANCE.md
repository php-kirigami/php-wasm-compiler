# Provenance

This directory vendors `ext/odbc` as-is from the `PHP-8.5.11` tag of
`php/php-src` (https://github.com/php/php-src/tree/PHP-8.5.11/ext/odbc):
`config.m4`, `php_odbc.c`, `odbc_utils.c`, `php_odbc.h`,
`php_odbc_includes.h`, `odbc.stub.php`, `odbc_arginfo.h`. `CREDITS`,
`config.w32` and `tests/` are left out, the same selection as
`compile/extensions/ldap/PROVENANCE.md`. The vendored files are unmodified;
see below for how the build works around `config.m4` without patching it.

Built as a `mode: shared` extension against the unixODBC driver manager
(`libodbc`), vendored as `unixodbc` (`compile/unixodbc/Dockerfile`).

## Backend selection: `ODBC_TYPE=unixODBC`, not `--with-unixODBC`

`config.m4` offers four mutually exclusive backends (`--with-ibm-db2`,
`--with-custom-odbc`, `--with-iodbc`, `--with-unixODBC`), each a
`PHP_ARG_WITH` inside an `AS_VAR_IF([ODBC_TYPE],, [...])` guard. The first
one enabled sets `ODBC_TYPE`, so the others are skipped.

That can't work under phpize (what `@php-wasm/compile-extension` runs):
`build/php.m4` sets `php_always_shared=yes` there, and `_PHP_ARG_ANALYZE`
then forces **every** `PHP_ARG_*` that evaluates to `no` to `yes, shared`.
The first block (ibm-db2) therefore always wins, even with
`--without-ibm-db2` passed explicitly. It then hard-fails:
`configure: error: ODBC header file '/home/db2inst1/sqllib/include/sqlcli1.h' not found!`

The workaround, passed through `config.yaml`'s `configArgs` and
`extraCflags`:

- `ODBC_TYPE=unixODBC` pre-sets the guard variable, so all four backend
  blocks are skipped. `config.m4` goes straight to its common "Extension
  setup" block, which defines `HAVE_UODBC` and `PHP_ODBC_TYPE` and links
  `$ODBC_LIBS`.
- `ODBC_CFLAGS`/`ODBC_LIBS` come from `pkgConfigVar: ODBC`, the same
  env-override mechanism as `ldap`/`gmp`.
- `ext_shared=yes`: normally set by the `PHP_ARG_*` analysis that was just
  skipped. Without it, `PHP_NEW_EXTENSION` builds the extension statically
  and the build ends with "Could not find a built .so under /build/modules".
- `-DHAVE_UNIXODBC=1` (`extraCflags`): the one define the skipped unixODBC
  block would have set. It makes `php_odbc_includes.h` include unixODBC's
  `sql.h`/`sqlext.h`.

The resulting extension reports `ODBC_TYPE` = `unixODBC`, the same as a
normal `--with-unixODBC` build.

## No driver shipped: what works and what doesn't

unixODBC is a **driver manager**. A real connection also needs a
database-specific ODBC driver (psqlODBC, MariaDB Connector/ODBC, the SQLite
ODBC driver, ...). The driver manager loads that driver at runtime with
`dlopen()` (through its bundled libltdl), from a path given in
`odbcinst.ini` or directly in a `DRIVER=` connection string.

No driver is vendored here (unixODBC's `--enable-drivers` stays off). So
today:

- The extension loads, and every `odbc_*` function is available.
- `odbc_connect()` fails cleanly: SQLSTATE `IM002` for an unknown DSN, and
  `01000` ("Can't open lib") for a `DRIVER=` path that can't be loaded.
  Both are exercised by `smoke-test.php`.
- Connecting to a real database would need a driver compiled as its own
  WASM side module and a `dlopen()` path that works inside the runtime.
  That hasn't been attempted.

## `smoke-test.php`

It exercises the driver manager's own code paths (ini parsing, DSN lookup,
driver loading), which can only fail without a driver. It found eight
exports missing from the core, one layer at a time (each fix let the call
get further):

1. `toupper`, `getpwuid`, `strncat`: the DSN lookup (ini parsing,
   `~/.odbc.ini` resolution).
2. `strrchr`, `access`: probing a `DRIVER=` library path.
3. `strlcpy`, `dlopen`, `dlerror`: libltdl actually trying to load the
   driver.

All are in `compile/php/Dockerfile`'s `.JS_ABI_EXPORTS`. `pdo_odbc` goes
through the same driver-manager code and needs the same set.

A `DRIVER=` path that can't be loaded fails with unixODBC's own SQLSTATE
`01000` ("Can't open lib ... : file not found"), not the ODBC spec's
`IM003`. The smoke test expects `01000`.

Regenerating: re-download the seven files listed above from the matching
PHP tag.
