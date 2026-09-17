# Build status

**Extraction done (2026-09-11).** Copied from `php-wasm-builder` into this
repo (see `NOTICE.md` for the full provenance details):

- `compile/base-image/` — Ubuntu + Emscripten Dockerfile
  (`kirigami-php-wasm:base`).
- `compile/php/` — the big PHP Dockerfile (2762 lines) + per-version
  patches + C sources (`php_wasm.c`, `proc_open.c`, etc.) + Emscripten JS
  glue.
- `compile/lib*/` and `compile/oniguruma/` — Dockerfiles for each
  third-party lib (libcurl, libpng16, libjpeg, libwebp, libaom, libavif,
  libgd, libxml2, libopenssl, libsqlite3, libiconv, libz, libzip,
  libImageMagick).
- `compile/opcache/`, `compile/php-wasm-memory-storage/`,
  `compile/php-wasm-dns-polyfill/`, `compile/php-post-message-to-js/` —
  custom extensions/patches copied in by the PHP Dockerfile.
- `compile/Makefile` — builds each third-party lib into a `.a` via Docker.
- `compile/build.js` — Playground's existing Node orchestrator, which
  drives `make base-image` then `docker build`s the PHP Dockerfile with
  all the `--build-arg`s; **adapted** here to no longer depend on the
  monorepo's `packages/php-wasm/...` path (output now goes directly to
  `<repo>/node-builds/<version>/{asyncify,jspi}`).
- `supported-php-versions.mjs` / `.d.mts` + `compile/update-php-versions.mjs`
  — list of supported PHP versions, with auto-refresh (>24h).
- `LICENSE` (GPL-2.0-or-later, inherited from Playground) + `NOTICE.md`.

**Deliberately not copied for now:**

- Prebuilt binaries (`*/dist/`, ~175MB) — rebuilt via `make`/Docker instead
  of vendored, to stay aligned with the todo.md goal of updating things
  more easily. Consequence: `make all_jspi` must run (once, locally or in
  CI with caching) before PHP itself can be built.
- Playground's optional shared extensions (`intl` — needs `icu.dat`, 30MB
  —, `xdebug`, `redis`, `memcached`): out of scope for the MVP.
- `libncurses`/`libedit`: already inactive upstream (commented out in
  their own Makefile), not carried over.
- `compile-extension/` (Playground's pipeline for compiling third-party PHP
  extensions on the fly): see decision 5 — we depend on the published npm
  package rather than vendoring this.

**Config/CLI done (2026-09-11), see decisions 9-14:**

- `config.yaml` (root) — single config file (PHP versions, extensions with
  `mode: static|shared|off`, library versions, build options, output).
- `matrix.json` (root) — known versions/sources for each third-party lib,
  extracted from the existing Dockerfiles (see table in decision 11).
- `patches/README.md` — documented convention, **not wired up yet** into
  the lib Dockerfiles (only `compile/php/Dockerfile` already applies
  patches, a different way).
- `compile/cli.mjs` — new entry point: `node cli.mjs` (interactive,
  prompts via `prompts`), `node cli.mjs --quiet` (no prompts, uses
  `config.yaml` as-is), `--dry-run` (prints commands without building).
  Checks Node >= 24 and, on Windows, the WSL/Ubuntu/Node stack (decision
  13); checks Docker is reachable; validates extensions (`mode: shared` or
  unknown → explicit error, decision 9); then invokes `build.js` as a
  subprocess per PHP version. Tested with `--quiet --dry-run`: loads
  `config.yaml`, correctly resolves every `--build-arg`, correctly rejects
  `mode: shared`.
- Dependencies added to `compile/package.json`: `yaml`, `prompts`
  (`npm install` already done in `compile/`).
- `.gitignore` (root) — ignores `node_modules/`, `node-builds/`,
  `web-builds/`, `output/`, and `compile/*/{asyncify,jspi}/` (rebuilt
  binaries, not committed).

**✅ Full end-to-end build succeeded (2026-09-12).** `node compile/cli.mjs --quiet`
ran the whole pipeline for real and produced a working PHP 8.5.10 JSPI/Node
build:

- `node-builds/8-5/php_8_5.js` (Emscripten Node.js loader, ~391KB)
- `node-builds/8-5/8_5_10/php_8_5.wasm` (~28MB, verified valid — starts with
  the `\0asm` magic bytes + a `dylink` section, consistent with the
  side-module JSPI linking this whole pipeline is built around)

This is the same output shape documented in
`../kirigami/packages/php-wasm/README.md` (decision 15) — `jspi/php_8_5.js` +
`jspi/8_5_10/php_8_5.wasm`, modulo the `jspi/` prefix Playground's own
per-platform build.js adds that ours currently doesn't (see "Not done yet").
Getting here took a real debugging session — every fix is recorded above as
its own numbered decision (6 through 20) rather than repeated here; skim
those for the *why* behind each one. In short, on top of the initial
extraction: `libwebp`/`libxml2`/`oniguruma` needed an explicit
`--host wasm32-unknown-emscripten` (autoconf regenerated via
`autoreconf`/`autogen.sh` doesn't reliably detect cross-compilation on its
own), `libwebp`'s example CLI tools don't link cleanly under `wasm-ld` and
were excluded from the build, `libopenssl` was upgraded to a single 3.6.4
build (was two old 1.1.x ones) with a Windows-only symlink failure tolerated
via `|| true`, `dom`/`simplexml`/`xmlreader`/`xmlwriter` turned out to be
bundled with `libxml` rather than independently toggleable, `build.js` had
a Windows-only path bug breaking every `spawn()` call in the file, and the
PHP Dockerfile needed two directories (`node-builds/`, `compile/shared/`)
to exist (even empty) before its unconditional `COPY` instructions.

`make all_asyncify` was never run and should not be — decision 6 excludes
Asyncify entirely.

**✅ Runtime smoke-tested, not just "the Docker build exited 0" (2026-09-12).**
After aligning the extension set to the reference build (decision 22) and
removing the `wasm_recv` bug (decision 23), wrote a throwaway Node script
(scratchpad, not committed) that imports `php_8_5.js`'s `init()` directly
(bypassing `@php-wasm/universal`, which isn't a dependency of this repo) with
a minimal `PHPLoader` stub (`locateFile`, `onRuntimeInitialized`). Result:
`init()` resolves, the `.wasm` instantiates, and `onRuntimeInitialized`
fires cleanly — no crash, no missing symbols. This is real evidence the
compiled artifact works at runtime, not just that the build pipeline
completes. Not yet done: actually invoking PHP code through it (would need
either `@php-wasm/universal`'s `PHP.load()` — an intentionally-not-added
dependency here — or reverse-engineering `php_wasm.c`'s low-level `ccall`
entry points).

**Not done yet:**

- `config.yaml`'s `libraries:` section (third-party lib versions) is
  **not wired up yet** in `cli.mjs`/`build.js` — today `build.js` passes no
  lib-version `--build-arg` to the PHP Dockerfile's `docker build` (the
  libs are already pre-built via `make`, at whatever version is pinned in
  their own Dockerfile). Still to decide: resolve this at the
  `make`/Makefile level (pass the version as a `--build-arg` to the lib
  Dockerfiles) rather than at the `build.js` level.
- Automatic "latest" resolution per lib (decision 11) — not implemented,
  only documented in `matrix.json`.
- Per-lib patches (decision 12) — folder created, not wired into the
  Dockerfiles.
- No GitHub Actions workflow yet.
- No final scoping of which PHP versions/extensions to support beyond
  what's already in `config.yaml`.
- `jsonk` and `apcu` (decision 43) — **✅ done, build-tested, and
  runtime-verified for real.** `get_loaded_extensions()` lists both;
  `jsonk_encode`/`decode` work and `json_encode`/`decode` are jsonk-backed
  by default; APCu's cache genuinely persists across separate `PHP.run()`
  calls on the same runtime instance (the open question decision 43 was
  added to answer — confirmed "yes"). Took 7 real bugs and 9 rebuild
  attempts to get here (see decision 43 for the full list) — none of them
  were guesses, each was diagnosed from an actual build failure.
- `mode: shared` (separately loadable extension) — **✅ fully implemented
  AND runtime-verified for real, including complex/dependent extensions,
  as of decision 45.** The long-standing `__stack_pointer` LinkError
  (decisions 32/42) that blocked anything past a trivial fixture is fixed
  for good (root cause: `-Wl,--export=` doesn't survive Binaryen's DCE
  under `MAIN_MODULE=2 -O3`; real fix routes through `EXPORTED_FUNCTIONS`
  instead). `sodium` (`sodium_crypto_secretbox()` genuinely encrypts data),
  `ftp`, and the bundled `mysqlnd`+`mysqli` pair (`mysqli_init()` returns a
  real object, load order fixed via decision 45's ini-filename-prefix
  trick) all load and run correctly against a real build — not just build
  successfully. `cmark` was tried as a pilot (decisions 32, 35) but removed
  entirely (decision 40) — not worth the maintenance cost. Still missing:
  the `@kirigami/php-wasm`-side auto-detection/auto-load scan itself
  (decision 39 resolved *who* owns it — `@kirigami/php-wasm`, not the
  `kirigami` framework — but the scan code
  lives in the `kirigami` repo, not started).
- `libssh2` — **done** (decision 24): vendored, cross-compiled, wired into
  `libcurl` for SFTP/SCP.
- No npm package assembled yet for the **core** `mode: static` build from
  the raw `node-builds/8-5/` output (index.js, runtime/runtime.js,
  index.d.ts, matching `../kirigami/packages/php-wasm/README.md`'s
  documented shape, decision 15) — right now it's just the raw Emscripten
  build output, one directory level shallower (`node-builds/8-5/php_8_5.js`)
  than that package's `jspi/php_8_5.js`. (Shared extensions don't have this
  gap — see decision 31, their output directory already is the package.)
- Runtime smoke-tested with real PHP execution as of decision 30 (not just
  `.wasm` magic bytes / Docker exit code) — via `@php-wasm/universal`'s
  `loadPHPRuntime()` + `PHP.run()`, in the course of validating the
  `mode: shared` mechanism. Not yet tried: `@php-wasm/universal` wired as an
  actual dependency of a real, assembled `@kirigami/php-wasm` package (see
  point above).

**✅ Full rebuild confirmed green (2026-09-17), see DECISIONS.md decision 51.**
`node compile/cli.mjs --quiet` completed with exit code 0 against the
refreshed `matrix.json` (`navicat` v0.1.5, `jsonk` v0.1.4, `mdhtml` v0.1.2,
`norm` v0.1.1 — the `phpinfo()` footer redesign plus `navicat`'s missing
`version` row), producing `node-builds/8-5/php_8_5.js` +
`8_5_10/php_8_5.wasm` (26,957,515 bytes). This also closes out decision 50's
"not build-tested yet" caveat for `norm` and the earlier latest-tags bump.

**`compile/update-lib-versions.mjs` now checks everything, not just
GitHub-hosted `libraries` — see DECISIONS.md decision 52.** New resolvers
for GitLab, Gitiles/googlesource, SourceForge, and a generic
listing-page scan cover every remaining `sourceType`; `matrix.extensions`
(mdhtml/jsonk/navicat/norm/igbinary/apcu/yaml) is now checked too;
GitHub API calls authenticate via `gh auth token` when available (fixes
the 60/req-hour unauthenticated cap); every candidate is verified with a
live HEAD/GET before being written. This pass bumped `libiconv` (1.19),
`libz` (1.3.2), `libaom` (3.15.0), `libsqlite3` (3.53.4 — also fixed a
stale hardcoded release-year in its Dockerfile/sourceTemplate), and
`mdhtml` (v0.1.3, the RINIT/RSHUTDOWN fix). Not build-tested yet — no
Docker build run this session for the newly-bumped libraries.

