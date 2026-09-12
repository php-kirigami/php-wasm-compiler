dnl Internal self-test fixture for the mode:shared pipeline (CLAUDE.md
dnl decision 30/31) — not a real, publishable extension. It has no external
dnl library dependency on purpose, so it validates the raw
dnl @php-wasm/compile-extension -> php.wasm ABI compatibility without also
dnl needing a vendored dependency cross-compiled with Emscripten.

PHP_ARG_ENABLE([kirigami_abi_probe],
  [whether to enable kirigami_abi_probe support],
  [AS_HELP_STRING([--enable-kirigami-abi-probe],
    [Enable kirigami_abi_probe support])],
  [no])

if test "$PHP_KIRIGAMI_ABI_PROBE" != "no"; then
  PHP_NEW_EXTENSION(kirigami_abi_probe, kirigami_abi_probe.c, $ext_shared)
fi
