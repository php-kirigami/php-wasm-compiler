# Provenance

This directory vendors `ext/soap` as-is from the `PHP-8.5.10` tag of
`php/php-src` (https://github.com/php/php-src/tree/PHP-8.5.10/ext/soap) —
unmodified, no patches.

Built as a `mode: shared` extension (config.yaml's `soap` entry), depending
on the same vendored libxml2 the static core already builds (`vendorLib:
libxml2`, `pkgConfigVar: LIBXML`, same mechanism as gmp -> libgmp).

Only the files needed to build the extension are vendored here (same
selection process as `compile/extensions/gmp/PROVENANCE.md`) — `CREDITS`,
`config.w32` (Windows build) and the whole `tests/` PHPT suite (729 files,
not needed to build) were intentionally left out.

Regenerating: re-download `config.m4`, `php_encoding.c/.h`, `php_http.c/.h`,
`php_packet_soap.c/.h`, `php_schema.c/.h`, `php_sdl.c/.h`, `php_soap.h`,
`php_xml.c/.h`, `soap.c`, `soap.stub.php`, `soap_arginfo.h` from the
matching PHP tag.
