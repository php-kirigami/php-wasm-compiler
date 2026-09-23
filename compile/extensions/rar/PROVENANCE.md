# Provenance

This directory vendors the `v4.3.1` tag of `cataphract/php-rar`
(https://github.com/cataphract/php-rar/tree/v4.3.1), the PECL rar
extension, published for PIE on Packagist as `cataphract/rar`. It's the
first mode:shared extension vendored from its own repository rather than
from php-src; matrix.json's `extensions.rar` records the repo and version.

Vendored: `config.m4`, `Makefile.frag`, the extension sources (`rar.c`,
`rar_error.c`, `rar_navigation.c`, `rar_stream.c`, `rar_time.c`,
`rararch.c`, `rarentry.c`, `php_rar.h`, `php_compat.h`, `rar.map`),
`LICENSE`, `CREDITS`, `README.md`, and the whole bundled `unrar/`
directory (RARLAB's UnRAR source, with its `LICENSE.txt`). Left out:
`tests/`, `docker/`, `Justfile`, `config.w32`, `package.xml`,
`composer.json`, the IDE project files (`php-rar.config`/`.creator`/
`.files`/`.includes`), `example.php`, `run-tests-rar.php`,
`valgrind.supp` and the maintainer notes (`extflow.txt`, `technote.txt`,
`unrardll.txt`, `unrar_update.md`, `php_upgrade.md`), plus one stray
libtool temporary file upstream commits by mistake (`unrar/crypt2.loT`).

Everything is unmodified.

## Build

No external library: the unrar code is compiled into the extension
(`config.m4` lists its `.cpp` files). But it's C++, and the core has no
C++ runtime, so `config.yaml` gives it `vendorLib: libcxx`
(`compile/libcxx/Dockerfile`, PIC libc++/libc++abi/libunwind alone),
linked into `rar.so`, as firebird and icu do for pdo_firebird and intl.

`config.m4` is a `PHP_ARG_ENABLE`, so compile-extension's own
`--enable-rar` is all it needs. Its `--version-script` link probe fails
under wasm-ld and is skipped, which only means the export list isn't
trimmed. unrar's multithreading (`RAR_SMP`) is only defined for Windows
builds, so this one is single-threaded.

## Licenses

php-rar is under the PHP License 3.01. The bundled UnRAR source is under
RARLAB's own freeware license (`unrar/LICENSE.txt`): free to use and
redistribute, but it may not be used to re-create the RAR compression
algorithm. The package's `license` is set to
`PHP-3.01 AND LicenseRef-UnRAR` in `config.yaml` accordingly.

## `smoke-test.php`

A real archive from php-rar's own tests (`latest_winrar.rar`, 712 bytes,
embedded as base64), read through `RarEntry::getStream()`, extracted to
disk with `RarEntry::extract()`, and read again through the `rar://`
stream wrapper.

Regenerating: re-download the tag's source archive and copy the files
listed above.
