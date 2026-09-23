# Provenance

anydoc (`hosmelq/ext-anydoc` on Packagist, tag `v0.2.4`,
https://github.com/hosmelq/ext-anydoc/tree/v0.2.4) is a PHP extension
written in Rust with ext-php-rs, wrapping Firecrawl's `anydoc` crate: Word,
PowerPoint, Excel, OpenDocument, RTF, EPUB, CSV and PDF to GitHub-Flavored
Markdown. MIT.

Nothing from upstream is vendored in this directory. The Rust crate is
downloaded and built by `compile/anydoc/Dockerfile` (matrix.json's
`libraries.anydoc`), and this directory is our own phpize wrapper around
the result: `config.m4`, `anydoc_wasm.c` (intentionally empty) and
`smoke-test.php`.

## How it's built

`@php-wasm/compile-extension`'s README gives the recipe for Rust
extensions: build the crate as a `wasm32-unknown-emscripten` static
archive with `panic=abort` and a rebuilt standard library, and pass the
archive to a regular phpize build. Here:

- `compile/anydoc/Dockerfile` starts from the compile-extension image (PHP
  8.5 headers and `php-config`, same emsdk), installs a pinned Rust
  nightly with `rust-src` and the `wasm32-unknown-emscripten` target, and
  builds the crate with `crate-type` switched from `cdylib` to
  `staticlib`, `-Zbuild-std=std,panic_abort`, `-C panic=abort` and
  `-C relocation-model=pic` (a side module needs PIC code, std included).
  bindgen parses PHP's headers for the wasm32 target (the same
  `BINDGEN_EXTRA_CLANG_ARGS` compile-extension's own script exports);
  cc-rs build scripts (zstd-sys) compile C with emcc.
- ext-php-rs's build script reads the Zend API version, thread safety and
  debug flags from `php -i`. The image's only host PHP CLI is Ubuntu's
  8.3, which would select the wrong API, so `PHP` points at a small script
  answering for PHP 8.5 (API 20250925, NTS, no debug).
- `libanydoc.a` is staged as the `anydoc` vendorLib. compile-extension
  links it into the side module with `--whole-archive`; it provides
  `get_module` and the whole extension, so `anydoc_wasm.c` only exists
  because phpize needs a C source.

## Known risks

- `pdf-inspector` (anydoc's PDF path) uses `rayon`. The VM has no threads,
  so rayon's thread pool can't start real workers; the smoke test's PDF
  case is what checks that it degrades instead of aborting.
- `panic=abort`: a Rust panic aborts the whole PHP runtime instead of
  becoming a PHP exception. anydoc returns errors as PHP exceptions for
  bad input; only genuine bugs would panic.

## `smoke-test.php`

CSV bytes, a minimal DOCX built with the core's `ZipArchive`, and a
minimal one-page PDF, each converted to Markdown.
