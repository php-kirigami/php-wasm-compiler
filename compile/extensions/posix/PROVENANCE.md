# Provenance

Vendored as-is from the `PHP-8.5.10` tag of `php/php-src`
(https://github.com/php/php-src/tree/PHP-8.5.10/ext/posix) — unmodified
source, no patches. Complete file listing verified against the real git
tree; `CREDITS` and `tests/` are the only entries not vendored here.

No external library, `PHP_ARG_ENABLE([posix], ...)` — `--enable-posix`
(compile-extension's own default `--enable-${EXTENSION_NAME}`) is enough,
no `configArgs` needed.

Expected to work reasonably well under Emscripten/Node: most `posix_*`
functions (`posix_getpid`, `posix_getcwd`, `posix_uname`, `posix_times`,
...) map to libc calls Emscripten's own Node-targeted libc genuinely
implements. Some (`posix_getgrgid`, `posix_kill` against a real process
group, `posix_isatty` in a non-TTY context) may behave differently than on
real Linux, but that's a runtime behavior question, not a build/link one.
