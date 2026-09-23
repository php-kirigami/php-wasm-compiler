# Provenance

This directory vendors `ext/snmp` from the `PHP-8.5.11` tag of `php/php-src`
(https://github.com/php/php-src/tree/PHP-8.5.11/ext/snmp): `config.m4`,
`snmp.c`, `php_snmp.h`, `snmp.stub.php`, `snmp_arginfo.h`. `CREDITS`,
`config.w32` and `tests/` are left out, the same selection as
`compile/extensions/pdo_dblib/PROVENANCE.md`.

The sources and `config.m4` are all unmodified.

Built as a `mode: shared` extension against net-snmp 5.9.5.2's client
library, vendored as `netsnmp` (`compile/netsnmp/Dockerfile`): libnetsnmp
only, with the vendored OpenSSL's `libcrypto.a` for SNMPv3.

## Configure flags

Given a bare `--with-snmp`, `config.m4` tries
`PKG_CHECK_MODULES([SNMP], [netsnmp >= 5.3])` first, and only falls back to
the `net-snmp-config` script when that fails. `pkgConfigVar: SNMP` answers
the probe, so no `net-snmp-config` is needed. Its two `PHP_CHECK_LIBRARY`
link probes (`init_snmp`, `shutdown_snmp_logging`) are answered with
`ac_cv_lib_netsnmp_*` instead of run, same reason as pgsql: `AC_CHECK_LIB`
is unreliable in a side-module link. The two `AC_CHECK_DECL` probes (SHA-256
and SHA-512 auth protocols) are compile-only and run normally.

## Runtime limits

- **UDP only**: ext/snmp always uses net-snmp's default UDP transport. It
  parses the host itself and takes the first `:` as the port separator, so
  a net-snmp transport prefix like `tcp:` can't be passed through (it
  becomes a host named `tcp`). Reaching an agent needs
  `@kirigami/php-wasm`'s UDP relay (its Node-side proxy relays UDP since
  2026-09-23) and the core's datagram fixes (`select()`/`poll()` readiness
  and blocking `recvfrom()`, in `compile/php/`).
- **No MIB files**: `--disable-mibs`, so only numeric OIDs resolve. MIB
  files could be shipped later as manifest `extraFiles`.
- **IPv4 only** (`--disable-ipv6`).

## `smoke-test.php`

Library-side settings (value retrieval, OID output format), then a real
`SNMP::get()` to `127.0.0.1:1`, a closed local port, which runs
net-snmp's session and transport code before failing (a warning and
`false`, or an `SNMPException`).

Regenerating: re-download the five files listed above from the matching
PHP tag.
