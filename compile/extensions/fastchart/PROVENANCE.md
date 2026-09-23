# Provenance

This directory vendors the `1.7.4` tag of `iliaal/fastchart`
(https://github.com/iliaal/fastchart/tree/1.7.4), published on Packagist as
`iliaal/fastchart` for PIE (extension name `fastchart`): chart rendering (38
chart families, plus QR codes and Code128 barcodes) to SVG, PNG, JPEG and
WebP.

Vendored: `config.m4`, every `fastchart*.c`/`fastchart*.h`,
`php_fastchart.h`, `fastchart.stub.php`, `fastchart_arginfo.h`, the bundled
`vendor/` directory (plutovg, plutosvg, qrcodegen, each with its license),
`LICENSE` and `README.md`. Left out: `tests/`, `bench/`, `docs/`,
`images/`, `scripts/`, `config.w32`, `composer.json` and the project's
Markdown notes (`CHANGELOG.md`, `CONTRIBUTING.md`, `SECURITY.md`,
`optimization.md`).

Upstream sources are unmodified except `config.m4`, which adds one file to
its source list (`wasm_default_font.c`, see "Fonts"). Local additions:
`wasm-pkgconfig/` (below), `wasm_default_font.c` and `wasm-font/`.

## Build

`config.m4` is a `PHP_ARG_ENABLE`. It finds FreeType (required) and
libpng, libjpeg, libwebp (each optional: a missing one only disables its
output format) by running `pkg-config` itself, `--exists`, `--cflags` and
`--libs`, rather than through `PKG_CHECK_MODULES`, so `config.yaml`'s
`pkgConfigVar` can't answer it. The vendored libraries' own `.pc` files
point at their build containers' prefixes and use `-L`/`-l` flags (the
libtool static-archive problem tidy and bz2 hit). So `wasm-pkgconfig/`
holds four small `.pc` files of our own (freetype2, libpng, libjpeg,
libwebp) whose `Cflags` point at the vendorLib staging paths
(`/build/vendor/<lib>/include`) and whose `Libs` name the staged archives,
and `config.yaml` sets `PKG_CONFIG_PATH=/build/wasm-pkgconfig`. Their
`Version` fields match the vendored libraries.

The libraries themselves are the ones already vendored for gd (`vendorLibs`
libfreetype, libpng16, libjpeg, libwebp, libz). `archives` limits each to
one copy of its code: libjpeg's build also ships `libturbojpeg.a` and
libpng16's ships `libpng.a`, which would be duplicate definitions, since
every staged archive is linked with `--whole-archive`.

No PDF output: `--with-pdfio` needs pdfio, which isn't vendored;
`renderPdf()` throws "PDF support not compiled in". The
`-Wl,--exclude-libs=ALL` link probe fails under wasm-ld and is skipped.

## Fonts

Every chart class measures and draws text through FreeType, from a font
file: `setFontPath()`, or a default probed at a few system paths
(`fastchart.c`'s `FASTCHART_DEFAULT_FONT_CANDIDATES`,
`/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf` first). The VM has no
system fonts.

So the module carries one. `wasm_default_font.c` embeds DejaVu Sans 2.37
(`wasm-font/DejaVuSans.ttf`, 740 KB, with its `LICENSE`: Bitstream Vera
terms plus public-domain DejaVu changes, free to redistribute) with C23
`#embed`, and a constructor, run when the module is loaded (before
fastchart's MINIT), writes it to that first candidate path unless a file
is already there. fastchart's own probe then finds it; `setFontPath()`
and an application-provided font at that path still take precedence. The
font costs about 740 KB in the `.so` and as much again in the VM's
filesystem once written. The only change to upstream is `config.m4`
listing `wasm_default_font.c` among its sources (re-apply after
re-vendoring).

## `smoke-test.php`

A QR code rendered to SVG, PNG, JPEG and WebP (one output per library),
a Code128 barcode to PNG, then a line chart with a title (SVG and PNG),
which only renders through the embedded default font.

Regenerating: re-download the tag's source archive, copy the files listed
above, re-apply the `config.m4` source-list line, and keep
`wasm-pkgconfig/` (update its `Version` fields if the vendored codec
libraries change), `wasm_default_font.c` and `wasm-font/`.
