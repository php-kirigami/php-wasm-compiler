# Provenance

This directory vendors `ext/xsl` from the `PHP-8.5.11` tag of `php/php-src`
(https://github.com/php/php-src/tree/PHP-8.5.11/ext/xsl): `config.m4`,
`php_xsl.c`, `php_xsl.h`, `xsltprocessor.c`, `php_xsl.stub.php`,
`php_xsl_arginfo.h`. `CREDITS`, `config.w32` and `tests/` are left out, the
same selection as `compile/extensions/pdo_dblib/PROVENANCE.md`.

The sources and `config.m4` are all unmodified.

Built as a `mode: shared` extension against libxslt 1.1.45 and libexslt,
vendored as `libxslt` (`compile/libxslt/Dockerfile`).

## Configure flags

`config.m4` runs `PKG_CHECK_MODULES([XSL], [libxslt >= 1.1.0])`, then an
optional `PKG_CHECK_MODULES([EXSLT], [libexslt])`. `pkgConfigVar: XSL`
answers the first. The second gets its own `EXSLT_CFLAGS`/`EXSLT_LIBS` in
`config.yaml`: without them the probe fails quietly and EXSLT support
(`HAVE_XSL_EXSLT`) is left out. `XSL_CFLAGS` is overridden to also point at
`include/libxml2`, since libxslt's headers include `<libxml/...>`.
`--with-xsl` is required (a `PHP_ARG_WITH`).

`php_xsl.h` includes `ext/dom/xml_common.h`, `ext/dom/xpath_callbacks.h` and
`ext/libxml/php_libxml.h`; `extraCflags` points at the php-src tree in the
extension build container, as for soap.

## Two copies of libxml2

`config.m4` has no `PHP_SETUP_LIBXML`, only the two probes above, so the
`libxslt` vendorLib is self-contained: `libxslt.a`, `libexslt.a` and a copy
of `libxml2.a`, the same 2.15.4 build the core links statically. `xsl.so`
therefore carries its own libxml2, as `soap.so` does.

Documents cross between the two: `importStylesheet()`/`transform*()` take
ext/dom documents built by the core's libxml2, and `transformToDoc()`
returns a document built by `xsl.so`'s copy, later freed by the core. That
works because both copies are the same version (identical struct layouts)
and share the core's `malloc`/`free`. What doesn't cross: global libxml2
state set by PHP on the core's copy. libxml errors raised inside libxslt's
copy don't reach `libxml_use_internal_errors()`/`libxml_get_errors()`, and
`document()`/`xsl:import`/`xsl:include` load files with libxml2's default
loader (plain file access in the VM's filesystem) rather than through PHP
stream wrappers.

The alternative, resolving libxml2 from the core's exports instead of
linking a copy, would need every libxml2 function libxslt/libexslt use
added to the core's exports (a long list, some of it not linked into the
core today).

## `smoke-test.php`

A real transformation from DOM documents, with an `xsl:sort`, an EXSLT
function (`str:tokenize`) and a PHP callback (`php:function`, which goes
through the core's ext/dom xpath callback code), then `transformToDoc()`.

Regenerating: re-download the six files listed above from the matching
PHP tag.
