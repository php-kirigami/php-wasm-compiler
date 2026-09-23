# Provenance

This directory vendors `ext/pdo_dblib` from the `PHP-8.5.11` tag of
`php/php-src` (https://github.com/php/php-src/tree/PHP-8.5.11/ext/pdo_dblib):
`config.m4`, `pdo_dblib.c`, `dblib_driver.c`, `dblib_stmt.c`,
`php_pdo_dblib.h`, `php_pdo_dblib_int.h`, `pdo_dblib.stub.php`,
`pdo_dblib_arginfo.h`. `CREDITS`, `config.w32` and `tests/` are left out,
the same selection as `compile/extensions/pdo_pgsql/PROVENANCE.md`.

The C sources are unmodified. `config.m4` has one local patch (below).

Built as a `mode: shared` extension against FreeTDS's db-lib (`libsybdb`),
vendored as `freetds` (`compile/freetds/Dockerfile`). It talks TDS, the wire
protocol of Microsoft SQL Server and Sybase ASE.

## Configure flag: `--with-pdo-dblib=/build/vendor/freetds`

`config.m4` is a single `PHP_ARG_WITH([pdo-dblib], ...)` with no pkg-config
branch. Given a directory, it checks for `DIR/include/sybdb.h` and
`DIR/lib/libsybdb.a` itself. So `config.yaml` points it straight at the
vendorLib staging path, the same shape as `tidy` (no `pkgConfigVar`).

## Local patch: `PHP_ADD_LIBRARY_WITH_PATH` removed from `config.m4`

The same libtool problem `tidy` and `bz2` hit. The macro adds a bare
`-L.../lib -lsybdb`, and libtool's shared-link mode can't resolve it
against a static-only archive with no `.la` sidecar. It silently
downgrades the build to a static module with no `.so`. The vendorLib
mechanism already passes the real `libsybdb.a` to the final link, so the
call is commented out, with an explanation in place.

Re-apply this patch after re-vendoring.

## FreeTDS build notes

See `compile/freetds/Dockerfile`. db-lib only, with no TLS (encrypted
connections, which Azure SQL requires, aren't possible yet) and no
Kerberos. `--disable-threadsafe` means MARS must be disabled too
(`--disable-mars`), since its code needs thread condition variables.
Charset conversion (UCS-2LE for TDS 7+) goes through musl's `iconv`. FreeTDS
also ships an ODBC driver (`src/odbc`), not built here. It's a possible
future driver for `ext/odbc`/`ext/pdo_odbc`.

## `smoke-test.php`

It connects to a closed local port (`127.0.0.1:1`), which runs FreeTDS's
real login and socket code before failing. That found four libc exports
missing from the core (`socketpair`, `gethostname`, `strerror`,
`asprintf`). They were added to `compile/php/Dockerfile`'s
`.JS_ABI_EXPORTS`. Unlike ldap's real bind attempt, this does not hang the
check's minimal runtime. Not yet tested against a real SQL Server.

Regenerating: re-download the eight files listed above from the matching
PHP tag, then re-apply the `config.m4` patch.
