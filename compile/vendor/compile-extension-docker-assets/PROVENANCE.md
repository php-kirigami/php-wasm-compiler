# Provenance

Vendors `compile-extension/docker/Dockerfile.ext` and
`compile-extension/scripts/build-in-docker.sh` from
`WordPress/wordpress-playground`'s tag `v3.1.53` — the exact tag matching
this repo's own installed `@php-wasm/compile-extension@3.1.53`
devDependency (`compile/package.json`). Unmodified, no patches: these two
files are the tool's own side-module build recipe, not something we intend
to maintain or change ourselves.

## Why this exists

`@php-wasm/compile-extension` (used by `cli.mjs compile-extension` for every
`mode: shared` extension) normally fetches its **entire** Docker asset set —
including `compile/base-image/Dockerfile`, the exact same Emscripten
toolchain setup this repo's own `compile/base-image/Dockerfile` is — fresh
from `WordPress/wordpress-playground` on first use, caching the result under
`~/.cache/php-wasm/compile-extension/docker-assets/<hash>/php-wasm/`. That
means every `mode: shared` build has been using a **different**,
un-patched base image than our own `kirigami-php-wasm:base` (missing, among
other things, decision 43's `em++` fix) — and depends on a live GitHub
fetch of a repo we don't control at all (CLAUDE.md decision 5's original
"unresolved concern," never actually closed).

Fixed for good (CLAUDE.md decision 45's follow-up) by vendoring only the
two files above — the ones genuinely specific to this tool, not duplicated
anywhere else in this repo — and having `compile/setup-compile-extension-
cache.mjs` assemble the rest of the expected directory structure from files
this repo already owns (`compile/base-image/*`, `compile/php/php8.5.patch`)
every time `cli.mjs compile-extension` runs, pointed at via the
`PHP_WASM_COMPILE_EXTENSION_CACHE_DIR` env var the tool already supports.
Net effect: `@php-wasm/compile-extension` now always builds its base image
from the **exact same, already-patched** Dockerfile our own `kirigami-php-
wasm:base` uses (Docker's own layer cache reuses the identical layers,
regardless of the different final tag it applies), and this repo never
makes a network call to `WordPress/wordpress-playground` again.

## Regenerating (only needed on a real `@php-wasm/compile-extension` upgrade)

Re-fetch both files from the tag matching the new version:

```
gh api repos/WordPress/wordpress-playground/contents/packages/php-wasm/compile-extension/docker/Dockerfile.ext?ref=v<version> --jq '.content' | base64 -d > compile-extension/docker/Dockerfile.ext
gh api repos/WordPress/wordpress-playground/contents/packages/php-wasm/compile-extension/scripts/build-in-docker.sh?ref=v<version> --jq '.content' | base64 -d > compile-extension/scripts/build-in-docker.sh
```

Then diff against the previous vendored copies before committing — if
either file changed meaningfully (not just this project's own unrelated
formatting), re-verify the ABI-compatibility reasoning in CLAUDE.md
decision 30 still holds.
