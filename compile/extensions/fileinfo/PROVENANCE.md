# Provenance

This directory vendors `ext/fileinfo` from the `PHP-8.5.11` tag of
`php/php-src` (https://github.com/php/php-src/tree/PHP-8.5.11/ext/fileinfo):
`config.m4`, `Makefile.frag`, `fileinfo.c`, `php_fileinfo.h`,
`php_libmagic.c`, `php_libmagic.h`, `fileinfo.stub.php`,
`fileinfo_arginfo.h`, `data_file.c`, and the whole `libmagic/` directory
(php-src's bundled, already-patched copy of file's libmagic, with its
`LICENSE`). `CREDITS`, `config.w32` and `tests/` are left out, the same
selection as `compile/extensions/pdo_dblib/PROVENANCE.md`, and so are the
maintainer tools that regenerate the bundled files (`create_data_file.php`,
`generate_patch.sh`, `libmagic.patch`, the empty `magicdata.patch`).

Everything is unmodified.

## No external library, one big file

libmagic is bundled in php-src, so there's no vendorLib. `config.m4` is a
`PHP_ARG_ENABLE` (default on), so compile-extension's own
`--enable-fileinfo` is all it needs.

`data_file.c` is file's compiled magic database (`magic.mgc`) as a C
array: 20 MB of source (about 530 KB compressed in git), about 10 MB of
data in the `.so`. `libmagic/apprentice.c` `#include`s it
(`Makefile.frag` records the dependency). That weight is why fileinfo is
`mode: shared` rather than in the core: it's only paid by those who
install the package. The data is loaded into the VM's memory along with
the module.

## `smoke-test.php`

Type detection from buffers (PNG and PDF signatures, JSON, plain text), a
full description (`PNG image data, ...`), and `mime_content_type()` on a
real file, which goes through libmagic's PHP-stream-based file path.

Regenerating: re-download the files listed above (and `libmagic/`) from
the matching PHP tag.
