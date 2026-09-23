# Provenance

This directory vendors `ext/gettext` as-is from the `PHP-8.5.11` tag of
`php/php-src` (https://github.com/php/php-src/tree/PHP-8.5.11/ext/gettext) —
unmodified, no patches. Complete file listing verified against the real git
tree before vendoring (same process as `compile/extensions/ftp/PROVENANCE.md`).

Built as a `mode: shared` extension needing NO vendored external C library,
unlike gmp/tidy in this same batch: Emscripten's own musl libc already ships
`<libintl.h>` plus its own `gettext`/`bindtextdomain`/`ngettext`/etc.
implementations (all present in the sysroot's `libc.a`, verified with
`llvm-nm`; the core exports them to side modules through `.JS_ABI_EXPORTS`
in `compile/php/Dockerfile`).

`config.m4` is back to the real upstream file (2026-09-22). An earlier
local patch had turned its "libintl not found" errors into silent
fallbacks. That hid a failed `AC_CHECK_LIB([c], [bindtextdomain])` probe and
quietly compiled out `ngettext()`/`dngettext()`/`dcngettext()`/
`bind_textdomain_codeset()`. config.yaml now pre-seeds those probes'
`ac_cv_lib_*` cache variables instead, the same approach pgsql uses.
`config.m4`'s own
`for i in $PHP_GETTEXT /usr/local /usr; do test -r $i/include/libintl.h`
loop still needs a real filesystem DIR argument though, so `configArgs:
'--with-gettext=<sysroot>'` in config.yaml points straight at the
Emscripten sysroot's own `include/libintl.h` rather than vendoring a
redundant copy.

Regenerating: re-download the same 5 files (`config.m4`, `gettext.c`,
`gettext.stub.php`, `gettext_arginfo.h`, `php_gettext.h`) from the matching
PHP tag -- `CREDITS` and `config.w32` (Windows build) are the only files in
the real extension not vendored here, neither is relevant to this pipeline.
