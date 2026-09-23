# Provenance

This directory vendors `ext/pgsql` as-is from the `PHP-8.5.11` tag of
`php/php-src` (https://github.com/php/php-src/tree/PHP-8.5.11/ext/pgsql) —
unmodified, no patches. Only the files needed to build the extension are
vendored (`config.m4`, `pgsql.c`, `pgsql.stub.php`, `pgsql_arginfo.h`,
`php_pgsql.h`) — `CREDITS`, `config.w32` (Windows build) and `tests/` are
left out, same selection as `compile/extensions/gmp/PROVENANCE.md`.

Built as a `mode: shared` extension against a newly-vendored external C
library, `libpq` (`compile/libpq/Dockerfile`) — see that Dockerfile and
matrix.json's own `libpq` entry for how it's cross-compiled (static-only,
`--with-template=linux`, no ICU/zlib/OpenSSL yet).

## Configure flag: `--with-pgsql`, not `--enable-pgsql`

`config.m4` declares itself via `PHP_ARG_WITH([pgsql], ...)`, not
`PHP_ARG_ENABLE`, defaulting to "no" when unset. `@php-wasm/compile-
extension`'s own default `--enable-pgsql` is simply an unrecognized flag
autoconf ignores. `config.yaml`'s `pgsql` entry passes `configArgs:
'... --with-pgsql'` to actually turn it on — same gap decision 47 already
found for `pdo_mysql`/`gmp`.

## `libpq` detection: `pkgConfigVar: PGSQL`

`config.m4` calls the shared `PHP_SETUP_PGSQL` macro (`build/php.m4`),
which tries `PKG_CHECK_MODULES([PGSQL], [libpq >= 10.0], ...)` first, only
falling back to a `pg_config`/manual search if that fails. `PKG_CHECK_
MODULES([PGSQL], ...)` skips the actual pkg-config probe entirely when
`PGSQL_CFLAGS`/`PGSQL_LIBS` are already set in the environment — the same
override mechanism `pkgConfigVar: GMP`/`LIBSODIUM` already rely on for
gmp/sodium. `config.yaml`'s `vendorLib: libpq` + `pkgConfigVar: PGSQL`
supplies those two env vars, pointing at every `*.a` file
`compile/libpq/Dockerfile` installs (`cli.mjs`'s `vendorLibFlags()` picks up
all of them automatically — see the `libpq.a` isn't self-contained note
below for why there's more than one).

## `AC_CHECK_LIB`-based feature probes are unreliable here: `ac_cv_lib_pq_*` overrides

Beyond the `PKG_CHECK_MODULES` probe above, `PHP_SETUP_PGSQL` and this
extension's own `config.m4` each run several `PHP_CHECK_LIBRARY([pq],
[<function>], ...)` calls — a thin wrapper around `AC_CHECK_LIB`, used both
as a libpq-version floor check and to gate PG12-18 feature `#define`s
(`HAVE_PG_RESULT_MEMORY_SIZE`, `HAVE_PG_CHANGE_PASSWORD`,
`HAVE_PG_SOCKET_POLL`, `HAVE_PG_CLOSE_STMT`, `HAVE_PG_SERVICE`,
`HAVE_PG_SET_CHUNKED_ROWS_SIZE`).

Two real, concretely-observed problems with running these for real here:

1. `AC_CHECK_LIB` unconditionally appends a bare `-lpq` to its test link
   line, on top of whatever `$PGSQL_LIBS` already supplies — resolvable only
   via a real `-L` search path, not the vendored archive's absolute path
   (real failure: `wasm-ld: error: unable to find library -lpq`).
2. Even given a `-L`/`-l` pair that *does* resolve, side-module WASM linking
   (`--unresolved-symbols=import-dynamic`) doesn't fail on a genuinely
   undefined symbol the way a normal static link would — so `AC_CHECK_LIB`
   reports "found" regardless of whether the function actually exists.
   Concretely caught: `HAVE_PG_SERVICE` (`PQservice`) got defined even
   though `PQservice` doesn't exist anywhere in real libpq 18.6 (absent from
   both `libpq-fe.h` and the built `libpq.a`'s symbol table) — `config.m4`'s
   own "PostgreSQL 18 or later" comment looks ahead to an unreleased
   version (`v19beta3` as of this writing, 2026-09-22). Left un-overridden,
   this trips a hard "undeclared function" compile error in `pgsql.c`
   (`case PHP_PG_SERVICE: result = PQservice(pgsql);`, guarded by `#if
   defined(HAVE_PG_SERVICE)`).

Fix: pre-seed autoconf's own result cache variables
(`ac_cv_lib_<lib>_<function>`) in `config.yaml`'s `configArgs`, which
short-circuits each probe entirely instead of running it — no patch to the
vendored `config.m4`/`pgsql.c` needed. Every function except `PQservice` is
forced to `yes` (genuinely present — PG10-17 features, comfortably below
18.6); `PQservice` is forced to `no`.

## Missing header: `libpq/libpq-fs.h`

`php_pgsql.h` (large-object `O_*` mode constants) includes
`<libpq/libpq-fs.h>` — a server-tree header (`src/include/libpq/libpq-
fs.h`), not one of `src/interfaces/libpq`'s own public headers. `compile/
libpq/Dockerfile`'s install step copies it in by hand alongside
`libpq-fe.h`/`postgres_ext.h`, at `include/libpq/libpq-fs.h` (matching the
real install layout a full `make install` would produce).

## `libpq.a` isn't self-contained: `libpgport_shlib.a`/`libpgcommon_shlib.a`

`libpq.a` only archives `src/interfaces/libpq`'s own object files —
configure's own generated metadata for static consumers (`VAL_LIBS`) says
plainly a static `-lpq` also needs `-lpgcommon -lpgport -lm`. Real,
concretely-observed consequence: without vendoring those two as well,
`pg_vsnprintf` (Postgres's own portable `vsnprintf`, from
`src/port/snprintf.c` — not a libc function the main `php.wasm` module
could ever export) came up as a genuinely missing symbol at PHP runtime
when this extension's `smoke-test.php` actually exercised a real
connection attempt (`compile/check-shared-extension-symbols.mjs`).
`compile/libpq/Dockerfile` now also installs `libpgport_shlib.a`/
`libpgcommon_shlib.a` (the PIC variants, matching how `libpq.a` itself was
built) into the same `lib/` directory — `cli.mjs`'s `vendorLibFlags()` picks
up every `*.a` file there automatically.

## `smoke-test.php`: connects to a closed local port, not a real server

No real PostgreSQL server is available in `check-shared-extension-
symbols.mjs`'s isolated runtime boot. `pg_connect()` against `127.0.0.1`
port 1 (guaranteed closed, refuses immediately, no hang) still runs
`fe-connect.c`'s real connection-establishment code path — which is exactly
where the `libpq.a`-isn't-self-contained gap above, and several genuinely
missing libc ABI exports (`memchr`, `strstr`, `strncmp`, `strdup`,
`strchrnul`, `strcpy` — added to `compile/php/Dockerfile`'s
`.JS_ABI_EXPORTS`, same shape as dba's own `atoi`/`memcmp`/`strcasecmp`),
were actually found.

Regenerating: re-download `config.m4`, `pgsql.c`, `pgsql.stub.php`,
`pgsql_arginfo.h`, `php_pgsql.h` from the matching PHP tag.
