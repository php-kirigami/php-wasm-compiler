# Bugs

Historical bugs found and fixed during development are recorded as dated
entries in the [DECISIONS.md](DECISIONS.md) decision log rather than
listed here, since each one is tied to the architectural decision it
prompted or resulted from.

Convention going forward: log a new bug here when it's found and still
open. Once fixed, move the entry into `DECISIONS.md` as a dated note (or
drop it if it's not architecturally significant) and remove it from this
file.

## Open

- **Side-module imports nothing provides (found 2026-09-23, unverified
  whether reachable).** After the libc exports of decision 64, these env
  imports still resolve to nothing: a call throws in the dynamic linker's
  lazy stub (a C++ caller then aborts, as pdo_firebird did).
  - pgsql, pdo_pgsql: `palloc`, `palloc0`, `palloc_mul`, `palloc0_mul`,
    `repalloc`, `pfree`, `pstrdup`, `pg_malloc0`, `pg_strdup`,
    `pg_log_generic`: libpq's own helpers (libpgcommon/libpgport), not
    linked into the vendored libpq.
  - soap: `php_session_start`, `php_get_session_status`,
    `php_get_session_var_str`, `php_set_session_var` (ext/session isn't in
    the core; only `SOAP_PERSISTENCE_SESSION` uses them).
  - ldap: `lutil_memcmp` (OpenLDAP's liblutil), `pthread_kill`.
  - enchant: `posix_spawnp`, and libffi's EM_JS `ffi_*_js` functions
    (ffi.so defines them; enchant alone doesn't).

