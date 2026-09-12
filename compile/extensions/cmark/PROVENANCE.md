# Provenance

This directory vendors `krakjoe/cmark` v1.2.0 (the real PECL `cmark`
extension's GitHub source, see `matrix.json`'s `extensions.cmark` entry),
**with all five `patches/cmark/*.patch` fixes already applied** — see
CLAUDE.md decision 35 for what each one fixes and why.

This is a deliberate exception to the convention used for the `mode:
static` build (`compile/php/Dockerfile` downloads `krakjoe/cmark` fresh at
build time and applies the same patches via `git apply --no-index`, never
vendoring pre-patched third-party source — see CLAUDE.md decision 34's
"architecture correction"). That convention exists because the static
build already downloads everything else fresh too, so vendoring a patched
copy would just be stale, duplicated state.

`mode: shared` extensions don't have that luxury: `@php-wasm/compile-extension`
(the tool `compile/cli.mjs compile-extension` shells out to) takes a local
`--source` directory as-is — it has no download/patch step of its own — so
a real, already-patched source tree has to exist somewhere for this build
path to work at all. Regenerating it: download the `v1.2.0` tag fresh from
`https://github.com/krakjoe/cmark`, apply the five `.patch` files under
`../../../patches/cmark/` in alphabetical order with `git apply --no-index`,
and copy the result here.
