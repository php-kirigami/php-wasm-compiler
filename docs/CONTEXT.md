# Project context

This repo is part of the Kirigami ecosystem (see `../kirigami/todo.md`,
section "Our own `php-wasm-builder`, derived from WordPress Playground").
The goal is to build our own PHP → WASM compiler, so we stop depending on
the fork as-is and can more easily update the PHP / extension / lib
versions we ship.

## Background

- `C:\projects\kirigami\php-wasm-builder` was our fork of the
  `WordPress/wordpress-playground` pnpm monorepo. It used to be a **huge
  full monorepo** (Playground packages, WordPress builds, website, etc.),
  later trimmed down to just `packages/php-wasm/`, and has since been
  **deleted entirely** (2026-09-12) — see
  [INSTRUCTIONS.md](INSTRUCTIONS.md) for why and how to recover it if ever
  needed. It is no longer on disk; treat it as historical reference only.
- The genuinely relevant file it had was
  `packages/php-wasm/compile/php/Dockerfile`: the Docker recipe that
  compiles PHP (php-src) to WASM via Emscripten, which had been modified
  on top of upstream Playground:
  - Stripping the opcache JIT sources incompatible with WASM
    (`ext/opcache/jit`), except `minilua.c`, `gen_ir_fold_hash.c`,
    `ir_strtab.c`, `ir_x86.dasc`.
  - Native compilation (gcc, not emcc) of `minilua` and
    `gen_ir_fold_hash` during the build, before the emmake step.
  - A patch to the generated Makefile for PHP 8.5, which otherwise forces
    the presence of the CLI build rule (`sapi/cli/php`).
  - Everything else (libcurl, libxml2, libzip, GD, imagick, asyncify vs.
    JSPI, etc.) came as-is from Playground.
- `packages/php-wasm/compile/*` (next to the Dockerfile) held the
  vendored sources/binaries of each third-party lib (libcurl, libpng16,
  libsqlite3, oniguruma, etc.) referenced by the Dockerfile via `COPY`.

All of the above was extracted into this repo; see
[DECISIONS.md](DECISIONS.md) for the settled architecture and
[STATUS.md](STATUS.md) for what has actually been built and verified.

## Current focus (2026-09-17)

Working on `mode: shared` extensions (separately loadable PHP extensions
compiled as WASM side modules) — see decision 45 onward in
[DECISIONS.md](DECISIONS.md) and the shared-extension items in
[STATUS.md](STATUS.md).
