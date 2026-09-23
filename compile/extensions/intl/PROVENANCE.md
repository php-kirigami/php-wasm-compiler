# Provenance

This directory vendors `ext/intl` from the `PHP-8.5.11` tag of
`php/php-src` (https://github.com/php/php-src/tree/PHP-8.5.11/ext/intl):
every file of the directory tree (165 files, the top-level sources plus
all subdirectories: `breakiterator/`, `calendar/`, `collator/`, `common/`,
`converter/`, `dateformat/`, `formatter/`, `grapheme/`, `idn/`,
`listformatter/`, `locale/`, `msgformat/`, `normalizer/`,
`resourcebundle/`, `spoofchecker/`, `timezone/`, `transliterator/`,
`uchar/`), except `CREDITS`, `config.w32` and `tests/`, the same selection
as `compile/extensions/pdo_dblib/PROVENANCE.md`.

Everything is unmodified.

Built as a `mode: shared` extension against ICU 78.3, vendored as `icu`
(`compile/icu/Dockerfile`). Like pdo_firebird, it's C++ and links its own
C++ runtime (the core has none).

## ICU data inside the `.so`

ICU needs its data (locales, collation, break rules, transliterators,
...): about 32 MB for ICU 78. It was the reason intl was left out so far
(`NOTICE.md`). Decided 2026-09-23: the data is compiled into the module
(`libicudata.a`, the native build's `.dat` file turned into C by
`genccode`), rather than shipped as a separate `.dat` file for
`ICU_DATA`. So `intl.so` works with no loader changes, and the ~32 MB is
only paid by those who install the package, and only in memory once the
module is loaded.

## Configure flags

`config.m4` is a `PHP_ARG_ENABLE`, so compile-extension's own
`--enable-intl` turns it on. `PHP_SETUP_ICU` (`build/php.m4`) runs
`PKG_CHECK_MODULES([ICU], [icu-uc >= 57.1 icu-io icu-i18n])`, answered by
`pkgConfigVar: ICU`. But it and `config.m4` also call pkg-config directly:
`$PKG_CONFIG icu-io --atleast-version=60` and `$PKG_CONFIG icu-uc
--atleast-version=74`, which picks C++17 over C++11. With no `.pc` file
to read, both are false, and ICU 78's headers don't compile as C++11.
`compile/icu/Dockerfile` stages ICU's own `icu-uc.pc`/`icu-i18n.pc`/
`icu-io.pc` in the vendorLib, and `config.yaml` sets `PKG_CONFIG_PATH` to
them. Their `prefix` is the ICU build container's, which doesn't matter:
only their `Version` field is read.

## Coexistence with php-norm

php-norm (static in the core) provides a `Normalizer` fallback and defers
to intl's when intl is loaded (`ZEND_MOD_OPTIONAL("intl")`, so the module
sort puts it after intl). This is the first build where that path is
exercised; `smoke-test.php` checks `Normalizer` comes from intl.

## `smoke-test.php`

Formatting in several non-root locales (a missing locale would silently
fall back to root, so this is what proves the data is there), collation,
normalization, transliteration, IDNA, grapheme and word breaking, message
formatting, `IntlChar`, and ext/date interop through `IntlCalendar`.

Regenerating: re-download the directory from the matching PHP tag
(everything but `CREDITS`, `config.w32`, `tests/`).
