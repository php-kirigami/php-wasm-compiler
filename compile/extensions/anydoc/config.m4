dnl Thin phpize wrapper for anydoc (hosmelq/ext-anydoc), a Rust extension
dnl built with ext-php-rs. Not upstream: the Rust crate is compiled to a
dnl wasm32-unknown-emscripten static archive by compile/anydoc/Dockerfile
dnl and staged as a vendorLib, which compile-extension links into the side
dnl module with --whole-archive. That archive provides get_module and the
dnl whole extension; anydoc_wasm.c only gives phpize a source to compile.
dnl See PROVENANCE.md.

PHP_ARG_ENABLE([anydoc],
  [whether to enable anydoc],
  [AS_HELP_STRING([--enable-anydoc], [Enable anydoc (Rust, via ext-php-rs)])])

if test "$PHP_ANYDOC" != "no"; then
  PHP_NEW_EXTENSION([anydoc], [anydoc_wasm.c], [$ext_shared])
fi
