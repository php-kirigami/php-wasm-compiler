# Provenance

This directory vendors `ext/ldap` as-is from the `PHP-8.5.11` tag of
`php/php-src` (https://github.com/php/php-src/tree/PHP-8.5.11/ext/ldap) —
only `config.m4`, `ldap.c`, `ldap.stub.php`, `ldap_arginfo.h`, `php_ldap.h`
vendored (`CREDITS`, `config.w32`, `tests/` left out, same selection as
`compile/extensions/gmp/PROVENANCE.md`). Completely unmodified — unlike
`bz2`/`tidy`, no patch to `config.m4` was needed (see below).

Built as a `mode: shared` extension against the OpenLDAP client libraries
(`liblber`+`libldap`, vendored as `openldap`, `compile/openldap/
Dockerfile`).

## Configure flag: `--with-ldap`, not `--enable-ldap`

`config.m4` declares itself via `PHP_ARG_WITH([ldap], ...)` with no
explicit default, defaulting to "no" when unset — `@php-wasm/compile-
extension`'s own default `--enable-ldap` is an unrecognized flag autoconf
ignores. `config.yaml`'s `ldap` entry passes `configArgs: '--with-ldap'`
to actually turn it on.

## `libldap`/`liblber` detection: `pkgConfigVar: LDAP`, no config.m4 patch needed

`config.m4` tries `PKG_CHECK_MODULES([LDAP], [lber ldap], ...)` first. Its
env-override short-circuit (`pkgConfigVar: LDAP` -> `LDAP_CFLAGS`/
`LDAP_LIBS`, same mechanism as gmp/pgsql) steers it down the
`PHP_EVAL_LIBLINE`-based success branch (raw-archive-path friendly,
confirmed already by pgsql's own experience) — **not** the manual
`PHP_LDAP_CHECKS` directory-search fallback branch, which uses
`PHP_ADD_LIBRARY_WITH_PATH` and would hit the exact same libtool bug
`bz2`/`tidy` did (a bare `-L...-llber`/`-L...-lldap` link flag libtool's
shared-link mode can't resolve against a static-only archive). Getting the
pkg-config branch to succeed avoids that whole code path — no patch to the
vendored `config.m4` needed at all, unlike `bz2`/`tidy`.

`AC_CHECK_FUNCS`/`AC_CHECK_FUNC` (the "sanity check" for
`ldap_sasl_bind_s`/`ldap_simple_bind_s`) are plain `AC_CHECK_FUNC` calls,
not `AC_CHECK_LIB` — they rely purely on `$LIBS` (already set to
`$LDAP_LIBS`) rather than appending their own bare `-l` flag, so they don't
hit pgsql's own `AC_CHECK_LIB` gotcha (see `compile/extensions/pgsql/
PROVENANCE.md`) and needed no `ac_cv_*` overrides.

## Missing headers: `ldap_cdefs.h`

`lber_types.h` (included by `lber.h`, included by `ldap.h`) includes
`<ldap_cdefs.h>` — not one of the headers a naive "just copy the ones
`ldap.h` itself mentions" pass would catch on the first try (found the same
way `libpq`'s own `libpq/libpq-fs.h` gap was: a real "file not found" build
failure). `compile/openldap/Dockerfile`'s install step copies the full
transitive closure by hand: `ldap.h`, `lber.h`, `ldap_features.h`,
`lber_types.h`, `ldap_cdefs.h`.

## `smoke-test.php`

`ldap_connect()` is lazy (never opens a socket by itself) — a real
`ldap_bind()` attempt against a closed local port (`127.0.0.1:1`, no real
LDAP server needed, no hang) is what actually exercises libldap's
connection-establishment code path. Found ten genuinely missing libc ABI
exports this way (`wctomb`, `wcstombs`, `mbtowc`, `mbstowcs`, `strchr`,
`fopen`, `getuid`, `siprintf`, `snprintf`, `gai_strerror`), added to
`compile/php/Dockerfile`'s `.JS_ABI_EXPORTS`.

Regenerating: re-download `config.m4`, `ldap.c`, `ldap.stub.php`,
`ldap_arginfo.h`, `php_ldap.h` from the matching PHP tag.
