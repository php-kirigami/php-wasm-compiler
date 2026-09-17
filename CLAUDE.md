# php-wasm-compiler

## Context

This repo is part of the Kirigami ecosystem (see `../kirigami/todo.md`,
section "Our own `php-wasm-builder`, derived from WordPress Playground").
The goal is to build our own PHP → WASM compiler, so we stop depending on
the fork as-is and can more easily update the PHP / extension / lib
versions we ship.

Background (reference state to explore, not to copy as-is):

- `C:\projects\kirigami\php-wasm-builder` was our fork of the
  `WordPress/wordpress-playground` pnpm monorepo. It used to be a **huge
  full monorepo** (Playground packages, WordPress builds, website, etc.) —
  it has since been trimmed down to just `packages/php-wasm/` for disk
  space (everything else was deleted; see the Environment section below).
- The genuinely relevant file in it was
  `packages/php-wasm/compile/php/Dockerfile`: the Docker recipe that
  compiles PHP (php-src) to WASM via Emscripten, which we had modified
  ourselves on top of upstream Playground:
  - Stripping the opcache JIT sources incompatible with WASM
    (`ext/opcache/jit`), except `minilua.c`, `gen_ir_fold_hash.c`,
    `ir_strtab.c`, `ir_x86.dasc`.
  - Native compilation (gcc, not emcc) of `minilua` and
    `gen_ir_fold_hash` during the build, before the emmake step.
  - A patch to the generated Makefile for PHP 8.5, which otherwise forces
    the presence of the CLI build rule (`sapi/cli/php`).
  - Everything else (libcurl, libxml2, libzip, GD, imagick, asyncify vs.
    JSPI, etc.) came as-is from Playground — see the full file (2762
    lines) for every available `ARG`/flag before rewriting it.
- `packages/php-wasm/compile/*` (next to the Dockerfile) held the
  vendored sources/binaries of each third-party lib (libcurl, libpng16,
  libsqlite3, oniguruma, etc.) referenced by the Dockerfile via `COPY`.

## Decided architecture

Decisions already settled with the user (2026-09-11), to be respected
unless explicitly revisited:

1. **Targeted extraction, not the whole monorepo.** We only vendor what we
   need from `php-wasm-builder` (the `compile/php` Docker context + the
   third-party libs it references), not the entire Playground pnpm
   monorepo. No submodule/dependency on the full monorepo.
2. **Node.js orchestrator**, consistent with the rest of the Kirigami
   ecosystem (already Node/pnpm) rather than PHP.
3. **Architecture model inspired by
   [php-static-autobuilder](https://github.com/ZmotriN/php-static-autobuilder)**
   (the user's personal project, static PHP for Windows) — not its code,
   its **organization**:
   - a central orchestrator driving the whole build (`phpsab.php` there,
     Node here),
   - a `matrix.json`-like file listing the combinations to build (PHP
     versions × extension sets),
   - a `patches/` folder per PHP version rather than inline patches in the
     Dockerfile,
   - a `libraries/` folder (recipes/sources for third-party dependencies),
   - config file(s) (`config.ini` there) describing a build profile
     (target name, enabled extensions, etc.).
4. **Dual build output**: a published npm package (consumed directly by
   Kirigami/kiribuild via `package.json`, as is done today with the fork)
   **and** raw artifacts (.wasm + JS glue) attached to GitHub Releases, in
   the spirit of what php-static-autobuilder already does with its `.exe`
   files.
5. **Two build modes, not just one** (requested by the user, 2026-09-11):
   - **Static**: everything compiled into a single `php.wasm` (what the
     `compile/` pipeline extracted below already does — extensions linked
     in via the Dockerfile's `ARG WITH_*` flags).
   - **Kirigami plugin of type "extension"**: a PHP extension compiled
     separately as a WASM side module (JSPI), loadable at runtime without
     rebuilding the core — so Kirigami plugins can ship their own PHP
     extension, in the spirit of the `"plugin"`/`"task"`/`"command"` types
     already mentioned in `../kirigami/todo.md`.
   - Playground **already** has this tool, published separately on npm:
     `@php-wasm/compile-extension` (`package.json` under
     `packages/php-wasm/compile-extension` in the fork). It takes a source
     directory with `config.m4`, compiles a JSPI WASM `.so` per PHP version
     plus a `manifest.json` (name, ini/env, sidecar files), consumable by
     `@php-wasm/universal` via the startup `extensions` option. It does
     **not** require a checkout of the Playground monorepo: it downloads
     the small subset of Docker assets it needs from the
     `WordPress/wordpress-playground` GitHub tag matching its own npm
     version (see its README for details, CI usage patterns, C/Rust/CMake
     dependency handling, troubleshooting).
   - **Provisional decision: depend on `@php-wasm/compile-extension` as-is
     (npm), do not vendor/fork it** — unlike `compile/` (the core), because
     it's explicitly designed for standalone external use. Unresolved
     concern: its source repository URL is **hardcoded** to
     `WordPress/wordpress-playground` (`compile-extension/src/docker-assets.ts`),
     so it downloads vanilla upstream Docker assets, not our patches
     (JIT/PHP 8.5/minilua). To validate: whether this is an actual problem
     (the Zend/PHP ABI exposed to extensions shouldn't be affected by our
     patches, which touch OPcache JIT and the CLI Makefile, not public
     headers) — otherwise we'll need to fork this package too and point
     `PlaygroundRepositoryUrl` at our own fork.
   - **Distribution/discovery plan for the "extension" mode, clarified by
     the user 2026-09-12**: the game plan is a clean `@kirigami/php-wasm`
     core package plus separately published, on-demand extension packages
     following an `@kirigami/ext-<name>` naming convention (e.g.
     `@kirigami/ext-sodium`) — each one just an npm install away. Once
     installed, it should be **auto-detected and auto-loaded**, no manual
     wiring by the app author. Not yet settled which layer owns that
     auto-detection: either `@kirigami/kirigami` (the framework, scanning
     for installed `@kirigami/ext-*` packages at startup) or
     `@kirigami/php-wasm` itself (the runtime this repo produces, which
     would need its own dependency-scanning logic) — both were floated by
     the user in the same breath, revisit when mode "shared" is actually
     implemented (depends on the `@php-wasm/compile-extension` question
     above being resolved first). **Purpose, stated explicitly**: this is
     what lets a Kirigami plugin (the `"plugin"`/`"task"`/`"command"` types
     in `../kirigami/todo.md`) depend on a specific PHP extension it needs
     — install `@kirigami/ext-sodium`, and a plugin using `sodium_*()`
     calls just works, without every Kirigami app having to carry every
     possible extension in its core `php.wasm`.
6. **JSPI only, no Asyncify** (decided by the user, 2026-09-11). Playground
   supports two variants for each lib/PHP: Asyncify (works in every
   browser, but adds overhead) and JSPI (JavaScript Promise Integration —
   faster, needs recent browsers). We only build/maintain JSPI. Concrete
   impact:
   - `compile/build.js`: the `WITH_JSPI` default must be `'yes'` (changed).
   - `compile/Makefile`: the `*_asyncify` targets are still present
     (inherited from the extraction) but unused — to be pruned later once
     we confirm we'll never need them.
   - So when rebuilding libs, we only ever build/commit `*/jspi/dist/`,
     never `*/asyncify/dist/`.
7. **Node.js only, no browser target** (decided by the user, 2026-09-11).
   Playground builds two separate runtimes (`web` and `node`,
   `web-builds/` vs. `node-builds/` folders). Kirigami runs on Node (`kiri`
   CLI, `kiribuild` via GitHub Actions) so the browser runtime isn't
   needed. Concrete impact in `compile/build.js`:
   - The `--PLATFORM` CLI option was **removed**; `platform` is now a
     hardcoded `'node'` constant.
   - `computeOutputDir()` always writes under
     `node-builds/<version>/{jspi,asyncify}` (no more `web-builds/` branch).
   - The `EMSCRIPTEN_ENVIRONMENT` build-arg is always `node`.
   - `platformDefaults.web` was removed; only `all` and `node` (NODEFS,
     MySQL, Imagick enabled) remain.
8. **Must run in GitHub Actions**, not just locally. As of this writing,
   `php-wasm-builder` had **no** GH Actions workflow building php-wasm via
   Docker (checked in `.github/workflows/`) — that build was done by hand.
   That's precisely part of the new work this repo does.
9. **A single `config.yaml` at the repo root, php-static-autobuilder style**
   (requested by the user, 2026-09-11) — all build options live in it (PHP
   versions, extensions, build options, output), instead of `build.js`'s
   CLI `--build-arg` flags. This file also holds the **default values**
   used in silent mode: "default" = "whatever is in the config file", not
   separate defaults hardcoded in JS.
   - Each extension has a `mode: static | shared | off` (see point 5). Only
     `static` is implemented today, for extensions already wired into
     `compile/php/Dockerfile`. `shared` is accepted by the schema (so the
     file doesn't need redesigning later) but the CLI **refuses** to build
     until the mechanism (`@php-wasm/compile-extension`, point 5) is wired
     up — an explicit error, not a silent failure.
   - `config.yaml` also lists, as `mode: off`, the PHP extensions targeted
     for later, php-static-autobuilder `config.full.ini` style (dom, pdo*,
     simplexml, sockets, gmp, ldap, sodium, tidy, intl, etc.) — not wired
     into the Dockerfile today (they need extra configure flags and, for
     some, a new vendored third-party lib). php-static-autobuilder's
     **Windows-only** extensions (win32std, winbinder, wcli, win32ps,
     win32service, com_dotnet) don't apply to a WASM target and were not
     carried over.
10. **Interactive CLI by default, `--quiet` to use the config file values
    as-is** (requested by the user, 2026-09-11) — `compile/cli.mjs` (new
    entry point, replaces direct command-line use of `compile/build.js`):
    - `node compile/cli.mjs` → interactive mode (prompts to review/edit PHP
      versions + enabled extensions), with the option to save changes back
      to `config.yaml`.
    - `node compile/cli.mjs --quiet` → no prompts, uses `config.yaml` as-is
      (CI usage).
    - In both cases, `cli.mjs` validates the config (shared/unknown modes
      rejected, see point 9) then invokes `build.js` (as a subprocess, once
      per PHP version in `php.versions`) with `--build-arg`s computed from
      the YAML.
11. **Version/link matrix per third-party library, php-static-autobuilder
    `matrix.json` style** (requested by the user, 2026-09-11) — a
    `matrix.json` at the repo root records, for each lib under
    `compile/lib*`, the version currently pinned in its Dockerfile and its
    download source (found by grepping each Dockerfile on 2026-09-11):

    | lib | pinned version | source |
    |---|---|---|
    | libcurl | 7.69.1 | curl.haxx.se tarball |
    | libgd | 2.3.3 | GitHub releases |
    | libzip | 1.2.0 **and** 1.9.2 (two builds, `ARG LIBZIP_VERSION`) | libzip.org tarball |
    | libopenssl | 1.1.0h **and** 1.1.1t (two builds, `ARG OPENSSL_VERSION`) | openssl.org tarball |
    | libImageMagick | 7.1.1-39 | GitHub tag archive |
    | libiconv | 1.17 | ftp.gnu.org |
    | libjpeg (turbo) | 3.0.3 | GitHub releases |
    | libavif | 1.3.0 | GitHub tag archive |
    | libaom | 3.13.1 | googlesource archive |
    | libpng16 | 1.6.39 | SourceForge |
    | libxml2 | v2.9.10 (git tag) | gitlab.gnome.org |
    | libwebp | pinned commit `845d547...` | chromium googlesource archive |
    | libsqlite3 | 3.51.0 (`3510000`) | sqlite.org |
    | libz | 1.2.13 | zlib.net |
    | oniguruma | **no pinned version** — clones the default branch on every build | GitHub |

    - `config.yaml` references these libs under `libraries:`, defaulting to
      `version: latest` → **by default we always download the newest known
      version**, the same way `build.js`/`update-php-versions.mjs` already
      does for PHP itself (auto-refresh if `lastRefreshed` is older than
      24h). An explicit `version:` in `config.yaml` pins a specific version
      instead.
    - **Not done yet**: a generic "latest" resolution mechanism per lib
      (like `update-php-versions.mjs` for PHP) — each lib has a different
      release source (direct tarball, GitHub releases, GitHub tags,
      SourceForge, googlesource, gitlab). `matrix.json` currently documents
      the pinned versions + where to check, without automatic scraping —
      to be built lib by lib.
    - **Explicit purpose (2026-09-11)**: when releasing the package
      (GitHub Actions workflow, point 8), the matrix must be refreshed
      first, so the published build always uses the latest known lib
      versions, not stale pinned ones — the same logic as
      `update-php-versions.mjs` for PHP, applied at release time rather
      than on every local build.
    - **oniguruma is a special case**: having no pinned version today, it's
      already "always latest" but non-reproducibly (different result
      depending on the day of the build). To fix eventually by pinning a
      tag, with `latest` handled explicitly like the other libs.
    - **`matrix.json` ships inside the published npm package** (clarified by
      the user, 2026-09-11), not just a repo-dev file. Consequence for the
      release workflow (point 8): a release should only be cut once the
      matrix has been refreshed to the latest known versions AND the full
      build (all libs + PHP) succeeds with that refreshed matrix — i.e. a
      release is a snapshot of "matrix refreshed + build green", not just
      "someone ran the publish command."
12. **Per-dependency hooks/patches, php-static-autobuilder style**
    (mentioned by the user, 2026-09-11 — php-static-autobuilder has a
    `patches/` folder + `create-patches.bat` to apply small per-dependency
    compilation hacks). Playground already does this in a scattered way:
    per-version PHP patches under `compile/php/php*.patch` (see the
    `apply-mysqlnd-patch.sh` point), but nothing equivalent for third-party
    libs (their build hacks are inline in each Dockerfile via
    `sed`/`replace.sh`). **Not done yet**: a generic
    `patches/<lib-or-version>/*.patch` folder + the hook point in each
    Dockerfile to apply them automatically (today only
    `compile/php/Dockerfile` has this kind of mechanism, not the libs).
13. **Node.js >= 24 everywhere, checked automatically** (requested by the
    user, 2026-09-11). `compile/cli.mjs` checks `process.versions.node` on
    startup and exits with a clear error below Node 24. On Windows
    specifically, it also checks (still automatically, via `wsl -l -q` and
    `wsl -d <distro> -- node -v`) that WSL is installed, that it has an
    Ubuntu distribution, and that Node >= 24 is installed inside that
    distro — to keep the local Windows dev environment close to the Linux
    CI runners this project will eventually build on (point 8). These
    checks currently only *verify* and print guidance; they do not
    auto-install anything (installing WSL/a distro/Node would need elevated
    permissions and is a bigger, separate decision). Also checked: Docker
    reachable, GNU Make available on PATH (`build.js` shells out to
    `make base-image`, and `compile/Makefile` drives every lib build, but
    Make isn't preinstalled on Windows — the user installed it separately
    via Chocolatey), and a POSIX shell + coreutils on PATH via `sh -c
    'command -v mkdir rm mv cp'` (`compile/Makefile`'s recipes use
    `mkdir -p`/`rm -rf`/`mv`/`$$(...)` — none of that is native to
    `cmd.exe`; the user flagged that `ls` and friends aren't standard on
    Windows either, they'd installed GNU tools themselves — likely via Git
    for Windows/Git Bash, which is what this session's own shell tool turned
    out to be using all along, not WSL). This machine does have WSL +
    Ubuntu, and the check correctly caught its Node being v20.20.2 — fixed
    by installing nvm and the latest Node inside that distro (2026-09-12).
    Doing so also surfaced a bug in the check itself: nvm only shadows the
    system Node on PATH via lines appended to `.bashrc`, and `.bashrc`
    itself starts with a guard clause that returns immediately for
    non-interactive shells (the standard Debian/Ubuntu skeleton) — so
    neither a plain `wsl -- node -v` nor `wsl -- bash -lc node -v` picks up
    nvm's Node; both keep reporting the stale system one (apt's
    `/usr/bin/node`). Fixed by having `checkWindowsWslStack()` explicitly
    source `$HOME/.nvm/nvm.sh` before running `node -v`, same as nvm's own
    non-interactive-use instructions.
14. **All project content in English, conversation in French** (stated by
    the user, 2026-09-11): "C'est un projet en anglais, MAIS NOUS Toi et moi
    on se parle en français." Code, comments, CLI text, error messages, and
    documentation files (`CLAUDE.md`, `README.md`, `NOTICE.md`,
    `patches/README.md`) are written in English; the live conversation with
    the user stays in French. See the `feedback-language-convention` memory
    for the durable version of this rule.
15. **`README.md` follows the Kirigami ecosystem's README convention**
    (requested by the user, 2026-09-11 — "vu qu'on est dans l'écosystème
    Kirigami, tu peux suivre les règles de ../kirigami, comme: comment les
    readme sont faits"), modeled on `../kirigami/packages/php-wasm/README.md`
    (the closest analog — also GPL-2.0-or-later, also a php-wasm build):
    centered `<div align="center">` header with the Kirigami logo, title,
    one-line bold tagline, shields.io badges (license, Node version,
    website), `---` rules between sections, a `Table of contents`, and
    closing `License`/`Author` sections. `../kirigami/packages/php-wasm/`
    is worth checking again once this repo actually publishes a package —
    it already contains a real, previously-built `@kirigami/php-wasm`
    (PHP **8.5.10**, JSPI, Node-only) with its own README documenting the
    exact consumer-facing shape (`index.js`, `runtime/runtime.js`,
    `jspi/php_8_5.js`, `jspi/8_5_10/php_8_5.wasm`) this repo's output should
    probably match. `config.yaml`'s `php.versions` was updated to `'8.5'`
    accordingly (was `'8.3'`).
16. **Target PHP version: 8.5** (confirmed explicitly by the user,
    2026-09-12: "On fait php 8.5 btw"). Matches the existing
    `@kirigami/php-wasm` build referenced in decision 15. `config.yaml`'s
    `php.versions` is `['8.5']`; `compile/php/php8.5.patch` (extracted from
    Playground) already covers the PHP-8.5-specific source patch, and the
    `compile/php/Dockerfile`'s own PHP-8.5 Makefile-CLI fix (see Context
    section) applies here too.
17. **OpenSSL upgraded to 3.6.4, single version** (requested by the user,
    2026-09-12: "tu devrais builder la dernière version de openssl avant
    non?"). Upstream Playground pinned two old versions in parallel
    (1.1.0h/1.1.1t) to support a range of PHP versions; since we only
    target PHP 8.5 (decision 16) there's no need for two. Checked
    php-static-autobuilder's own `matrix.json` for precedent first — it
    pins OpenSSL to `3.0.14` (the long-term-support 3.x branch) and
    `libssh2` to `1.10.0`, both via `download_url`/`install_script` entries
    under its top-level `libraries` array (distinct from `extensions`).
    GitHub tags for `openssl/openssl` show `4.0.x` already exists but is
    very new (an alpha tag is even newer); went with **3.6.4**, the latest
    *stable* 3.x release, as a middle ground between "latest" and
    "proven" — verified with a standalone `docker build` that it
    cross-compiles with the existing `compile/libopenssl/Dockerfile`
    **unmodified** (the only failure, the Linux-only `afalg` hardware
    engine module, was already tolerated by the Dockerfile's existing
    `|| true`, and happens after `libssl.a`/`libcrypto.a` are already
    copied out). Changed: `compile/Makefile` (single `OPENSSL_VERSION :=
    3.6.4` variable, one build instead of two), `compile/build.js`
    (`WITH_OPENSSL_VERSION` default `3.6.4`), `config.yaml`
    (`build.openssl_version: '3.6.4'`), `matrix.json`. Old `1.1.0h`/`1.1.1`
    dist artifacts deleted from disk.
    - **Follow-up requested, not done yet**: "pis on va faire aussi libssh2
      pis toute" (add `libssh2` too, and more) — `libssh2` isn't vendored
      under `compile/` at all yet (no Dockerfile, no Makefile targets, not
      wired into `libcurl/Dockerfile`'s `--with-openssl`-style flags via a
      new `--with-libssh2`). Needs its own `compile/libssh2/Dockerfile`
      (depends on libz + our new libopenssl 3.6.4), Makefile targets, a
      `matrix.json` entry (php-static-autobuilder's pin: `1.10.0`, itself
      dated — check for newer), and a `libssh2` entry in `config.yaml`'s
      `libraries:`. Do this once the current JSPI libs build is green with
      the new OpenSSL, not mid-build.
18. **Fixed a real bug surfaced by the user's own config.yaml edit**
    (2026-09-12): the user set `dom: { mode: static }` directly in
    `config.yaml`, which `cli.mjs`'s `validateExtensions()` would have
    rejected — `dom` isn't in `IMPLEMENTED_EXTENSIONS` because it has no
    Dockerfile flag of its own. Checking `compile/php/Dockerfile`'s
    "Add Libxml2 if needed" block confirmed `--enable-dom
    --enable-simplexml --enable-xmlreader --enable-xmlwriter` are all
    bundled unconditionally into the `WITH_LIBXML=yes` branch — Playground
    never exposes them as independent flags. Fixed by adding a
    `BUNDLED_WITH_LIBXML` list in `cli.mjs`: these four extensions must
    always have the same `mode` as `libxml` (enforced, clear error if not),
    and `config.yaml` now sets all four to `static` to match `libxml`. This
    is a real design fix, not a stopgap — the same bundling issue likely
    applies to some of the still-`off` extensions too (worth re-checking
    against the Dockerfile before assuming any of them needs a *new* flag).
19. **Fixed a real Windows bug in `compile/build.js`** (2026-09-12,
    found while attempting the first real — not `--dry-run` — PHP build):
    `sourceDir` was computed as `path.dirname(new URL(import.meta.url).pathname)`,
    which on Windows keeps a leading `/` before the drive letter
    (`/C:/projects/...`). That broken path was passed as `cwd` to every
    `spawn()` call in the file (`make base-image`, the PHP `docker build`,
    the extraction `docker run`), and silently made Node's `make` lookup
    fail with `ENOENT` even though `checkMake()` had already confirmed
    `make` was on PATH (that check has no `cwd` issue, so it passed fine —
    the two are not inconsistent, `build.js` was just wrong on its own).
    Fixed by switching to `fileURLToPath(import.meta.url)` (Node's `url`
    module), which `cli.mjs` was already using correctly. Lesson: on
    Windows, never derive a filesystem path from `new URL(...).pathname`
    directly — always go through `fileURLToPath()`.
20. **Fixed two more real bugs found during the first real PHP build**
    (2026-09-12): `compile/php/Dockerfile` unconditionally
    `COPY ./${EMSCRIPTEN_ENVIRONMENT}-builds/ ...` (i.e. `node-builds/`,
    to scan already-built loadable extensions there for the decision-5
    shared-extension mechanism) and `COPY ./compile/shared/ ...` (Playground's
    optional shared-extension metadata, which this repo doesn't vendor —
    see decision on `compile-extension/`). Docker's `COPY` fails outright
    if the source directory doesn't exist at all (not just if it's empty),
    and neither existed on a fresh checkout — `node-builds/` is gitignored
    build output, `compile/shared/` was never created since we don't vendor
    Playground's shared extensions. Fixed: `compile/build.js` now
    `fs.mkdirSync(..., { recursive: true })` both (possibly empty) right
    before invoking `docker build`, and `compile/shared/.gitkeep` was
    committed so the folder exists in the repo even though it's empty.
21. **Queued, not started**: "on fera une commande bin pour le plugin...
    pour le package acuse[sic]" (user, 2026-09-12, likely a `bin` CLI entry
    point for the eventual `@kirigami/php-wasm`-consuming plugin/package —
    exact scope still unclear, follow up with the user before implementing).
22. **`config.yaml`'s extension set aligned to match the existing, known-working
    `@kirigami/php-wasm` build exactly** (requested by the user, 2026-09-12:
    "Fis toi au package @kirigami/php-wasm" / "Il faut que l'interface reste
    la même"). Diffed our config against the exact `Configure Command` string
    recorded in that package's own `phpinfo()` dump
    (`../kirigami/packages/php-wasm/README.md`, decision 15). Found and fixed:
    - **Real config mismatches** — the reference build has
      `--disable-opcache`, `--disable-soap`, `--disable-fileinfo`,
      `--disable-mbregex`, and no mysql/mysqli/pdo_mysql flag at all. Our
      `config.yaml` had `opcache`/`soap`/`fileinfo`/`mbregex`/`mysql` all set
      to `static` — flipped all five to `off` to match.
    - **Another bundling bug, same shape as decision 18**: the reference
      build's `--with-sqlite3 --enable-pdo --with-pdo-sqlite` confirms
      `pdo`/`pdo_sqlite` are bundled with `sqlite` in
      `compile/php/Dockerfile` (WITH_SQLITE=yes block), exactly like
      dom/simplexml/xmlreader/xmlwriter are bundled with libxml. Generalized
      `cli.mjs`'s single-purpose `BUNDLED_WITH_LIBXML` array into a
      `BUNDLED_EXTENSIONS` map (extension → host extension) covering both
      groups, and moved `pdo`/`pdo_sqlite` from the "off, not wired" list to
      `static` (matching `sqlite`).
    - **`ws_networking_proxy` confirmed intentionally `off`, not a gap to
      fill**: the user pointed out `@kirigami/php-wasm` already has a
      networking runtime — its own `runtime/runtime.js`
      (`getPHPRuntimeWithNetwork()`) proxies sockets on the **Node side**
      (SOCKFS interception via `node:http`/`node:net`), independent of the
      WASM-side `-lwebsocket.js`/`wasm_recv` mechanism `WITH_WS_NETWORKING_PROXY`
      controls. So there's no need to chase the `wasm_recv` dead-code-elimination
      bug found while smoke-testing (see "Current status" below) — just
      leave this flag off.
    - **Explicitly NOT abandoning the broader extension matrix ambition**:
      the user confirmed ("il faut garder ma matrice comme php-auto-builder
      pour que toutes les extensions possibles de pecl fonctionnent") that
      the long `mode: 'off'` list in `config.yaml` (dom/simplexml/etc. were
      here before being reclassified; pdo_mysql, pgsql, sockets, gmp, ldap,
      sodium, tidy, intl, etc. remain) is intentional groundwork toward
      eventually toggling every extension php-static-autobuilder's own
      `matrix.json` covers (~109 extensions, many PECL) — matching the
      reference build now is about interface parity for *this* build, not
      a narrowing of future scope.
    - `cli_sapi`, `nodefs`, `libzip`, `curl`, `iconv`, `libxml`, `exif`,
      `gd`, `mbstring`, `openssl`, `imagick` were already correctly `static`
      and needed no change (either confirmed present in the reference
      Configure Command, or — for `cli_sapi`/`nodefs` — invisible to
      Configure Command by construction since they only affect emcc/JS
      link flags, not `.php-configure-flags`; left as-is, unproven either
      way but not contradicted by any evidence).
23. **Removed a real, unconditional bug from `compile/php/Dockerfile`**
    (2026-09-12), found by actually smoke-testing the built `php_8_5.js` in
    Node (`init()` threw `ReferenceError: _wasm_recv is not defined`, every
    time, for every Node build). Root cause (comment right above the
    removed block, `compile/php/Dockerfile` ~line 2678): upstream Playground
    injects a `wasm_recv: _wasm_recv` alias into `wasmImports` so a
    dynamically-loaded **memcached** shared extension can resolve that
    import — unconditionally for every `EMSCRIPTEN_ENVIRONMENT=node` build,
    regardless of whether memcached is anywhere in the picture. This
    project doesn't vendor memcached (out of scope, see the "Deliberately
    not copied" list) and handles networking Node-side via the consuming
    package's own runtime (SOCKFS proxy, per decision 22), never via a
    WASM-side `wasm_recv` — so the alias was pure dead weight that also
    happened to crash every build. **Correction to decision 22**: that
    entry originally assumed this bug was tied to `WITH_WS_NETWORKING_PROXY`;
    it is not, the injection is unconditional on `EMSCRIPTEN_ENVIRONMENT`
    alone and would have broken the build regardless of that flag. Removed
    the block entirely rather than guarding it, since we have no use for it;
    re-add (correctly guarded on `_wasm_recv`'s actual existence) if
    memcached ever gets vendored.
24. **`libssh2` added and wired into `libcurl` (curl SFTP/SCP support)**
    (requested by the user, 2026-09-12, repeatedly: "pis on va faire aussi
    libssh2 pis toute" → "libssh2 pis l'intégrer à libcurl") — first concrete
    deliverable of a much larger ask ("fait les dockerfiles pour toutes les
    libs et extensions de php-static-autobuilder, avec check pour la
    dernière version"). That full ask (55 libraries + 109 extensions in
    php-static-autobuilder's own `matrix.json`) is far too large to do with
    real per-library testing in one session — every library added tonight
    took genuine debugging (see decisions 6-23). Scoped this session's real
    work to: (a) `matrix.json`'s `plannedLibraries` section, recording
    gh-api-*verified* (not guessed) upstream repos + current latest versions
    for every tractable php-static-autobuilder library not yet wired into a
    `compile/<lib>/Dockerfile`, so future sessions don't have to
    re-research sources; (b) `compile/update-lib-versions.mjs`, a
    `update-php-versions.mjs`-style checker for GitHub-hosted libraries
    (release or tag based) — the "check for the latest version" part of the
    ask, scoped to GitHub sources for now; (c) actually building and testing
    `libssh2` end to end, as the one library explicitly named more than
    once.
    - `compile/libssh2/Dockerfile`: new, downloads the GitHub release
      tarball (ships a pre-generated `configure`, unlike libwebp/libxml2/
      oniguruma), `--host wasm32-unknown-emscripten` cross-compile flag,
      links against our libz + libopenssl 3.6.4.
    - **Real bug found and fixed, not libssh2-specific**: libssh2's
      autoconf-generated dependency check does a generic `-L/root/lib/lib`
      link test, which picked up `libssl.so`/`libz.so` (shared objects that
      happen to sit alongside the `.a` static ones in `libopenssl`'s and
      `libz`'s dist directories — OpenSSL's/zlib's own `make install`
      produces both) instead of the static archive. `wasm-ld` refuses this:
      "attempted static link of dynamic object". Every other lib Dockerfile
      dodges this by referencing the exact `.a` path directly rather than a
      generic `-l` search, so this had been a **latent, unnoticed bug across
      the whole build** (confirmed stray `.so`/`.so.*` files sitting in
      libz's, libopenssl's, libpng16's, libsqlite3's, and libwebp's dist
      trees) — libssh2 is just the first thing that actually tripped over
      it. Fixed generally, not as a one-off: `compile/Makefile` now has a
      `STRIP_SO` macro (`rm -f $(1)/lib/*.so $(1)/lib/*.so.*`), called right
      after `libz_jspi` and `libopenssl_jspi` populate their dist dirs —
      those two are the only libs actually consumed as a *build-time
      dependency* by other lib Dockerfiles (via `COPY ./libz/` /
      `COPY ./libopenssl/`), so they're the only ones where this ambiguity
      can bite a downstream consumer. (Diagnosed by inspecting libssh2's
      real `configure`/`config.log` inside a throwaway debug image — several
      earlier guesses, PKG_CONFIG_PATH and PKG_CHECK_MODULES-style
      `LIBSSL_CFLAGS`/`LIBSSL_LIBS` overrides, turned out not to be the
      actual cause and were removed from the Dockerfile again once the real
      fix was found, per "garde le code bien propre.")
    - `compile/libcurl/Dockerfile`: added `COPY ./libssh2/` + the matching
      `cp`, `--with-libssh2=/root/lib` on curl's `configure`, and `-lssh2`
      added to `EMCC_SKIP` on the JSPI link step. Confirmed via the build
      log: `checking for libssh2_channel_open_ex in -lssh2... yes`, and
      curl's `vssh/libcurl_la-libssh2.lo` actually compiles.
    - `compile/php/Dockerfile`: added `COPY ./compile/libssh2/
      /root/builds/libssh2` next to the other libs (the existing
      `find /root/builds -path '*/jspi/dist/root/lib' ...` sweep picks it up
      automatically, no special-casing needed) and added
      `/root/lib/lib/libssh2.a` to `.emcc-php-wasm-sources` in the
      `WITH_CURL` block, alongside `libcurl.a`.
    - **Also removed Asyncify entirely from `compile/Makefile`** while in
      there (requested repeatedly: "on veut pas de asyncify" /
      "on en a rien a foutre du asyncify") — decision 6 already said we
      only build JSPI, but the `*_asyncify` targets, the commented-out
      libncurses/libedit blocks (explicitly Asyncify-only, per their own
      comment), and every `all_asyncify`/`clean-*` reference to them were
      still sitting in the file. Deleted rather than commented out; the
      leftover `*/asyncify/dist/` directories on disk (~80MB) were deleted
      too. `all_jspi` is now just `all`.
    - **Not done**: the other ~50 libraries and ~100 extensions from
      php-static-autobuilder's matrix — see `matrix.json`'s
      `plannedLibraries` for what's already researched and ready to pick up.

25. **`nghttp2` added (curl HTTP/2 support) + targeted lib builds (only
    build what enabled extensions actually need) + a real base-image caching
    bug fixed** (2026-09-12, three related changes from the same underlying
    ask: "il y aura une commande qui va actualiser toutes les versions" plus
    "dans php-auto-build, il y avait aussi chaque deps pour chaque extension
    pour ne pas avoir à tout compiler à chaque build").
    - **`nghttp2`**: curl's own `configure` summary confirmed
      `HTTP2: disabled (--with-nghttp2)` — the `libnghttp2` seen in build
      logs is just an apt package pulled in by the base Ubuntu image's own
      tooling, never linked into the WASM cross-compile. Added
      `compile/nghttp2/Dockerfile` (release tarball, pre-generated
      `configure`, `--enable-lib-only` to skip the CLI apps/examples that
      pull in unrelated deps like libcurl/libev/jansson/boost), a
      `nghttp2_jspi` Makefile target, wired into `libcurl/Dockerfile`
      (`COPY`, `--with-nghttp2=/root/lib`, `-lnghttp2` in `EMCC_SKIP`) and
      `compile/php/Dockerfile`'s `WITH_CURL` `.emcc-php-wasm-sources` line
      (`libnghttp2.a` alongside `libcurl.a`/`libssh2.a`). Moved from
      `matrix.json`'s `plannedLibraries` into `libraries` proper.
    - **`libcurl`'s own version bump was explicitly rejected, for now**: the
      same conversation surfaced that `compile/libcurl/Dockerfile`'s
      `ARG CURL_VERSION="curl-7.69.1"` had a stale hardcoded default nothing
      ever overrode (curl's real latest, verified via `gh api`, is 8.22.0 —
      5+ years newer). Fixed the *mechanism*, not the version: removed the
      hardcoded default (`ARG CURL_VERSION` with no default, matching
      `libopenssl`/`libzip`'s convention of always requiring an explicit
      `--build-arg`), added a `CURL_VERSION` Makefile variable (still
      `7.69.1`, the known-working pin), and switched `matrix.json`'s
      `libcurl` entry from `sourceType: "tarball"` to `"github-release"`
      (repo `curl/curl`) so `update-lib-versions.mjs` can track it —
      **correction made mid-thread**: `update-lib-versions.mjs` always
      *appends* newly-discovered versions rather than holding them back, so
      `matrix.json` must stay an honest mirror of upstream
      (`versions: ["7.69.1", "8.22.0"]`, `latest` = 8.22.0) — the actual
      build pin lives only in `compile/Makefile`'s `CURL_VERSION` variable
      and `config.yaml`'s explicit `{ version: 7.69.1 }` (not `latest`),
      independently of what the matrix says is newest. Bumping the real
      build to 8.22.0 needs its own dedicated cross-compile test pass
      first (5+ years of curl changes — new/removed configure flags,
      possible autoconf behavior differences), same reasoning as the
      already-documented libpng/webp/jpeg/avif/xml2 "deliberately not
      bumped" note. Also deleted three fully-dead Asyncify-only `RUN` blocks
      from `libcurl/Dockerfile` while touching it (unreachable in JSPI-only,
      carried a stale `curl-7.69.1` path themselves).
    - **Targeted lib builds**: added `LIB_TARGETS_BY_EXTENSION` +
      `computeRequiredLibTargets()` + `runLibBuild()` in `compile/cli.mjs`,
      mirroring php-static-autobuilder's per-extension dependency list.
      Extension → Makefile leaf target (e.g. `curl` → `libcurl_jspi`; only
      the leaf is listed, `compile/Makefile`'s own prerequisite chain pulls
      in the rest — `libcurl_jspi` already depends on `libz_jspi`,
      `libopenssl_jspi`, `libssh2_jspi`, `nghttp2_jspi`). Extensions with no
      vendored lib of their own (`cli_sapi`, `fileinfo`, `soap`, `exif`,
      `mbstring`, `ws_networking_proxy`, `opcache`, `mysql`, `nodefs` — all
      compile from php-src's own bundled sources) have no entry and need
      none. `cli.mjs` now runs `make <only the needed targets>` itself
      before invoking `build.js`, instead of requiring a separate manual
      `make all_jspi` first — closes the gap noted in "Current status"
      below ("not wired up yet"/"config.yaml's libraries: section... not
      wired up"). Two harmless pre-existing orphans found and *not* added to
      the map (deliberately excluded, not missed): `libzip`'s 1.2.0 build
      (php/Dockerfile's "Add Libzip" block only ever reads the 1.9.2 one)
      and `libaom` (`libavif`'s own Dockerfile doesn't reference it at all —
      copied into `/root/builds` by `compile/php/Dockerfile` but never in
      any `.emcc-php-wasm-sources` line, so never actually linked into
      `php.wasm`). Neither breaks anything; both are just wasted build time
      in a full `make all_jspi`, now skipped by the targeted path.
    - **Real bug found while testing this**: `compile/Makefile`'s
      `base-image: base-image/.ready` recipe never actually created
      `base-image/.ready` — a pre-existing quirk noted in the Environment
      section below as "harmless" during the post-wipe full-rebuild
      (`make all_jspi` from empty Docker data, where rebuilding everything
      was the desired behavior anyway). It is **not harmless** for targeted
      builds: GNU Make treats a prerequisite whose target file never gets
      created as "just rebuilt" on every invocation, and since literally
      every lib target depends on `base-image`, this cascaded into
      rebuilding *every* lib on *every* `make` call regardless of whether
      its own `.a` already existed — silently defeating the entire point of
      `LIB_TARGETS_BY_EXTENSION`. Caught live: `make libcurl_jspi` (to test
      nghttp2) unexpectedly recompiled all of OpenSSL from scratch. Fixed by
      adding `touch base-image/.ready` at the end of the recipe (plus
      `base-image/Dockerfile` as an explicit prerequisite, so it still
      rebuilds for real when that file changes) and a `clean-base-image`
      target to force one manually. `compile/base-image/.ready` added to
      `.gitignore`.
    - **Also renamed stray `php-wasm`/`php-wasm-tmp` Docker image/container
      names to `kirigami-php-wasm`/`kirigami-php-wasm-tmp`** in
      `compile/build.js` (spotted by the user in Docker Desktop's image
      list) — every other image in this pipeline already follows the
      `kirigami-php-wasm:<lib>` convention (see `compile/Makefile`); this
      was the one leftover from the Playground extraction still tagged
      `php-wasm` bare.

26. **"Always try latest first" adopted as the explicit default policy**
    (user, 2026-09-12, correcting the caution in decision 25's curl entry:
    "toujours essayer en premier les dernières versions des libs"). Tested
    live rather than just changed on paper — one lib at a time (sequential,
    not parallel, per the user's own follow-up clarification, to avoid
    repeating tonight's RAM/disk crisis — see Environment section):
    - **Succeeded and adopted**: `libpng16` 1.6.39 → 1.6.58, `libavif` 1.3.0
      → 1.4.2, `libxml2` v2.9.10 → v2.15.4, `libwebp` (commit pin) →
      v1.6.0 (moved off the commit pin to a real tag — confirmed
      `libsharpyuv.a` still produced alongside `libwebp.a`), `libImageMagick`
      7.1.1-39 → 7.1.2-31 (rebuilt cleanly against the fresh
      libpng/libjpeg/libwebp above), and `curl` 7.69.1 → 8.22.0 — needed one
      real fix, `--without-libpsl` on curl's `configure` (curl 8.x added a
      hard dependency on `libpsl`, the Public Suffix List cookie-security
      library, which we don't vendor; disabling that one optional feature
      was enough — `HTTP2: enabled (nghttp2)` confirmed still present in the
      build log). See `NOTICE.md` for the full writeup.
    - **Tested and reverted — a real incompatibility, not caution**:
      `libjpeg-turbo` 3.0.3 → 3.2.0 fails to build: 3.2.0 added a bundled
      `src/spng/` (PNG) module using raw x86 SSE2 intrinsics (`__m128i`,
      `_mm_cvtsi128_si32`) that are unconditionally reachable under our
      `-D__x86_64__` compile flag and don't exist under Emscripten without
      `-msimd128`. Stayed on 3.0.3; would need a real patch (disable spng,
      or add SIMD polyfill flags) to move forward. Documented in
      `matrix.json`'s `libjpeg` note rather than silently dropped.
    - **Real bug found and fixed while testing this**:
      `update-lib-versions.mjs`'s `github-tag` fallback couldn't resolve
      `libImageMagick` at all — its regex only accepted plain dotted
      versions, but ImageMagick tags a trailing `-NN` patch counter
      (`7.1.2-31`, not a prerelease suffix — a meaningfully different
      release each time). Fixed `normalizeVersion()` and `latestGithubTag()`
      to accept and correctly numeric-sort the `-NN` suffix instead of
      silently failing with "could not resolve a version" or crashing the
      sort on a `NaN` component.
    - `matrix.json`'s `plannedLibraries`-style "note" field is now used
      consistently on every "always try latest" entry to record what's
      queued vs. tested vs. reverted-with-a-reason — this is deliberately
      *not* the same thing as the version-bump-specific caution language
      decision 25 first used for curl (which this decision supersedes: the
      default is "try it", not "hold back until someone gets to it").
27. **`compile/cli.mjs` gained subcommands** (user asked to "work on the
    CLI" while lib rebuilds ran in the background, 2026-09-12): restructured
    from a single flat command into `build` (the previous default
    behavior — config/quiet/save/dry-run flags unchanged, still the
    default when no subcommand is given) and `update-versions` (wraps
    `update-php-versions.mjs`'s `updatePHPVersions()` and
    `update-lib-versions.mjs`'s `updateLibVersions()` behind one command,
    with a `--write` flag controlling only the library side — PHP versions
    have no dry-run mode of their own and always write). Docker/Make/
    POSIX-tools/WSL environment checks moved out of the shared `main()` and
    into `runBuildCommand()` specifically, since `update-versions` needs
    none of them (just network + local file writes) — only
    `checkNodeVersion()` still runs unconditionally for both commands.
    Uses yargs' `.command()` + `.parseAsync()` (was a single flat
    `.options()` + `.parse()`).
    - **Not done yet, scoped by the user in the same conversation**: a
      further `update-versions`-adjacent command that, after bumping
      `matrix.json`, automatically `make clean-<lib> <lib>_jspi`s only the
      libraries whose version actually changed (plus anything depending on
      them in the Makefile's own prerequisite graph) — "recompile just what
      changed in the chain." Deliberately not built tonight: every real
      version bump attempted this session needed individual judgment (the
      libjpeg 3.2.0 failure above being the clearest example) — a fully
      automatic version-bump-and-rebuild pipeline needs a real answer for
      "what happens when the bumped version fails to build" (auto-revert
      matrix.json? leave it bumped but flag the lib as broken? something
      else?) before it's safe to automate. Also flagged by the user as a
      prerequisite for **versioning the eventual `@kirigami/ext-*` npm
      packages** (decision 5's extension-package distribution plan): an
      extension package's own npm version presumably needs to track when
      the libs/PHP ABI it was compiled against actually changed, which
      needs this same "what changed in the chain" detection. Revisit once
      mode "shared" (decision 5) is actually being implemented.
28. **`matrix.json` wired in as the actual, single source of truth for lib
    versions** (user, 2026-09-12: "le numero de la version de curl devrait
    venir de la matrice non?" — pointing out that `compile/Makefile`'s
    `CURL_VERSION`/`OPENSSL_VERSION` and `config.yaml`'s
    `build.openssl_version` were all hand-synced duplicates of what
    `matrix.json` already recorded, exactly the gap decision 11 originally
    flagged as "not done yet"). New `compile/matrix-version.mjs`:
    `getMatrixVersion(libraryKey)` (a lib's `versions` array, last entry —
    "latest") usable both as an importable function and as a CLI
    (`node matrix-version.mjs <lib>`, for `$(shell ...)` in Make). Wired in
    two places:
    - `compile/Makefile`: new `MATRIX_VERSION` macro
      (`$(call MATRIX_VERSION,someLibraryKey)`); `OPENSSL_VERSION` and
      `CURL_VERSION` now both resolve from it instead of a hardcoded literal
      — bumping `matrix.json` now directly changes what the next `make`
      actually builds, no separate variable to remember to update.
    - `compile/cli.mjs`: `buildArgsForVersion()`'s `--WITH_OPENSSL_VERSION`
      now defaults to `getMatrixVersion('libopenssl')` instead of a
      hardcoded (and stale — `'1.1.0h'`) fallback; `config.yaml`'s
      `build.openssl_version` field still works as an explicit override
      (removed its hardcoded `'3.6.4'`, now commented as "defaults to
      matrix.json").
    - **Consequence, made explicit**: `matrix.json`'s `versions` array is no
      longer just documentation — its last entry is now literally what gets
      built next. This retroactively firms up decision 26's "always try
      latest first, revert if it breaks" policy: reverting a failed bump now
      means removing/reordering that entry in `matrix.json` itself (as was
      done for the reverted `libjpeg` 3.2.0 attempt, kept in the array but
      not last), not just editing a separate Makefile variable — one
      accurate place instead of two that can drift apart.
    - `config.yaml`'s `libraries.libcurl` was flipped from a caution-driven
      explicit pin (`7.69.1`, from decision 25) back to `latest` now that
      8.22.0 is confirmed working — matching every other library entry's
      default.
29. **`libfreetype` (GD text rendering) added and wired — found half-done,
    completed (2026-09-12).** Picked up a prior session's in-progress work
    that wasn't yet reflected here: `compile/libfreetype/Dockerfile` (new,
    zlib/libpng-only build via CMake/emcmake, no HarfBuzz/Brotli/bzip2 —
    matches what GD actually uses) and `compile/libgd/Dockerfile` (added
    `COPY ./libfreetype/`, `-DENABLE_FREETYPE=ON`/`-DFreetype_LIBRARY=...`
    in the cmake invocation) were both already written, and
    `compile/Makefile` already had `libfreetype_jspi` wired as a real
    prerequisite of `libgd_jspi`. But `compile/php/Dockerfile` — the file
    that actually links everything into the final `php.wasm` — had **not**
    been updated: it was still missing `COPY ./compile/libfreetype/
    /root/builds/libfreetype` (so libfreetype's `dist/root/lib` never
    reached the `find /root/builds -path '*/jspi/dist/root/lib'` sweep that
    populates `/root/lib`, the same sweep pattern decision 20 documents) and
    the `WITH_GD` block's `.emcc-php-wasm-sources` line was missing
    `/root/lib/lib/libfreetype.a` alongside `libgd.a`/`libjpeg.a`/etc. Since
    `libgd.a` is now built with `ENABLE_FREETYPE=ON` and therefore references
    `FT_*` symbols, the final PHP link would have failed with undefined
    symbols the next time someone ran a real (non-`--dry-run`) build — a
    real, if not yet triggered, regression. Fixed both gaps. No PHP-level
    `--with-freetype-dir` configure flag needed: modern PHP's external-GD
    mode calls libgd's own `gdImageStringFT` wrapper, not FreeType directly,
    so this is purely a final-link dependency, same shape as the
    libssh2/nghttp2 wiring in decisions 24-25. Also updated `matrix.json`:
    moved the `freetype` entry from `plannedLibraries` (stale note: "GD
    works without it... would need investigation") into `libraries` proper
    (`sourceType: github-tag`, pinned `2.14.3`, matching the Dockerfile's
    `ARG FREETYPE_VERSION` default) — no `cli.mjs` change needed since
    `LIB_TARGETS_BY_EXTENSION`'s existing `gd: ['libgd_jspi']` entry already
    pulls in `libfreetype_jspi` transitively via the Makefile's own
    prerequisite chain (decision 25's pattern).
30. **Shared extensions (mode: `shared`) investigation started, ABI
    de-risked, full validation in progress (2026-09-12).** User clarified
    the actual goal: "shared" means being able to compile a PHP extension
    **independently**, so it can ship as its own npm package release (the
    `@kirigami/ext-<name>` plan from decision 5), without rebuilding all of
    `php.wasm`. This is decision 5's still-open item — picking it up for
    real. First step: resolve decision 5's "unresolved concern" about
    `@php-wasm/compile-extension` (the npm package we provisionally decided
    to depend on rather than fork) — its `PlaygroundRepositoryUrl` is
    hardcoded to `WordPress/wordpress-playground`, so it always fetches
    *vanilla* upstream Docker assets, never our JIT/CLI-Makefile patches.
    Confirmed via `gh api` against the exact git tag matching the installed
    npm version (`v3.1.53`, package `@php-wasm/compile-extension@3.1.53`):
    - Its fetched `compile/base-image/Dockerfile` pins the exact same
      Emscripten version we do: `4.0.19`. This matters because
      `compile/base-image/Dockerfile`'s own comment says mismatched
      Emscripten versions between a main module and a side module produce
      undefined-symbol errors — same-version match removes that risk
      entirely, not just reduces it.
    - Its fetched `compile/php/php8.5.patch` is **byte-for-byte identical**
      (`diff` exit 0) to our own `compile/php/php8.5.patch`.
    - Combined with decision 5's original reasoning (our own patches touch
      OPcache JIT sources and the CLI Makefile, not public Zend/PHP headers)
      and the fact its `cli.js` hardcodes the exact same PHP release we
      built (`8.5.10`), this is strong evidence depending on the npm package
      as-is (no fork) is safe for PHP 8.5 — the provisional decision in
      decision 5 stands, upgraded from "provisional" to "verified, pending
      one real end-to-end runtime test."
    - Also confirmed from its README: it's JSPI-only (`asyncMode: "jspi"`
      hardcoded) and Rust/CMake/Makefile dependency-vendoring patterns are
      documented (vendor the dep source, build with Emscripten yourself,
      pass `--extra-cflags`/`--extra-ldflags` pointing at `/build/...`
      paths) — relevant later for extensions like `sodium` that need an
      external lib we don't currently vendor.
    - Added `@php-wasm/compile-extension@3.1.53` as a **devDependency** in
      `compile/package.json` (pinned, not a runtime dependency of the
      published package — it's a build-time tool, same category as `yargs`/
      `prompts`).
    - User picked `sodium` as the real pilot extension for the eventual
      `mode: shared` feature (it needs `libsodium`, not yet vendored here),
      but also agreed to validate the raw mechanism first with zero
      dependencies before spending a Docker build on vendoring libsodium.
      Wrote a throwaway `kirigami_abi_probe` extension (scratchpad, not
      committed — trivial `config.m4` + one `PHP_FUNCTION` returning `42`,
      no external library) and kicked off
      `php-wasm-compile-extension --source ... --name kirigami_abi_probe
      --php-versions 8.5 --out ...` in the background to produce a real
      `.so` side module. **Not yet done**: actually loading that `.so`
      against our own `node-builds/8-5/php_8_5.js` runtime and calling
      `kirigami_abi_probe()` — the real end-to-end proof, still pending
      when this was written. `@php-wasm/universal`'s `PHP.load()` extension-
      loading path (README's "Loading the result" section: stages the `.so`,
      writes a startup `.ini`, registers the extension scan directory) is
      the reference behavior to replicate or depend on for whatever ends up
      wiring `mode: shared` into `cli.mjs` — not decided yet whether that
      needs `@php-wasm/universal` as a new dependency or a smaller
      hand-rolled loader given this repo doesn't otherwise depend on it
      (decision 23's runtime smoke test bypassed it entirely).
    - Not yet touched: `cli.mjs`'s `validateExtensions()` still rejects
      `mode: shared` outright (decision 9) — deliberately left as-is until
      the pilot extension actually loads and runs, per the user's "validate
      ABI first" call.
    - **✅ End-to-end validation succeeded (2026-09-12).** Installed
      `@php-wasm/universal@3.1.53` in a throwaway scratchpad test project
      (not added to `compile/package.json` — this is the *runtime* consumer
      side, a decision for the eventual `@kirigami/php-wasm` package itself,
      not for the `compile/` build pipeline). Found the exact API needed and
      confirmed it works against an arbitrary, non-Playground-bundled PHP
      build: `loadPHPRuntime(phpLoaderModule, options)` takes any object
      shaped like `{ init, dependencyFilename, dependenciesTotalSize }` —
      which is exactly what our own `node-builds/8-5/php_8_5.js` already
      exports (no adapter needed) — so `@php-wasm/universal` does **not**
      require going through `@php-wasm/node`'s bundled runtime packages
      (`@php-wasm/node-8-5` etc.) at all. Paired with `resolvePHPExtension()`
      + `withResolvedPHPExtensions()` (also runtime-agnostic, `format: 'so'`
      + raw bytes) to stage the `kirigami_abi_probe-php8.5-jspi.so` built in
      the step above, then ran actual PHP code through the `PHP` class:
      `<?php echo kirigami_abi_probe();` against our own patched
      `php_8_5.wasm` printed **`abi_probe=42`**, and
      `get_loaded_extensions()` listed `kirigami_abi_probe` alongside every
      other real extension — proof of genuine Zend module registration, not
      just a raw successful `dlopen`. Only blemish: a harmless `PHP Warning:
      Missing arginfo for kirigami_abi_probe()` (cosmetic — the toy
      extension's `zend_function_entry` has no `ZEND_BEGIN_ARG_INFO` block;
      unrelated to the ABI question and trivial to fix in a real extension).
      **This closes decision 5's open ABI question for good**: an extension
      built by unmodified upstream `@php-wasm/compile-extension` genuinely
      loads and runs correctly against this repo's own JIT/CLI-Makefile-
      patched, differently-extension-configured `php.wasm` build. No fork of
      `compile-extension` is needed.
    - **Next decisions, not yet made**: (1) whether `@php-wasm/universal`
      becomes an actual dependency of the eventual `@kirigami/php-wasm`
      runtime package (this test strongly suggests yes — it's the only
      piece that knows how to stage a `.so` + generate the startup `.ini`,
      reimplementing that by hand would just be worse), or whether Kirigami
      hand-rolls a smaller loader against the same `resolvePHPExtension`
      shape; (2) how `mode: shared` gets invoked from `cli.mjs` — presumably
      a new subcommand (`node cli.mjs compile-extension <name>`?) that reads
      a `sourceDir` per shared extension from `config.yaml` and shells out
      to `@php-wasm/compile-extension`'s CLI, separately from the main
      `build` subcommand's per-PHP-version loop; (3) the real pilot is still
      `sodium`, which needs `libsodium` vendored + cross-compiled with
      Emscripten first (per the npm package's own "Dependencies" README
      section: vendor the source, build it yourself, pass
      `--extra-cflags`/`--extra-ldflags` pointing at the resulting
      `/build/...` archive) — not started yet.
31. **`mode: shared` wired into `cli.mjs`, with the publish-package shape
    decided up front rather than left for later (2026-09-12).** User asked
    explicitly to think about the eventual published-package architecture
    *while* doing this wiring, not after. Key realization: a shared
    extension's `@php-wasm/compile-extension` output — `manifest.json` + one
    `.so` per PHP version — is **already** everything an `@kirigami/ext-<name>`
    npm package needs to ship (decision 5's naming convention); the consumer
    loads it straight via `@php-wasm/universal`'s
    `{ format: 'manifest', manifestUrl: ... }`, no extra JS glue required. So
    instead of a `node-builds/` staging area that a later, separate
    "packaging" step would promote into a package, the CLI writes real
    extensions **directly** into `packages/ext-<name>/` — that directory
    already *is* the package, one generated `package.json` away from
    `npm publish`. `/packages/` added to `.gitignore` alongside `/node-builds/`
    (same reasoning: `package.json` is fully regenerated from `config.yaml`
    on every run, nothing there is hand-edited, no reason to commit it).
    Concretely:
    - `config.yaml`: shared extensions now need `source` (path to the
      extension's `config.m4` + sources, relative to the repo root) and may
      set `internal: true` for pipeline self-test fixtures that should never
      become a publishable package (their output stays under
      `node-builds/extensions/<name>/` instead of `packages/ext-<name>/`).
      Moved the `kirigami_abi_probe` toy extension from the earlier
      scratchpad test into a committed fixture,
      `compile/extensions/abi-probe/` (`config.m4` + `.c`, with a proper
      `ZEND_BEGIN_ARG_INFO` this time — the earlier run threw a harmless
      "missing arginfo" warning), declared in `config.yaml` with
      `internal: true`.
    - `cli.mjs`: `validateExtensions()` no longer rejects `mode: shared`
      outright — it now requires a `source` string instead, checked *before*
      the `IMPLEMENTED_EXTENSIONS` lookup (shared extensions are never in
      that map — it's Dockerfile-flag bookkeeping for `mode: static` only).
      New `compile-extension [name]` subcommand (no name = every `mode:
      shared` extension) resolves `compile/node_modules/.bin/php-wasm-compile-extension`
      (installed as a devDependency, decision 30) and shells out to it once
      per extension with `--source`/`--name`/`--php-versions` (from
      `config.php.versions`, same list the main build uses)/`--out`, plus
      optional passthrough `extraCflags`/`extraLdflags`/`configArgs` config
      fields for later dependency-vendoring extensions (sodium and friends).
      For non-internal extensions, also writes a minimal but real
      `package.json` (`name: "@kirigami/ext-<name>"`, `version: "0.1.0"`
      placeholder — see decision 27's still-unsolved versioning question,
      `license: GPL-2.0-or-later`, `files: ["manifest.json", "*.so"]`) into
      the same output directory right after the compile succeeds. Supports
      `--dry-run` like the `build` command.
    - **Real bug found and fixed while testing this**: `runCommand()`'s
      `spawn()` call threw `EINVAL` on Windows the first time it actually
      ran (not on `--dry-run`) — npm's `.bin/` wrappers are `.cmd` shims on
      Windows, and Node's `child_process.spawn()` needs `shell: true` to
      execute those directly (a plain path to a `.cmd` file isn't a
      directly-executable image as far as Windows' `CreateProcess` is
      concerned). Fixed with `shell: process.platform === 'win32'`. The
      existing `runBuild()`/`runLibBuild()` spawns (`node`, `make`) never hit
      this because those are real executables, not shell shims.
    - **Verified end-to-end twice**: once with `internal: true` (the
      committed fixture, output under `node-builds/extensions/`, no
      `package.json`), once with a throwaway `internal: false` config copy
      (output under `packages/ext-kirigami_abi_probe/`, real `package.json`
      written, directory deleted afterward — it was only a wiring test, not
      meant to be kept). Both times, re-ran the same `@php-wasm/universal`
      loader script from decision 30 against the freshly-rebuilt `.so` and
      got `abi_probe=42` again, this time with **no** arginfo warning.
    - **Still not done**: no `@kirigami/php-wasm` (core, `mode: static`)
      package assembly exists yet — this decision only closes the gap for
      shared extensions, which happened to need much less machinery (no JS
      glue, decision 15's core package shape — `index.js`, `runtime/runtime.js`
      — doesn't apply here). The `sodium`/libsodium pilot is still not
      started. Extension-package versioning (decision 27) is still an open
      placeholder (`0.1.0` always). No GitHub Actions wiring yet for
      publishing `@kirigami/ext-<name>` packages.
32. **`sodium` pilot: libsodium vendored and cross-compiled successfully;
    the real ext/sodium build compiles and links, but exposed a genuine,
    unresolved bug in this repo's own `php.wasm` — complex side modules
    fail to *load* at runtime, not just to build (2026-09-12).**
    - **libsodium vendored**: `compile/libsodium/Dockerfile` (new; release
      tarball ships a pre-generated `configure`, same shape as libssh2 —
      decision 24), `LIBSODIUM_VERSION` resolved from `matrix.json` via the
      decision-28 `MATRIX_VERSION` macro (no hardcoded default — the user
      corrected an initial draft that had one, mid-session: "aucune version
      encodé svp... toujours la dernière... celle de la matrice qui sera
      mise à jour"). `matrix.json`'s `libsodium` entry moved from
      `plannedLibraries` to `libraries` proper (was already gh-api-verified
      at `1.0.22` from an earlier session).
    - **Real bug found and fixed, not sodium-specific in principle**:
      first build attempt failed with `error: invalid input constraint 'q'
      in asm` in `ed25519_ref10.c`. Root cause: unlike every other lib
      Dockerfile in this repo, libsodium's `sodium/runtime.c` (`_cpuid()`)
      and `ed25519_ref10.c`/`sodium/core.c` gate **real inline `__asm__`
      blocks** (literal `cpuid`, x86 register constraints like `"q"`) behind
      `defined(__x86_64__)` — not intrinsics Emscripten can lower to wasm
      SIMD the way libjpeg/libwebp's code does. This repo's pipeline-wide
      `-D__x86_64__` (used everywhere else to unlock legitimate SIMD-
      intrinsics fast paths) just makes libsodium think it's really running
      on x86_64 and pulls in asm with no wasm equivalent. Fixed by omitting
      `-D__x86_64__` specifically in `compile/libsodium/Dockerfile` (every
      other lib keeps it) — without it, libsodium correctly falls through
      to its portable `ref`/`ref10` implementations. `make libsodium_jspi`
      then succeeded cleanly (`libsodium.a`, ~487KB).
    - **`compile/extensions/sodium/`**: vendored php-src's real `ext/sodium`
      files as-is from the `PHP-8.5.10` tag (`config.m4`, `libsodium.c`,
      `sodium_pwhash.c`, `php_libsodium.h`, arginfo headers, stub.php files)
      — not rewritten, so the extension's actual PHP-facing API matches
      upstream exactly. `config.yaml`'s `sodium` entry moved from the
      long `mode: off` PECL-style list (decision 9) to the shared-extensions
      section, with two new fields generalizing the vendored-dependency
      mechanism beyond the abi-probe fixture:
      - `vendorLib: libsodium` — tells `cli.mjs compile-extension` to copy
        `compile/libsodium/jspi/dist/root/lib/` into
        `compile/extensions/sodium/vendor/libsodium/` before compiling
        (mirrors the README's own "build the dependency yourself, place
        headers/.a under the extension source, pass /build/... paths"
        pattern) and derive `--extra-cflags`/`--extra-ldflags` from it
        automatically (`-I/build/vendor/libsodium/include`,
        `/build/vendor/libsodium/lib/libsodium.a`). New
        `compile/extensions/*/vendor/` gitignore entry — regenerated fresh
        every run, nothing there is hand-edited.
      - `pkgConfigVar: LIBSODIUM` — ext/sodium's own `config.m4` calls
        `PKG_CHECK_MODULES([LIBSODIUM], [libsodium >= 1.0.8])`, which would
        fail inside the build container (no real pkg-config/`libsodium.pc`
        there for a vendored lib). Per pkg.m4's own documented behavior,
        `PKG_CHECK_MODULES` skips calling pkg-config entirely when
        `<VAR>_CFLAGS`/`<VAR>_LIBS` are already set — so `cli.mjs` passes
        those two as `--config-args` (autoconf accepts bare `VAR=value`
        arguments as environment overrides). Chosen over patching
        ext/sodium's `config.m4` itself, to keep the vendored extension
        source byte-identical to upstream.
    - **Two real Windows `spawn()` bugs found and fixed while wiring this**:
      (1) `EINVAL` calling the compile-extension `.cmd` shim directly — same
      shape as decision 31's fix, needs `shell: true`; (2) once `shell: true`
      was in place, a `--config-args` value containing a space (two
      `VAR=value` tokens) reached `cmd.exe` **unquoted** and got split into
      two separate CLI arguments (`yargs` on the other end reported
      `Unknown argument: LIBSODIUM_LIBS=...`) — unlike the non-shell Windows
      spawn path, Node does not auto-quote arguments for `shell: true`.
      Fixed by having `runCommand()` manually wrap any arg containing a
      space or quote in `"..."` (escaping embedded quotes) before invoking,
      Windows-only.
    - **`ext/sodium` compiles and links successfully** once the above was in
      place: `node compile/cli.mjs compile-extension sodium` produced
      `packages/ext-sodium/sodium-php8.5-jspi.so` (~381KB, both
      `libsodium.c` and `sodium_pwhash.c` compiled, `libsodium.a`
      force-linked via `--whole-archive`/`--no-whole-archive`, matching the
      README's documented static-archive handling).
    - **❌ Fails to *load* at runtime — a real, unresolved gap in this
      repo's own `php.wasm`, not in the sodium extension or in
      `@php-wasm/compile-extension`.** Re-ran the decision-30 loader script
      against `sodium-php8.5-jspi.so`: `WebAssembly.Instance(): Import #47
      "env" "__stack_pointer": imported mutable global must be a
      WebAssembly.Global object`. Diagnosed concretely, not guessed:
      - Inspected both `.so` files' imports directly via
        `WebAssembly.Module.imports()`: `kirigami_abi_probe.so` (decision
        30/31's simple fixture) imports only `env.__memory_base`.
        `sodium.so` additionally imports `env.__stack_pointer` and
        `env.__table_base` — expected and normal: more complex code
        (libsodium is large, uses indirect calls) needs these standard
        Emscripten `MAIN_MODULE`/dylink globals; the trivial fixture simply
        never exercised this path before.
      - Inspected `node-builds/8-5/8_5_10/php_8_5.wasm`'s own **exports**
        via the same API: 9612 exports total, but **no** `__stack_pointer`
        or `__table_base` among them (only `memory` and
        `__indirect_function_table`) — even though `compile/php/Dockerfile`
        does pass `-s MAIN_MODULE=2` (confirmed at its final link step,
        with an explicit comment: "MAIN_MODULE=2 retains the PHP extension
        ABI for externally built side modules to dynamically link against"
        — i.e. Playground's own pipeline already intended to support
        exactly this scenario).
      - Read `node-builds/8-5/php_8_5.js`'s own dylink runtime
        (`resolveGlobalSymbol()`, `isInternalSym()` — which explicitly lists
        `__stack_pointer`/`__table_base` as recognized internal dylink
        symbols) and confirmed it resolves them by looking up
        `wasmImports['__stack_pointer']` — i.e., it expects the **main
        module's own instantiation** to have already populated a real
        `WebAssembly.Global` there for side modules to later import. Found
        no code anywhere in `php_8_5.js` that actually sets
        `wasmImports['__stack_pointer']` (or `__table_base`) to any such
        Global — so `resolveGlobalSymbol()` returns `undefined` for it,
        which is exactly what produces this precise V8 LinkError when a
        side module then tries to import it as a mutable global.
      - Root cause, most likely (not yet proven by a rebuild): `-s
        MAIN_MODULE=2` (as opposed to `=1`, "export everything") only
        exports symbols explicitly requested. `compile/php/Dockerfile`
        already has real, purpose-built machinery for this exact problem —
        an `llvm-nm --defined-only --extern-only` pass over `libphp.a`
        feeding an explicit `-Wl,--export=` list (`.WASM_ABI_EXPORTS`,
        around line 2233) — but `llvm-nm` only ever lists real C symbols
        (functions/data) that exist in some object file. `__stack_pointer`
        and `__table_base` are not such symbols — they are special globals
        **synthesized by wasm-ld itself** as part of dylink/MAIN_MODULE
        support, so this repo's existing "export the whole libphp ABI"
        mechanism structurally cannot and does not capture them. They most
        likely need their own explicit, separate
        `-Wl,--export=__stack_pointer -Wl,--export=__table_base` (or the
        equivalent Emscripten-level setting) added to the same final link
        command, alongside the existing nm-derived export list.
      - Checked whether this is simply an emsdk-too-old issue first, since
        it seemed like the cheapest explanation: found Emscripten PR #25530
        ("Export mutable wasm globals as JS objects", merged 2025-10-11,
        marked NFC/no functional change) which is clearly in this exact
        area — but our pinned `4.0.19` was tagged 2025-11-04, *after* that
        merge, and "NFC" means it's a refactor with no behavior change on
        its own anyway. So this isn't simply "upgrade Emscripten and it's
        fixed" — ruled out without needing a rebuild to test.
    - **Deliberately not fixed yet**: the fix candidate (adding explicit
      `--export=` entries for these two synthesized globals) requires a
      **full `php.wasm` rebuild** to test — this repo's slowest, most
      resource-intensive build (a whole prior debugging session, decisions
      6-20), on a host already documented as severely disk/RAM-constrained
      (see Environment section). Stopped here deliberately rather than
      burn another multi-GB Docker build chasing an unconfirmed fix — this
      is a call for the user, weighing cost against unblocking `mode:
      shared` for any extension more complex than a trivial fixture.
    - **Scope note**: this bug is orthogonal to everything decisions 30-31
      validated. `@php-wasm/compile-extension` itself works correctly (it
      built a real, correct `sodium.so`); the ABI compatibility question
      (decision 30) is still resolved; the `vendorLib`/`pkgConfigVar`
      mechanism (this decision) works as designed. The gap is specifically
      in how `compile/php/Dockerfile` exports (or fails to export) two
      wasm-ld-synthesized dylink globals from the main module.
33. **Full audit: every third-party lib Dockerfile now sources its version
    from `matrix.json`, none hardcode one anymore (2026-09-12).** User
    flagged this repeatedly and specifically while decision 32's sodium work
    was in progress — first the `libsodium` Dockerfile draft had a
    hardcoded `ARG LIBSODIUM_VERSION="1.0.22"` default ("aucune version
    encodé svp... toujours la dernière... celle de la matrice qui sera mise
    à jour"), then `libz/Dockerfile`'s long-standing `zlib-1.2.13` literal
    ("encore des versions hardcodé...."), then an explicit request to sweep
    every Dockerfile. Audited all of them (`grep` for `ARG.*VERSION="..."`
    defaults and for version-looking strings with no `ARG` backing them at
    all) and found **14 more** beyond libsodium, none previously flagged:
    - **No `ARG` at all, version baked straight into the download URL**:
      `libz` (zlib-1.2.13), `libpng16` (1.6.58), `libjpeg` (3.0.3, twice),
      `libwebp` (v1.6.0), `libaom` (v3.13.1), `libavif` (v1.4.2),
      `libImageMagick` (7.1.2-31, plus a second hardcoded `"7.1.2"` inside a
      generated `MagickWand-config` shell script's `--version` case,
      easy to miss since it's not near the download step), `libiconv`
      (1.17), `libxml2` (`--branch v2.15.4`), `libsqlite3` (trickiest:
      `sqlite-autoconf-3510000` — sqlite.org's own compact encoding,
      `MAJOR*1000000 + MINOR*10000 + PATCH*100 + BUILD`, not the dotted
      version at all).
    - **Had an `ARG` but with a hardcoded default nothing overrode**
      (same shape as decision 25's original curl finding): `libfreetype`
      (`FREETYPE_VERSION="2.14.3"`), `libgd` (`GD_VERSION="2.3.3"`, plus
      **five more** hardcoded `2.3.3` occurrences elsewhere in the same
      file — a `find /root/libgd-2.3.3` path-construction line and, same
      shape as libImageMagick's `MagickWand-config`, a generated
      `gdlib.pc`/`gdlib-config` pair with the version baked into their
      text), `libssh2` (`LIBSSH2_VERSION="1.11.1"`, and — unlike freetype/gd
      — the Makefile never even passed a `--build-arg` for it, so the
      Dockerfile default was the *only* thing setting it), `nghttp2`
      (`NGHTTP2_VERSION="1.70.0"`, same missing-`--build-arg` gap as
      libssh2).
    - Fixed all 14 the same way: `ARG X_VERSION` with **no default**,
      `compile/Makefile` gets `X_VERSION := $(call MATRIX_VERSION,<matrix.json
      key>)` (decision 28's existing mechanism — note some matrix.json keys
      don't match the Dockerfile folder name, e.g. `freetype` not
      `libfreetype`, `libImageMagick` not `libimagemagick`) and an added
      `--build-arg X_VERSION=$(X_VERSION)` on the `docker build` line. The
      two generated-config-script cases (libgd, libImageMagick) needed extra
      care since their heredocs use a **quoted** `<<'EOF'` delimiter
      specifically so pkg-config's own `${prefix}`/`${libdir}` variable
      syntax isn't mistaken for shell variables to expand — fixed by writing
      a `GD_VERSION_PLACEHOLDER` token into the heredoc text and `sed`-
      replacing it with the real `$GD_VERSION` in a separate step afterward
      (libgd), and, since libImageMagick's script is built line-by-line via
      `echo '...' >> file` rather than a heredoc, just switching that one
      `echo` to double quotes with `\"` escaping so `$IMAGEMAGICK_VERSION`
      expands directly (stripping the trailing `-31` build suffix via
      `${IMAGEMAGICK_VERSION%%-*}`, since that config script's own
      `--version` convention only ever reported `MAJOR.MINOR.PATCH`).
      `libsqlite3` needed a real computation, not just substitution: added
      `SQLITE_VERSION_COMPACT := $(shell node -e "...")` in the Makefile
      (mirroring `MATRIX_VERSION`'s own "shell out to Node for logic Make
      can't easily do" pattern) to convert `matrix.json`'s dotted
      `3.51.0` into `3510000` before passing it to the Dockerfile — the
      Dockerfile itself just consumes the precomputed value, no arithmetic
      inside it. (In passing, also fixed a real pre-existing bug in
      `libsqlite3_jspi`'s recipe: a missing trailing `\` line-continuation
      meant `--progress=plain` was silently running as its own no-op shell
      command after `docker build`, not as part of it.)
    - **Real, dangerous inconsistency found and fixed while verifying this**:
      `matrix.json`'s `libjpeg` entry had `versions: ["3.0.3", "3.2.0"]` —
      `3.2.0` *last*, even though that same entry's own `note` documents
      "TESTED and REVERTED 2026-09-12: 3.2.0 fails a real build" (the
      SIMD/spng issue from decision 26) and that the pipeline "stayed on
      3.0.3". Decision 28's own text claims this exact case was "kept in
      the array but not last" — it wasn't, in the actual file. This was
      harmless before this decision (libjpeg's Dockerfile hardcoded `3.0.3`
      directly, ignoring `matrix.json` entirely), but wiring
      `LIBJPEG_VERSION` to `MATRIX_VERSION` as part of this same decision
      would have made the *next* `make libjpeg_jspi` actually try to build
      the known-broken `3.2.0` — caught by checking `make -n -B`'s resolved
      `--build-arg` values for every newly-wired lib before trusting any of
      them, not by a failed build. Fixed by reordering to
      `["3.2.0", "3.0.3"]` and adding a note explaining why the order is
      deliberately non-chronological. **Lesson for later "always try latest
      first" bumps**: reordering `versions` on a revert is necessary, not
      optional, the moment a lib's version is wired through
      `MATRIX_VERSION` — it's not just documentation anymore.
    - **Deliberately left alone**: `oniguruma` — still genuinely unpinned
      (clones its default branch, decision 11's already-documented gap, a
      "pin it to a real tag" problem rather than a "stop hardcoding"
      problem) — and `libzip`'s two parallel, intentionally-fixed
      `LIBZIP_VERSION=1.2.0`/`1.9.2` builds and `libImageMagick.a`'s own
      `libMagickCore-7.Q16HDRI.a` output filename (ImageMagick bakes its
      *major* version into every lib/config-script filename by convention;
      matrix.json only ever pins 7.x releases, so this isn't a "latest
      version" drift risk the way the `--version` string was).
    - Verified every resolution with `node matrix-version.mjs <key>` and
      `make -n -B <target>` (forced dry-run) for all 15 newly-wired targets
      (14 plus libsodium) after the fix — every one now matches exactly the
      version each matrix.json entry's own note says is the tested, adopted
      pin. Not yet done: actually rebuilding any of these libs with the new
      plumbing (only `libsodium_jspi` and the extension-probe libs have been
      rebuilt for real this session) — the dry-run/resolution check confirms
      the wiring is correct, not that every Dockerfile still builds
      end-to-end unchanged (they shouldn't have, since only the version
      *source* moved, not the build steps, but a real rebuild is the only
      full confirmation).
34. **`yaml` and `cmark` PECL extensions added in `mode: static`, sourced
    from their own GitHub repos rather than pecl.php.net (2026-09-12) —
    `cmark` needed a real, non-trivial PHP 7→8 Zend API migration, now
    packaged as a proper patch file, the first real use of decision 12's
    long-documented `patches/<lib>/*.patch` mechanism.**
    - **Why GitHub, not PECL**: user's explicit reasoning — pecl.php.net's
      own package archive lags behind the extension's real GitHub repo.
      Verified concretely, not assumed: `php/pecl-file_formats-yaml`
      (yaml's real upstream) has releases up to `2.3.0` on GitHub; cmark's
      real PECL source is `krakjoe/cmark` (confirmed via `gh api` against
      pecl.php.net's own package page, which links both `krakjoe/cmark` and
      `php/pie`), latest tag `v1.2.0`. `matrix.json`'s `extensions.yaml` and
      new `extensions.cmark` entries now carry `repo`/`sourceTemplate`/
      `versions` fields directly (previously only `libraries.*` entries had
      this — extension *source* tracking is new).
    - **libyaml and cmark (the C library) vendored and cross-compiled** the
      same way as every other lib in this pipeline:
      `compile/libyaml/Dockerfile` (release tarball ships a pre-generated
      `configure`, same shape as libssh2/nghttp2/libsodium) and
      `compile/libcmark/Dockerfile` (CMake, `-DBUILD_TESTING=OFF`, builds
      only the `cmark` target — not the default `all`, which also includes
      a `cmark_exe` CLI executable that doesn't make sense as a
      `-sSIDE_MODULE` build). Both wired into `compile/Makefile`
      (`libyaml_jspi`/`libcmark_jspi`, `MATRIX_VERSION`-sourced per decision
      33, `STRIP_SO`, added to `all_jspi`/`clean`).
    - **Real bug found and fixed, libyaml-specific**: `make libyaml_jspi`
      failed with `configure: error: /bin/bash config/config.sub
      wasm32-unknown-emscripten failed` — libyaml's bundled `config/
      config.sub` (from whenever `0.2.5` was released) predates the
      `wasm32-unknown-emscripten` triplet, unlike every other lib's bundled
      copy so far. Fixed by overwriting it with the base image's own
      (Ubuntu Noble's `/usr/share/misc/config.sub`, confirmed via a
      throwaway `docker run` to actually recognize the triplet) rather than
      fetching one over the network — a first attempt fetching from
      `git.savannah.gnu.org` (the canonical upstream) hit a live 502 Bad
      Gateway, so a local, already-present file is both more reliable and
      faster. `make libyaml_jspi` and `make libcmark_jspi` both then
      succeeded (`libyaml.a` ~403KB, `libcmark.a` ~194KB).
    - **`compile/php/Dockerfile` wiring**: `ARG YAML_EXT_VERSION`/
      `ARG CMARK_EXT_VERSION` (matrix-sourced, passed through `cli.mjs` →
      `build.js` → `docker build`, the same three-layer chain
      `WITH_OPENSSL_VERSION` already used — **both `build.js`'s explicit,
      hand-maintained `--build-arg` list and `cli.mjs`'s `IMPLEMENTED_EXTENSIONS`
      needed new entries; a flag cli.mjs generates but build.js doesn't
      forward is silently dropped**, caught by actually reading build.js's
      full `docker build` invocation rather than assuming the existing
      `WITH_YAML`/`WITH_CMARK` `IMPLEMENTED_EXTENSIONS` entry alone was
      enough), `WITH_YAML`/`WITH_CMARK` configure-flag blocks mirroring the
      existing `WITH_MBREGEX` pattern exactly (`--with-yaml=/root/lib` /
      `--with-cmark=/root/lib` + the built `.a` appended to
      `.emcc-php-wasm-sources`). New `compile/matrix-version.mjs`
      function, `getMatrixExtensionVersion()`, mirroring `getMatrixVersion()`
      but reading `matrix.json`'s `extensions.<key>` entries instead of
      `libraries.<key>` (extension *source* versions are a new category
      matrix.json didn't track numerically before this).
    - **Validation strategy, cheap before expensive**: rather than testing
      via a full `php.wasm` rebuild (this pipeline's slowest, most
      resource-intensive build), reused the decision 30/31/32
      `compile-extension` mechanism as a **disposable smoke-test harness**
      — pointing it at the vendored extension sources with `mode: shared`
      in a throwaway config copy (never committed) purely to get a fast
      real-compiler verdict on "does this extension's C code build against
      real PHP 8.5 headers and our vendored lib," independent of the
      static-vs-shared question. This surfaced both real findings below
      without touching the expensive full build.
    - **yaml: clean.** Configures and compiles with no errors against
      vendored `libyaml.a` (`ext/yaml`'s own `config.m4` does plain
      autoconf header/lib detection, no PKG_CHECK_MODULES). The
      smoke-test's only failure was libtool refusing to link a `.a`-only
      dependency into a real `.so` ("this system can not link to static lib
      archive... module is probably useless") — expected and irrelevant to
      our actual target: `mode: static` never asks libtool to produce a
      standalone `.so` at all; php-src's own build just compiles the `.o`
      files and lets our own `.emcc-php-wasm-sources` final-link mechanism
      handle linking, entirely bypassing that code path. Confirmed this is
      the exact same non-issue as `mode: shared`'s own README warning
      against `PHP_ADD_LIBRARY_WITH_PATH`, not a config or vendoring
      mistake on our part.
    - **cmark: config.m4 cross-compile blocker patched successfully, then a
      second, deeper, real PHP 7→8 incompatibility found underneath.**
      First smoke-test attempt failed at `./configure` itself:
      `krakjoe/cmark`'s `config.m4` calls `AC_TRY_RUN` to check libcmark's
      version, which *executes* the compiled test program — impossible
      when cross-compiling (no fourth "action-if-cross-compiling" argument
      given, so autoconf hard-fails with "cannot run test program while
      cross compiling"), regardless of anything about our Docker/Emscripten
      setup. Patched by replacing that `AC_TRY_RUN` block with a
      cross-compile-safe skip (we already vendor a known, fixed libcmark
      version verified to satisfy the minimum). Second attempt then got
      past `./configure` cleanly (confirmed in the log: "checking for
      cmark_markdown_to_html in -lcmark... yes", "checking for cmark
      minimal version... skipped...") but failed to *compile*, with real
      `-Wincompatible-function-pointer-types` errors in `src/custom.c` and
      `src/node.c` — `krakjoe/cmark` implements custom Zend object handlers
      (`read_property`/`write_property`/`has_property`/`unset_property`/
      `get_debug_info`/`clone_obj`) using PHP 7's signature
      (`zval *object, zval *member, ...`), but PHP 8 changed this ABI to
      `zend_object *object, zend_string *member, ...`. Investigated whether
      this was already fixed anywhere before attempting a patch: `krakjoe/
      cmark`'s `develop` branch is byte-identical to its `v1.2.0` tag
      (`ahead_by: 0`), its last real code commit is from **2019** (long
      before PHP 8 shipped), and even the user's own past fork
      (`ZmotriN/php-cmark`) only adds a Windows build tweak on top of the
      same 2019 base — confirming this is a genuine, unfixed-anywhere
      upstream gap, not something to search for further.
    - **User explicitly chose the full migration** over shelving cmark or
      hunting for an alternative extension. Migrated by hand across every
      affected file (found by grepping for the handler-assignment lines,
      not by assuming the two files the first compiler error happened to
      hit were the only ones): `src/handlers.h`/`src/handlers.c` (the base
      `php_cmark_node_*` read/write/isset/unset/debug/clone — every
      subtype's handler falls back to these), `src/custom.c`,
      `src/block.c`, `src/heading.c`, `src/list.c` (both `list` and
      `ordered_list` variants), `src/media.c`, `src/text.c`, and
      `src/call.c`/`src/call.h` (`clone_obj` only — found by grepping for
      handler *assignments* specifically, since `clone_obj`'s signature
      change is a single-argument function that a signature-shaped grep
      for "zval *object, zval *member" had missed). Added a `*_fetch_obj()`
      macro variant next to each type's existing `*_fetch()` (which expects
      a `zval*` via `Z_OBJ_P()`) for use inside the migrated handlers, which
      now receive a `zend_object*` directly. Also collapsed every
      `#if PHP_VERSION_ID >= 70400` / `#else` split (upstream's own PHP
      7.4-vs-earlier `write_property` return-type compatibility shim) down
      to the single `>= 7.4` branch, since this repo only ever targets PHP
      8.5 — simplification, not just migration.
    - **Real bug found and fixed mid-patch, unrelated to the Zend API
      itself**: a code comment written as `zend_object*/zend_string*`
      accidentally closed the enclosing `/* ... */` C comment early at the
      literal `*/` inside it, spilling an em dash out into real code as an
      "unexpected character <U+2014>" compiler error — fixed by spacing it
      to `zend_object* / zend_string*`. Caught by actually re-running the
      smoke test, not by inspection.
    - **Patch generated and verified independently of the edited working
      copy**, not just trusted by construction: diffed the final migrated
      source against a byte-for-byte pristine `v1.2.0` tarball (kept aside
      specifically for this) with `diff -ru`, producing a 14-file, ~1500-
      line unified patch, saved as `patches/cmark/php8-object-handlers.patch`
      — the first real file in decision 12's previously-empty `patches/`
      convention. Verified twice: `patch -p1 --dry-run` against a second
      fresh pristine copy, and `git apply --no-index` (the exact command
      `compile/php/Dockerfile` now runs) against a third — both applied
      every file cleanly with zero fuzz.
    - **Architecture correction requested mid-session**: initially vendored
      yaml/cmark's full source as committed files under
      `compile/extensions/{yaml,cmark}/` (mirroring decision 30's sodium
      fixture) and hand-edited them in place. User asked directly whether
      independent patch files were being produced for source that gets
      downloaded, rather than a committed pre-patched copy — correct
      instinct: every other dependency in this repo downloads fresh at
      build time and never commits vendored third-party source.
      Restructured accordingly: `compile/php/Dockerfile` now downloads both
      extensions' sources fresh via `wget` from the matrix-sourced GitHub
      URL directly into `php-src/ext/{yaml,cmark}`, then (cmark only)
      applies `patches/cmark/php8-object-handlers.patch` via `git apply
      --no-index`, matching the exact mechanism `compile/php/php*.patch`
      already uses for PHP itself. The `compile/extensions/{yaml,cmark}/`
      directories were deleted — they only ever existed as the staging
      ground used to produce the patch.
    - **Queued for later, not done now**: contribute the Zend API migration
      back upstream as a real PR (to `krakjoe/cmark` or the user's own
      `ZmotriN/php-cmark` fork) — flagged explicitly by the user mid-session
      as a "think about this later" item, not a "do this now" one.
      **Reconfirmed 2026-09-12, after decision 35's three follow-on cmark
      bugs were found and fixed (Traversable/abstract ordering on Node and
      on every subclass, plus the missing `zend_object_properties_size()`/
      `object_properties_init()` in every `create_object`)**: the user
      wants to package all four accumulated cmark patches together and
      publish them as our own maintained fork/release once the current
      rebuild confirms everything works end-to-end — not just the one
      Zend API migration patch from this entry, the whole set. Revisit
      once decision 35's final rebuild is green.
      Also
      still open: `compile/extensions/*/vendor/`-style dependency
      auto-staging isn't relevant here (yaml/cmark are `mode: static`, no
      `vendorLib` mechanism involved — that machinery is specific to
      `mode: shared`, decision 32), and no full `php.wasm` rebuild has
      exercised this new code path end-to-end yet — same "batch with the
      decision 32 `__stack_pointer` fix" reasoning as before, still
      pending.

35. **Real cmark runtime bug found and fixed: `CommonMark\Node` registered
    `Traversable` in the wrong order relative to becoming abstract
    (2026-09-12).** Surfaced by the user actually consuming a built
    `php.wasm` (`kiri phpinfo`), not by a build/link failure: `PHP Fatal
    error: Class CommonMark\Node must implement interface Traversable as
    part of either Iterator or IteratorAggregate`. Traced to
    `Zend/zend_interfaces.c`'s `zend_implement_traversable()`: a class
    implementing bare `Traversable` (not `Iterator`/`IteratorAggregate`
    directly) is only exempted from this check if it already carries
    `ZEND_ACC_EXPLICIT_ABSTRACT_CLASS` **at the exact moment**
    `zend_class_implements()` registers the interface — confirmed by
    reading `zend_do_implement_interface()` (`Zend/zend_inheritance.c`):
    the interface is appended to `ce->interfaces[]` and its
    "gets_implemented" hook fires immediately, synchronously, not deferred.
    `krakjoe/cmark`'s `src/node.c` sets that flag in
    `PHP_RINIT_FUNCTION(CommonMark_Node)` (once per **request**), but calls
    `zend_class_implements(php_cmark_node_ce, 2, php_cmark_node_visitable_ce,
    zend_ce_traversable)` back in `PHP_MINIT_FUNCTION` (once per **process**,
    which always runs first) — so the exemption was always applied one
    phase too late. Confirmed no other node subtype file (document, heading,
    list, etc.) has this bug: none of them re-declare Traversable or set
    the abstract flag themselves, they just inherit Node's already-resolved
    interfaces. Confirmed the fix doesn't break anything else: `object_init_ex`
    (used internally to instantiate Document/Heading/etc. objects) never
    checks the abstract flag — only the `new` opcode does, and cmark never
    uses it internally — so marking Node abstract earlier doesn't block
    internal subclass instantiation; `ZEND_ACC_FINAL` deliberately stays in
    RINIT (moving it to MINIT too would break every subclass's own MINIT,
    which extends Node via `zend_register_internal_class_ex` and would hit
    "cannot extend final class"). Fix: new
    `patches/cmark/php8-node-traversable-abstract-order.patch` (second
    patch file for cmark, applied by the same existing `git apply --no-index
    /root/patches-cmark/*.patch` glob in `compile/php/Dockerfile` — no
    Dockerfile change needed) moves `ZEND_ACC_EXPLICIT_ABSTRACT_CLASS` into
    MINIT, right after `get_iterator` is set and before
    `zend_class_implements()`. Verified with `git apply --no-index --check`
    against a fresh pristine `v1.2.0` `src/node.c` download — applies
    cleanly. Not yet verified against a real running `php.wasm` (that
    requires the full rebuild below to finish).
    - **Also discovered while investigating**: the Docker build cache for
      this project was completely gone (no `kirigami-php-wasm:*` images at
      all — only leftovers from an unrelated sibling project,
      `kirigami-audiowaveform-wasm`), and 3 uncommitted files
      (`.dockerignore`, `compile/libgd/Dockerfile`,
      `compile/php/Dockerfile`) were sitting from an apparently-interrupted
      prior rebuild attempt: `.dockerignore` allowing `patches/` into the
      Docker build context, a libgd CMake `FREETYPE_*` variable casing fix,
      and decision 32's `-Wl,--export=__stack_pointer` candidate fix (still
      missing `--export=__table_base`, also identified as needed back then
      — left as-is here, not re-investigated this round).
    - **Real Make bug hit again while relaunching the build**:
      `compile/base-image/.ready` was a **stale marker** — it still existed
      on disk (dated *after* the Docker data wipe) even though the actual
      `kirigami-php-wasm:base` image no longer existed, so `make` considered
      `base-image` already satisfied and every lib build immediately failed
      trying to `FROM kirigami-php-wasm:base` (Docker attempted to pull it
      from Docker Hub instead of building it locally: "pull access denied").
      This is exactly the failure mode decision 25's own `base-image/.ready`
      fix was meant to prevent for *normal* rebuilds, but it doesn't protect
      against the marker surviving an *external* wipe of the Docker image
      store itself (this repo's files were untouched, only Docker's data
      was gone) — deleted the stale `.ready` by hand to force `make` to
      rebuild `base-image` for real. **Not fixed generically**: a future
      session hitting "pull access denied for kirigami-php-wasm:base" again
      should suspect this same stale-marker-vs-wiped-Docker-store mismatch
      first.
    - **Correction / continuation, same session**: the first rebuild
      succeeded (base image + all libs + PHP, exit code 0), and a real
      runtime smoke test (via `@php-wasm/universal`'s `loadPHPRuntime` +
      `PHP.run`, same technique as decision 30, throwaway script) confirmed
      `CommonMark\Node` itself no longer crashes MINIT — but immediately
      surfaced the **exact same bug on every concrete subclass**:
      `CommonMark\Node\Text` (the first one PHP reaches next). Root cause:
      `zend_do_inherit_interfaces()` (`Zend/zend_inheritance.c`) calls
      `do_implement_interface()` — which re-invokes `zend_implement_traversable`
      — **for every interface a class inherits from its parent, not just
      ones it declares itself**. Since every concrete node type
      (`Document`, `Text`, `Heading`, `CodeBlock`, `Link`, ... — confirmed
      21 registration call sites across 12 files) registers via
      `zend_register_internal_class_ex(&ce, php_cmark_node_ce)` (or a
      grandparent like `php_cmark_node_text_ce`), each one re-triggers the
      same check, and none of them carry the abstract exemption. Ruled out
      "just implement `zend_ce_iterator` directly instead of bare
      `Traversable`" as the fix: read `zend_implement_iterator()` in the
      same file and confirmed it only safely early-returns when a class's
      `get_iterator` pointer differs from its parent's (true for `Node`
      itself, which has no parent) — every subclass inherits the *identical*
      `get_iterator` pointer unchanged, so that function would fall through
      to dereferencing `funcs_ptr->zf_rewind->common.scope` for a
      `rewind()` method that doesn't exist anywhere in this codebase (no
      class defines real `current`/`key`/`next`/`rewind`/`valid` PHP
      methods — iteration is entirely handled by the custom C
      `get_iterator` handler) — a near-certain null-pointer dereference/wasm
      trap. Also ruled out "mark every subclass abstract like Node" as a
      blanket fix: confirmed (via `grep PHP_METHOD.*__construct`) that
      **every single concrete node type has a public `__construct`** —
      `new CommonMark\Node\Text("...")` is a documented, intended way to
      *build* documents programmatically, not just parse them — so leaving
      any of them permanently abstract would silently break that.
    - **Actual fix**: toggle `ZEND_ACC_EXPLICIT_ABSTRACT_CLASS` on and back
      off around each registration call only — set it on the local `ce`
      right before `zend_register_internal_class_ex()` (satisfies the
      check with zero cost, no method-table involvement, same safe path
      Node already uses), then clear it on the *returned* persistent class
      entry immediately after, before `PHP_MINIT_FUNCTION` returns and long
      before any request could try to `new` it. New second patch file,
      `patches/cmark/php8-subclass-traversable-abstract-order.patch`,
      applies this 3-line wrap to all 21 call sites across
      document/quote/list/item/block/paragraph/heading/break/text/code/
      inline/media `.c` files (mechanical, generated via a small Perl
      one-liner over freshly-downloaded pristine sources, not hand-typed —
      see the process note below).
    - **Process bug caught before it could waste a rebuild**: the first
      attempt at writing both new patch files used the `Write` tool with
      manually-composed diff text rather than the file the `diff -ru`
      command actually produced. Comparing the two byte-for-byte
      afterward turned up real corruption from the manual transcription —
      lost leading spaces on blank context lines (unified diff requires
      every hunk line to start with ' ', '+', or '-'; a bare empty line
      is technically malformed) *and* actual typos (wrong namespace
      string for `Emphasis`/`Strong`, a swapped method-table variable name
      for `OrderedList`, missing `create_object` assignment lines).
      `git apply --no-index --check` against a fresh pristine tree caught
      this immediately (clean apply for the first, hand-typed patch by
      luck/leniency; hard failures for the second). Fixed by copying the
      raw `diff -ru` output directly into the repo files instead of
      retyping it, then re-verifying. **Lesson for next time a patch file
      is authored here**: always generate it with `diff -ru a b` (or `git
      diff --no-index`) and copy the resulting file byte-for-byte — never
      hand-compose unified-diff text — and always run `git apply --no-index
      --check` against a *freshly downloaded* pristine copy (not the
      working copy used to draft it) before trusting it in a Dockerfile
      that costs a full rebuild to fail on.
    - Second full rebuild kicked off in the background with all three
      cmark patches (`php8-node-traversable-abstract-order.patch`,
      `php8-object-handlers.patch`,
      `php8-subclass-traversable-abstract-order.patch`, applied in that
      alphabetical order by the Dockerfile's existing `*.patch` glob —
      verified this exact three-patch sequence applies cleanly in one shot
      against fully pristine sources before spending the rebuild on it).
      Also still exercises decision 32's `__stack_pointer` export in the
      same build.
    - **✅ Second rebuild succeeded, and the originally-reported bug is
      confirmed fixed.** Re-ran the decision-30-style `@php-wasm/universal`
      smoke test: `get_loaded_extensions()` lists `cmark` (MINIT completes
      cleanly for the whole extension, no more `E_CORE_ERROR`), and
      `\CommonMark\Parse("# Hello\n\nWorld")` (the extension's real
      top-level parse function — `ZEND_NS_NAMED_FE("CommonMark", Parse, ...)`
      in `cmark.c`; there is no `Document::parse()` static method, that was
      a wrong guess in the first test script) successfully parses markdown
      into a `CommonMark\Node\Document` object tree. The fatal error the
      user originally hit via `kiri phpinfo` is gone.
    - **❌ New, separate, unresolved bug found while smoke-testing further**:
      PHP crashes with a hard, uncatchable WASM trap
      (`RuntimeError: memory access out of bounds` in
      `zend_object_dtor_property` / `zend_object_std_dtor`, from Node's own
      JS stack trace, not a catchable PHP exception) during **request
      shutdown**, specifically when destroying **any** cmark `Node`-derived
      object — reproduced with nothing but a bare
      `new CommonMark\Node\Document()` and no parsing at all, so it is
      unrelated to tree size/complexity. This means cmark objects can be
      created and used within a request today, but the runtime currently
      crashes when PHP tries to garbage-collect/destroy them at request
      end. Likely area (not yet confirmed): `php_cmark_node_t`'s custom
      object layout (`php_cmark_node_handlers.offset =
      XtOffsetOf(php_cmark_node_t, std)`, a non-standard "zend_object
      embedded mid-struct" layout) vs. how `zend_object_properties_size(ce)`
      /`ecalloc` sizing interacts with `zend_declare_property_null`'s
      9 declared properties (parent/previous/next/firstChild/lastChild/
      startLine/endLine/startColumn/endColumn) during
      `zend_object_std_dtor`'s properties-table walk — but this is a
      hypothesis, not yet diagnosed the way the Traversable bug was (no
      confirmed root cause in the actual C source yet). **Deliberately not
      chased further this session** (token/time budget) — the object
      handlers migration patch (`php8-object-handlers.patch`) is the most
      likely place to have introduced or exposed this, but `free_obj`
      itself was never in scope for that patch (its signature is unchanged
      between PHP 7.4 and 8), so this could equally be a pre-existing
      upstream bug that simply never had a chance to surface before (the
      Traversable fatal crashed MINIT before any object was ever created).
      Next session: reproduce with a debug PHP build
      (`WITH_DEBUG=yes`/sourcemaps) for a real stack trace into the C
      source rather than a bare WASM function-index trace, before guessing
      at a fix.
    - **✅ Root cause found and fixed for real, session continued
      2026-09-12 with purchased credits.** The `object_properties_init()`
      fix above was rebuilt and re-tested: the *original* shutdown crash
      moved (not disappeared) to a *new* crash at `new
      CommonMark\Node\Document()` itself, inside `zend_std_get_constructor`
      — proof the previous fix was treating a symptom, not the cause.
      Ruled out a general build problem first (`new stdClass()`,
      `new Exception()`, `new ArrayObject()`, and even `new
      CommonMark\CQL()` — the extension's *other*, unrelated class from
      `call.c` — all worked correctly, including CQL correctly throwing a
      catchable `ArgumentCountError`), narrowing the bug to something
      specific to the `Node` class hierarchy alone. Read `src/node.h`
      (fetched fresh from the real `krakjoe/cmark` v1.2.0 tag) and found
      it: `php_cmark_node_t` doesn't embed a real `zend_object`, it
      hand-rolls its own mimic struct —
      `struct { zend_refcounted_h gc; uint32_t handle; zend_class_entry
      *ce; const zend_object_handlers *handlers; HashTable *properties; }
      std;` — so that every subtype's own struct (`php_cmark_node_text_t`,
      `_heading_t`, etc.) can embed `php_cmark_node_t h` by value and add
      its own trailing `zval` fields (`literal`, `fence`, ...) that double
      as that subtype's declared-properties storage, all without a
      separate allocation. This is a real, deliberate, and actually rather
      elegant design — **but it requires this mimic struct's fields to
      exactly match the real `zend_object`'s, in order, forever**, because
      Zend's own internals (`object_properties_init`,
      `zend_object_std_dtor`, `zend_std_get_constructor`, anything doing
      `object->ce`/`object->properties_table`/etc.) read/write through
      this memory via `zend_object*`-typed, *offset-based* field access —
      they have no idea this is a hand-rolled mimic, they just trust the
      byte layout. Fetched the real `zend_object` definition from the
      exact `php-8.5.10` tag this project builds
      (`Zend/zend_types.h`) and found the mismatch: real `zend_object` has
      **six** fields — `gc, handle, extra_flags, ce, handlers, properties`
      — plus a trailing `properties_table[1]`. Cmark's mimic is missing
      `extra_flags` (a `uint32_t` inserted between `handle` and `ce`,
      added to PHP at some point after cmark's last commit in 2019, for
      `OBJ_EXTRA_FLAGS()`). Every field after the missing one — `ce`,
      `handlers`, `properties`, and therefore the whole properties-table
      region used by both `object_properties_init` (decision 35's earlier
      patch) and by `zend_object_std_dtor`'s original, unpatched behavior
      — was being read and written 4 bytes off from where cmark's own code
      actually stores it, corrupting whatever real data happened to sit at
      that wrong offset. This explains *both* crashes: the original one
      (`zend_object_dtor_property` at shutdown, walking a misaligned
      properties-table) and the new one this fix's own predecessor
      accidentally caused (writing `object_properties_init`'s default
      zvals 4 bytes off, smearing into `ce`/`handlers`, surfacing later at
      the next `new`). Fix: **one line**, a new
      `patches/cmark/php8-node-object-layout-extra-flags.patch` adding
      `uint32_t extra_flags;` in the correct position inside `src/node.h`'s
      mimic struct — the *only* place in the whole extension with this
      hand-rolled shape (confirmed by grepping every other header for
      `zend_refcounted_h gc`; every subtype embeds `php_cmark_node_t`
      instead of redefining its own, so fixing this one struct fixes the
      whole hierarchy). Verified all 5 cmark patches now apply cleanly
      together, in the Dockerfile's alphabetical glob order, against a
      fully pristine `v1.2.0` checkout, before spending a rebuild on it.
    - **✅✅ Full end-to-end success, verified for real.** Rebuilt once more
      (fifth `php.wasm` build this session) and re-ran the complete
      original scenario: `\CommonMark\Parse("# Hello\n\nWorld")` → a real
      `CommonMark\Node\Document` tree; `$doc instanceof Traversable` is
      `true`; `foreach ($doc as $child)` genuinely walks the tree
      (Document → Heading → Text → Heading → Paragraph → Text → Paragraph
      → Document, matching cmark's own enter/leave visitor pattern) with
      no crash; the whole request — including destroying every node object
      at shutdown — completes with exit code 0. Also re-confirmed the
      simpler cases from decision 35's earlier entries still hold (`new
      CommonMark\Node\Document()` alone, `get_loaded_extensions()` lists
      `cmark`). **This closes out the entire cmark investigation**: five
      patches now live in `patches/cmark/`, each fixing one distinct,
      real, independently-diagnosed PHP 7→8 (and PHP-version-drift)
      incompatibility in unmodified upstream `krakjoe/cmark` v1.2.0 code:
      (1) Node's own Traversable/abstract registration order, (2) the
      original Zend object-handler signature migration (zval*→zend_object*),
      (3) the same Traversable/abstract ordering bug on all 21 concrete
      subclass registrations, (4) object-properties-table
      initialization (harmless-now-redundant given fix 5, but not wrong —
      left in place), (5) the actual root cause, a missing `extra_flags`
      field in the extension's hand-rolled `zend_object` mimic struct.
      **User's explicit follow-up (2026-09-12, after this fix landed)**:
      package all five patches together and publish them as our own
      maintained cmark fork/release (see the updated note earlier in this
      same decision) — not just the one Zend API migration patch, the
      whole accumulated set. Also requested: write down how to actually
      *use* the now-working extension (see the new "cmark usage" note in
      this file / wherever it ends up) so this is easy to pick back up
      when `kirigami/kirigami` actually wants to consume it.

36. **`ext/cmark` abandoned as the core, `mode: static` markdown extension —
    despite being fully fixed and verified end-to-end by decision 35 — in
    favor of a new, Kirigami-owned extension (2026-09-12).** *(Backfilled
    from `matrix.json`'s own `extensions.cmark` "ABANDONED" note and
    `config.yaml`'s comments — this decision and decision 37 happened in a
    session whose narrative notes weren't carried into this file at the
    time; reconstructed from repo state rather than a full blow-by-blow.)*
    `config.yaml`'s `cmark` entry moved from `mode: static` to `mode:
    shared` (`source: compile/extensions/cmark`, `vendorLib: libcmark`) —
    still available as an optional install, just no longer baked into every
    core build. `matrix.json`'s note preserves the full five-patch fix
    write-up so it isn't lost if cmark is ever reconsidered.
37. **`mdhtml` (new `php-kirigami/php-mdhtml` repo) added as the core,
    `mode: static` markdown extension replacing cmark (2026-09-12).**
    *(Backfilled, see decision 36's note.)* A Kirigami-owned CommonMark+GFM
    extension built on `cmark-gfm` (`compile/libcmark-gfm/Dockerfile`,
    `matrix.json`'s `libraries.libcmark-gfm` — `github/cmark-gfm` release
    `0.29.0.gfm.13`) rather than plain `cmark` — GFM (tables, strikethrough,
    autolinks, task lists) plus GitHub-flavored rendering, output as an HTML
    string rather than a traversable object tree (no Zend object-layout
    complexity to migrate, unlike cmark — see decision 35). `matrix.json`'s
    `extensions.mdhtml` entry: `repo: php-kirigami/php-mdhtml`, pinned
    `v0.1.0`. Wired into `compile/php/Dockerfile` (`WITH_MDHTML` flag,
    `--with-mdhtml=/root/lib`, links `libcmark-gfm.a` +
    `libcmark-gfm-extensions.a`) the same way cmark was. `config.yaml`'s
    `mdhtml: { mode: static }` sits in the core's PECL-sourced group
    alongside `yaml`.

38. **This repo becomes an npm-workspaces monorepo for `@kirigami/phpext-*`
    packages — one real published npm package per `mode: shared` extension
    (2026-09-15).** Renamed the naming convention decisions 5/27/31
    established (`@kirigami/ext-<name>`) to **`@kirigami/phpext-<name>`**
    — extensions only (the core `mode: static` build stays a separate
    package, `@kirigami/php-wasm`, assembled and published from the
    `kirigami` repo per decision 15 — explicitly out of scope here, per the
    user).
    - **npm workspaces, not pnpm** — a correction to decision 2's claim
      that the Kirigami ecosystem is "already Node/pnpm": checked the
      sibling `kirigami` repo directly and found it actually uses plain npm
      workspaces (`package.json`'s `"workspaces": ["packages/*"]`,
      `package-lock.json`, no `pnpm-workspace.yaml`) plus a hand-rolled
      `scripts/publish.js` — no Changesets either. Matched that exact
      pattern here for consistency rather than introducing a different tool
      the rest of the ecosystem doesn't use. New root `package.json`
      (`private: true`, `workspaces: ["packages/*"]`, `scripts.release` /
      `release:dry`).
    - **`scripts/publish.js`** — adapted directly from
      `../kirigami/scripts/publish.js`: same `alreadyPublished()` idiom
      (skip a package whose exact `name@version` is already on the npm
      registry — this is the actual "don't publish packages for nothing"
      mechanism, not a git-diff heuristic), same pack-then-`npm
      publish`-a-tarball flow, same interactive confirm/`--dry-run`/`--otp`
      flags. Dropped: the jsDelivr schema-purge step (phpext packages ship
      no JSON schemas) and the dependency-order topological sort (phpext
      packages don't depend on each other). Added: a build phase up front
      (`node compile/cli.mjs compile-extension`, `--skip-build` to opt out)
      — kirigami's own script builds nothing itself, so this is a real
      difference to accommodate that these packages are always freshly
      compiled, not hand-written.
    - **Version bumps are automatic and hash-driven, not manual** — the
      other half of "don't publish for nothing." `compile/cli.mjs`'s new
      `computeBuildHash()` hashes a package's actual compiled output
      (`manifest.json` + every `*.so`, sorted, name+bytes); `.buildhash`
      (committed, next to `package.json`) records the hash from the last
      time the version was bumped. `syncExtensionPackage()` (replacing the
      old hardcoded `EXTENSION_PACKAGE_VERSION = '0.1.0'` placeholder from
      decision 27) bumps the patch version only when a fresh build's hash
      differs from `.buildhash` — an unchanged rebuild leaves the version
      (and `.buildhash`) untouched, so `scripts/publish.js`'s registry
      check then correctly skips it. Verified with a standalone, no-Docker
      reproduction of the exact algorithm against a copy of the real
      sodium build output: unchanged rebuild → version held (`0.1.0` →
      `0.1.0`), changed rebuild → bumped (`0.1.0` → `0.1.1`) — see the
      scratchpad test run this session (not committed, `compile/cli.mjs`'s
      real functions were copied verbatim for the test, then verified
      identical to what actually landed in the file).
    - **`packages/*/package.json` and `.buildhash` are now committed**, a
      reversal of decision 31's "nothing here is hand-edited, don't commit
      any of it" — `.gitignore` narrowed from a blanket `/packages/` ignore
      to just `packages/*/manifest.json` and `packages/*/*.so` (the actual
      compiled artifacts, still fully regenerated every build). The
      distinction: `package.json`/`.buildhash` are mechanically
      maintained, not hand-edited, but they ARE the real record of what
      version was last published — exactly the kind of thing that belongs
      in git (same category as a lockfile), unlike the multi-hundred-KB
      `.so` binaries themselves.
    - **`extensionPackageJson()` fleshed out** to match the field shape
      every other `@kirigami/*` package.json in the `kirigami` repo uses
      (`keywords`, `homepage`, `bugs.url`, `publishConfig.access: public`,
      `author`) — previously a minimal `name`/`version`/`description`/
      `license`/`repository`/`files` only.
    - **`packages/phpext-sodium/README.md` and `packages/phpext-cmark/
      README.md`** (new, committed) — written on the exact template
      `../kirigami/packages/audiowaveform-wasm/README.md` uses (centered
      header with the Kirigami logo, tagline, npm/license/node/website
      badges, `---` rules, Table of contents, closing License/Author),
      scaled to these packages' size. Each documents its own pinned
      versions concretely (sodium: `ext/sodium` from the `PHP-8.5.10` tag +
      vendored `libsodium 1.0.22`; cmark: `krakjoe/cmark v1.2.0` +
      vendored `commonmark/cmark 0.31.2`, plus a table of all 5 patches)
      rather than describing the mechanism only in the abstract.
    - **`kirigami` package.json section added** (same day, follow-up):
      every `@kirigami/plugin-<name>` package.json in the `kirigami` repo
      already carries a `kirigami: { type: "plugin", minVersion, ... }`
      metadata block (e.g. `packages/plugin-embed/package.json`) — extended
      that same convention to `phpext-<name>` packages with `type:
      "extension"` (CLAUDE.md decision 5's own term for this plugin kind).
      New `buildKirigamiExtensionMetadata()` in `cli.mjs` fills it with:
      `phpVersions` (the major.minor list this build ships, straight from
      `config.php.versions`), `minVersion` (the oldest full PHP patch
      version among them, e.g. `"8.5.10"`, resolved via
      `supported-php-versions.mjs` — deliberately *not* a
      `@kirigami/php-wasm` semver, since this repo has no visibility into
      that package's own version number; the user chose "the PHP version
      actually compiled against" as the honest thing to record instead),
      `vendorLib: { name, version }` when the extension vendors one
      (matrix.json-resolved, via the existing `getMatrixVersion()`),
      and `buildHash` (the same hash `.buildhash` tracks, mirrored into
      package.json so a consumer/scanner can read it without a second
      file). Verified with the same no-Docker scratchpad technique as the
      version-bump logic above, this time also importing the real
      `matrix-version.mjs`/`supported-php-versions.mjs` (not fabricated
      data) — produced `minVersion: "8.5.10"`,
      `vendorLib: { name: "libsodium", version: "1.0.22" }` correctly.
      **Real bug caught by this same syntax-check step**: a docstring
      describing the plugin-package convention originally read
      `packages/plugin-*/package.json` — the literal `*/` substring closed
      the enclosing `/** */` JSDoc comment early (same failure shape as
      decision 35's em-dash-inside-a-comment lesson, different character),
      turning the rest of the comment into invalid code. Caught by
      `node --check` before it ever reached a real build; reworded to name
      one concrete example package instead of a glob pattern.
    - **Not done this session** (no Docker build was run — a build for a
      different project was already in progress on this machine, so
      nothing here was compiled): the actual `packages/phpext-sodium/` and
      `packages/phpext-cmark/` directories exist right now with only their
      `README.md` — `package.json`, `.buildhash`, `manifest.json`, and the
      `.so` files are only created by the next real (Docker-backed) `node
      compile/cli.mjs compile-extension` run. The stale, pre-existing
      `packages/ext-sodium/` (old naming, from decision 32's pilot, always
      untracked) was deleted rather than renamed, since it's fully
      regenerated anyway.
39. **Decision 5's open question — which layer owns `@kirigami/phpext-*`
    auto-detection — resolved: `@kirigami/php-wasm` itself, not the
    `kirigami` framework (2026-09-15).** Stated directly by the user: install
    `@kirigami/phpext-sodium`, and `@kirigami/php-wasm` "devra scanner parmi
    les dépendances s'il y a des packages d'extension" at startup and load
    them automatically — no manual wiring by the app author, no involvement
    from the `kirigami` framework layer at all. Concretely, this needs (in
    the `kirigami` repo, **not** this one — out of scope for this session):
    a startup scan of the consuming app's `node_modules` for installed
    `@kirigami/phpext-*` packages, then feeding each one's `manifest.json`
    into `@php-wasm/universal`'s `resolvePHPExtension()` /
    `{ format: 'manifest' }` loading path (already proven to work,
    decision 30). The user's own framing — "exposer le fichier .so et la
    config de base .ini" — already exists today without new work: a
    shared extension's `manifest.json` (decision 5) already carries the
    ini/env directives such a scanner needs; no separate raw `.ini` file is
    shipped or required. This repo's job is only to guarantee the contract
    a scanner would rely on — package name pattern `@kirigami/phpext-*`,
    `manifest.json` + `*.so` always sitting at the installed package's
    root — which decision 38's rename/workspace already satisfies. Actually
    implementing the scan-and-load logic itself is `@kirigami/php-wasm`'s
    own follow-up work in the `kirigami` repo, not tracked further here.

40. **`cmark` (both the abandoned `mode: static` build and the `mode: shared`
    pilot from decision 38) removed entirely from the build and from
    `matrix.json` — the user decided it's not worth the maintenance cost
    (2026-09-15).** Triggered by actually trying to build the `mode: shared`
    pilot for real (this session's background `compile-extension` run,
    right after decision 38 landed): it failed with `make: *** No rule to
    make target '/build/cmark.c', needed by 'cmark.lo'`. Diagnosed via
    `gh api repos/krakjoe/cmark/git/trees/v1.2.0` against the actual
    committed `compile/extensions/cmark/` tree: the vendored source was
    genuinely **incomplete** — missing the root `cmark.c`/`php_cmark.h`
    and roughly a dozen `src/*.c`/`.h` files (`visitor.c`, `render.c`,
    `parse.c`, `iterator.c`, `cql*.c`, every header but a handful) — never
    actually exercised via `compile-extension` before now, since decision
    35's own end-to-end verification was for the `mode: static` path
    (fresh `wget` + patches in `compile/php/Dockerfile`), a completely
    different code path from `mode: shared`'s pre-vendored
    `compile/extensions/cmark/` directory. Started re-vendoring it
    correctly per the directory's own `PROVENANCE.md` recipe (fresh v1.2.0
    download, apply all 5 patches, copy the complete tree) — but the user
    interrupted mid-fix with "on s'en fou pas mal de CMARK en fait" (we
    don't really care about cmark), then "on va carrément l'enlever du
    build" + "et même de la matrice" (remove it from the build entirely,
    and from matrix.json too).
    - Removed: `compile/extensions/cmark/` (the incomplete vendor
      directory), `patches/cmark/` (all 5 patches — their write-up already
      lives in decision 35's text, not lost), `compile/libcmark/` (the
      plain `commonmark/cmark` C library Dockerfile — only ever consumed
      by `ext/cmark`, nothing else references it), `packages/phpext-cmark/`
      (its README).
    - `config.yaml`: the `cmark: { mode: shared, ... }` entry deleted.
    - `matrix.json`: `libraries.cmark` and `extensions.cmark` entries
      deleted (the latter carried decision 35's five-patch write-up as a
      `note` field — redundant with CLAUDE.md, not lost).
    - `compile/php/Dockerfile`: `ARG CMARK_EXT_VERSION`, the
      `wget`+`git apply` block downloading/patching `ext/cmark`,
      `COPY ./compile/libcmark/`, `ARG WITH_CMARK`, and the
      `--with-cmark`/`--without-cmark` configure-flag block all removed.
      `mdhtml`/`libcmark-gfm` (decision 37) — a completely independent
      library and extension, despite the similar name — is untouched, per
      the user's explicit confirmation ("mdhtml devra faire parti du build
      static comme présentement").
    - `compile/build.js`: the `WITH_CMARK`/`CMARK_EXT_VERSION` build-arg
      forwarding removed (both flags cli.mjs no longer generates).
    - `compile/Makefile`: `CMARK_VERSION` variable, the `libcmark_jspi`
      target, and its `all_jspi`/`clean`/`clean-libcmark` references all
      removed.
    - `compile/cli.mjs`: `cmark` removed from `IMPLEMENTED_EXTENSIONS` and
      `LIB_TARGETS_BY_EXTENSION`; the `--CMARK_EXT_VERSION=...` arg push
      removed.
    - Verified with `--dry-run` (no Docker): the static `build` command's
      resolved `--build-arg` list no longer mentions `WITH_CMARK`/
      `CMARK_EXT_VERSION` and still correctly includes
      `WITH_MDHTML`/`MDHTML_EXT_VERSION`; `compile-extension --dry-run`
      now only lists `kirigami_abi_probe` (internal fixture) and `sodium`
      — no `cmark`. `node --check` on every edited `.mjs`/`.js` file passed.
    - **`sodium`'s real build succeeded in the same background run** (ran
      before `cmark` failed, alphabetically first in `config.yaml`):
      `packages/phpext-sodium/` now has a real `package.json`
      (`version: "0.1.0"`, `kirigami: { type: "extension", phpVersions:
      ["8.5"], minVersion: "8.5.10", vendorLib: { name: "libsodium",
      version: "1.0.22" }, buildHash: "ed838c36..." }`), `manifest.json`,
      `.buildhash`, and `sodium-php8.5-jspi.so` — the first real,
      non-scratchpad confirmation that decision 38's whole mechanism
      (hash-driven versioning, `kirigami` metadata) works end-to-end, not
      just in a no-Docker simulation.
    - **Not committed yet** as of this writing — the user immediately
      pivoted to "on va faire les shared des autres extensions built-in"
      (make shared-mode versions of the other already-static extensions),
      a separate, bigger follow-up not yet scoped.

41. **`sockets` added to the static core build, not `mode: shared`
    (2026-09-15).** Follow-up to decision 40: the user asked to build
    `mode: shared` packages for the other `mode: off`, not-yet-wired
    extensions (`pdo_mysql`, `pgsql`, `sockets`, `gmp`, `ldap`, `tidy`,
    `intl`, etc. — deliberately *not* the already-static ones like
    curl/gd/imagick/openssl/xml, which stay static exactly as they are —
    "REGARDE, curl, gd, imagick, openssl, xml sont DÉJÀ STATIQUES"). Checked
    `ext/sockets/config.m4` (`PHP-8.5.10` tag) first: `PHP_ARG_ENABLE`
    only, no external library, no `PHP_ADD_EXTENSION_DEP` — it compiles
    from php-src's own bundled sources with nothing but a configure flag,
    the same shape as `exif`/`mbstring`/`mysql` in
    `IMPLEMENTED_EXTENSIONS`'s existing "no vendored lib" group. The user's
    call: an extension this cheap (no dependency, no extra build weight)
    should just be static, not opt-in — `mode: shared` is for extensions
    that actually cost something to include. Wired exactly like
    `exif`/`mbregex`'s existing `--enable-x`/`--disable-x` blocks:
    `compile/php/Dockerfile` (`ARG WITH_SOCKETS` + a new "Add sockets if
    needed" block), `compile/build.js` (`getArg('WITH_SOCKETS')` forwarded
    as a `--build-arg`; no yargs `.option()` or `platformDefaults` entry
    added, since `WITH_YAML`/`WITH_MDHTML` already established this isn't
    required for `getArg()` to work — `cli.mjs` always passes the flag
    explicitly), `compile/cli.mjs`'s `IMPLEMENTED_EXTENSIONS` map
    (`sockets: 'WITH_SOCKETS'`), `config.yaml` (moved from the long
    `mode: off` list into the static group). Verified with `--dry-run`
    (no Docker): the resolved `build.js` invocation now includes
    `--WITH_SOCKETS=yes` in the right place.
    - **Still not started**: the actual `mode: shared` extensions from the
      `off` list this whole detour was about. `mysqli` was floated as a
      candidate next — real research done, not yet built: its
      `config.m4` (`PHP-8.5.10` tag) needs no external C library (it uses
      the bundled `mysqlnd` driver, same as the static build's own
      `WITH_MYSQL` block already does), but it does declare
      `PHP_ADD_EXTENSION_DEP(mysqli, mysqlnd)` — mysqlnd is a real,
      separate Zend module (its own `MINIT`, not just a linkable library),
      so a working `mode: shared` `mysqli` needs `mysqlnd` built and
      loaded as *its own* shared extension first. Checked
      `@php-wasm/compile-extension`'s README "Dependencies" section: it
      only documents linking external C libraries (`.a` archives via
      `--extra-cflags`/`--extra-ldflags`), nothing about one shared
      PHP-extension module depending on another being loaded first — this
      would be a genuinely new, unvalidated pattern (two coordinated
      `@kirigami/phpext-*` packages, `mysqlnd` then `mysqli`, relying on
      Emscripten's side-module dynamic linker resolving `mysqli`'s
      imports against symbols `mysqlnd`'s side module previously
      exported/imported — plausible by analogy with how a real ELF `.so`
      loaded earlier makes its symbols visible to one loaded after it, but
      unconfirmed). Recommended validating the simpler, already-proven
      "off → shared, no dependency" pattern first (a candidate like `ftp`
      or `gettext` — though `gettext` turned out to need `libintl`, a real
      external lib, so not as trivial as first assumed either) before
      attempting the two-extension `mysqlnd`+`mysqli` pair — not yet
      decided by the user which order to actually go in.

42. **`ftp` and `mysqli`+`mysqlnd` built as real `mode: shared` packages;
    a genuine `@kirigami/php-wasm`-side auto-load contract designed and
    generated; the actual runtime blocker turned out to be decision 32's
    already-known bug, not a new one (2026-09-15).**
    - **`ftp`**: vendored complete from the `PHP-8.5.10` tag (`gh api
      repos/php/php-src/git/trees/<sha>` used to get the real, complete file
      list first — after decision 40's cmark incident, no more guessing at
      what files an extension needs). No external library
      (`--with-ftp-ssl` defaults off since `$PHP_OPENSSL` is unset in an
      isolated `compile-extension` build). Compiled successfully on the
      first try.
    - **`mysqlnd`+`mysqli`**: also vendored complete from the same tag.
      `ext/mysqlnd`'s own file is `config9.m4`, not `config.m4` (a
      php-src-tree-only numbered-fragment convention for `buildconf`) —
      renamed for `phpize` to find it. `mysqlnd` built with
      `--disable-mysqlnd-compression-support` (skips a zlib dependency) and
      SSL support off by default (skips an openssl dependency) — both
      avoidable for this pilot. `mysqli.c`'s `#include "ext/mysqlnd/*.h"`
      (its own compile-time dependency on mysqlnd's headers) resolves
      because `@php-wasm/compile-extension`'s build environment only
      installs a *minimal* php-src tree (confirmed in its own README's
      troubleshooting section) — `ext/mysqlnd/` isn't in it since mysqlnd
      is itself optional. Fixed by copying mysqlnd's complete header set
      (not its `.c` files — never referenced by mysqli's own
      `PHP_NEW_EXTENSION` list) into `compile/extensions/mysqli/ext/
      mysqlnd/`, the exact relative path mysqli's own quoted `#include`s
      expect. Both compiled successfully.
    - **User decision: mysqlnd+mysqli ship as ONE npm package, not two** —
      asked directly rather than assumed. Checked the actual
      `php-extension-manifest-schema.json` first: it has no field for "load
      this other extension first," so there's no way to express the
      dependency at the manifest level alone. Implemented as: `mysqlnd`
      marked `internal: true` in `config.yaml` (never published on its
      own — reusing the existing internal-fixture mechanism, decision 31)
      and `mysqli` given a new `bundleExtensions: [mysqlnd]` field.
      `cli.mjs`'s `runCompileExtensionCommand()` refactored
      (`buildExtensionArgs()` / `buildOneExtensionArtifact()` extracted)
      so a "primary" extension can trigger building each of its
      `bundleExtensions` into its *own* output directory first, renaming
      the tool's always-`manifest.json` output to `manifest-<name>.json`
      each time so multiple manifests coexist in one package directory
      without clobbering each other. A full (`--name`-less) run now
      excludes any extension referenced by another's `bundleExtensions`
      from its own top-level build (redundant Docker time otherwise) while
      keeping it directly targetable (`compile-extension mysqlnd`) for
      isolated debugging. `computeBuildHash()` widened to hash every
      `manifest*.json`, not just the literal `manifest.json`.
      `extensionPackageJson()`'s `kirigami` metadata gained a `bundles`
      field (`['mysqlnd']`) recording the load order. Verified end-to-end
      with a real build: `packages/phpext-mysqli/` came out with
      `manifest-mysqlnd.json` + `manifest.json` + both `.so` files + one
      `package.json` correctly listing `kirigami.bundles: ["mysqlnd"]`.
    - **The actual `@kirigami/php-wasm` auto-load contract, designed and
      generated now rather than left entirely to decision 39's future
      "kirigami repo, not started" scanner** — the user described the
      intended flow directly: "php-wasm load => glob les packages
      d'extensions disponibles -> call register -> le register retourne
      les so à monter et le ini à ajouter -> php-wasm load les fichiers
      dans sa vm et ajoute les infos ini." Implemented as a generated
      `index.js` in every non-internal shared-extension package, exporting
      `async function register(phpVersion)`: reads the package's own
      manifest(s) in `kirigami.bundles` order, resolves each via
      `@php-wasm/universal`'s `resolvePHPExtension()`, and returns the
      resolved array ready for `withResolvedPHPExtensions()` — the future
      `@kirigami/php-wasm` scanner (kirigami repo, still not started) needs
      only `glob('node_modules/@kirigami/phpext-*')` + `import(pkg).then(m
      => m.register(phpVersion))`, no per-package knowledge of bundled
      manifests or load order required. `extensionPackageJson()` gained
      `type: "module"`, `main: "index.js"`, and a real
      `dependencies: { "@php-wasm/universal": "3.1.53" }` (pinned to match
      `compile/package.json`'s own `@php-wasm/compile-extension` version,
      decision 30) — the first time a phpext package has had a real
      runtime dependency. **Real, load-bearing finding while writing this**:
      `resolvePHPExtension()`'s `format: 'manifest'` + `manifestUrl` path
      goes through `fetch()` internally, and Node's native `fetch()`
      (undici) does not support `file://` URLs — confirmed by hitting
      `TypeError: fetch failed ... not implemented... yet...` directly.
      Since an installed npm package's files are always on local disk
      anyway, `register()` was written to read `manifest*.json` and `*.so`
      bytes itself via `node:fs` and call `resolvePHPExtension()` with
      `format: 'so'` directly — sidesteps the broken path entirely rather
      than working around it with a custom `fetch`. This is a real
      constraint worth remembering for any other code in this ecosystem
      that might reach for `format: 'manifest'` + a `file://` URL.
    - **The runtime smoke test (real goal: does `mysqli.so` actually load
      with `mysqlnd.so`'s symbols resolved?) never got that far — both
      failed for a different, already-documented reason.** Wrote a real
      `@php-wasm/universal` loader test (same technique as decision 30,
      using the cached `php-wasm-universal-3.1.53.tgz` tarball from that
      earlier session rather than re-fetching) against our own
      `node-builds/8-5/php_8_5.wasm`. Both `mysqlnd.so` and `mysqli.so`
      (and, presumably, `sodium.so` too — same shape) failed to load with
      the exact `LinkError: ... "__stack_pointer": imported mutable global
      must be a WebAssembly.Global object` decision 32 already diagnosed
      months ago for sodium — meaning the mysqlnd→mysqli dependency
      question is **still open**, blocked behind this pre-existing,
      unrelated core bug. Checked `compile/php/Dockerfile`'s final `emcc`
      link step directly: `-Wl,--export=__stack_pointer` was present (the
      half of decision 32's candidate fix that must have been applied in
      an earlier, undocumented session) but `-Wl,--export=__table_base`
      was still missing — exactly the gap decision 32 and decision 35's
      "discovered while investigating" note both flagged and never
      resolved. Added the missing line. This requires a full `php.wasm`
      core rebuild to test (this pipeline's slowest step) — explicitly
      confirmed with the user before spending it (disk space is no longer
      the constraint it was per the `project-disk-space-resolved` memory,
      but the RAM/build-time cost is still real). Rebuild kicked off in
      the background; not yet finished as of this writing. Once it lands,
      re-run: (1) the sodium runtime test that's been blocked since
      decision 32, (2) the real mysqlnd→mysqli dependency question this
      whole detour was actually trying to answer, (3) regenerate
      `packages/phpext-{sodium,ftp,mysqli}/` with the new `index.js` (none
      of the three have it yet — added to the generator after their most
      recent real builds).
    - **First rebuild attempt failed — on a completely different, brand-new
      bug**: `ext/sockets` (just added to the static core, decision 41)
      failed to compile: `error: incomplete definition of type 'struct
      sockaddr_ll'` and `error: use of undeclared identifier 'SKF_AD_OFF'`/
      `'BPF_RET'`/etc. (32 error lines total, all in `sockets.c`, confirmed
      the only file affected by grepping the whole build log). Root cause:
      `ext/sockets/sockets.c` guards its `AF_PACKET` (raw Ethernet socket
      address handling, `struct sockaddr_ll`) and `SO_ATTACH_REUSEPORT_CBPF`
      (classic BPF socket-filter attachment) code paths with plain `#ifdef
      AF_PACKET` / `#ifdef SO_ATTACH_REUSEPORT_CBPF` — true on real Linux
      that both the named constant AND the associated kernel struct/macros
      (`struct sockaddr_ll`, `SKF_AD_OFF`, `BPF_RET`, `BPF_A`, `BPF_LD`,
      `BPF_W`, `BPF_ABS`, `SKF_AD_CPU`, `SKF_AD_QUEUE`, `struct
      sock_filter`, `struct sock_fprog`) are always defined together, but
      Emscripten's partial POSIX emulation defines the two named constants
      alone without any of the rest. Fixed with a new
      `patches/sockets/wasm-af-packet-cbpf.patch` — all 11 guard sites
      (9× `AF_PACKET`, 2× `SO_ATTACH_REUSEPORT_CBPF`) changed to also
      require `!defined(__EMSCRIPTEN__)`, disabling both entire feature
      blocks under this pipeline's only real target rather than chasing
      individual missing macros one at a time (this pipeline is
      Emscripten-only, so the condition is unconditionally correct here,
      not just a narrow workaround). Verified the same way as every cmark
      patch (decision 35's lesson): generated via a real `diff -ru`
      between a pristine `PHP-8.5.10` download and a hand-edited copy,
      then `git apply --no-index --check` against a **third**, separately
      downloaded pristine copy, confirmed byte-identical (modulo CRLF)
      to the intended result before trusting it. Wired into
      `compile/php/Dockerfile` right after the existing `php8.5.patch`/
      `apply-mysqlnd-patch.sh` application, `cd`'d into `php-src` first
      (this patch's `-p1` path prefix is `ext/sockets/sockets.c`, unlike
      the main patch's `php-src/ext/...` — applied from a different
      directory accordingly). `patches/README.md` updated: its "not wired
      up yet" status was stale even before this (cmark already wired
      `patches/cmark/` the same way, just never reflected here) —
      corrected to describe the real, current state (wired for php-src
      extensions via `compile/php/Dockerfile`, still not wired for the
      third-party lib Dockerfiles). Second rebuild (with this patch)
      kicked off; also not yet finished as of this writing.

43. **`jsonk` (php-kirigami/php-jsonk, the user's own project) and `apcu`
    (krakjoe/apcu) wired into the static core build (2026-09-16). Neither
    has been build-tested yet — this decision documents the wiring and a
    real, verified structural constraint found while scoping it, not a
    finished/proven result.**
    - **The literal request — "disable json by default so jsonk replaces
      it" — is not achievable, and jsonk's own design doesn't need it to
      be.** Verified directly, not assumed: `php-src`'s real
      `ext/json/config.m4` (`PHP-8.5.10` tag) takes no `PHP_ARG_ENABLE` at
      all — there is no `--disable-json` flag in PHP 8.5, ext/json is an
      unconditional core extension. Separately, `jsonk.c`'s own
      `zend_module_dep` declares `ZEND_MOD_REQUIRED("json")` — jsonk
      refuses to load unless `json` is already loaded, because its
      replacement mechanism clones the `zend_internal_function` struct
      PHP's own `zend_register_functions()` built for `json_encode`/
      `json_decode` (via jsonk's aliased `jsonk_json_encode_replacement`/
      `jsonk_json_decode_replacement` functions) and splices it into the
      `json_encode`/`json_decode` slots of the *same* function table
      ext/json's MINIT already populated — the extension's own doc
      comments spell this out. So "json" can't be disabled, and jsonk
      itself would refuse to load if it somehow were. What actually
      realizes "jsonk replaces json" is jsonk's own existing
      `jsonk.replace_json_functions` ini setting (`STD_PHP_INI_BOOLEAN`,
      upstream default `"0"`) — flipped to default `"1"` for this build via
      a one-line `/root/replace.sh` sed in `compile/php/Dockerfile` (not a
      `patches/jsonk/*.patch` file — too small a literal-string change to
      warrant one, unlike the multi-file cmark/sockets patches). From
      userland, `json_encode()`/`json_decode()` are transparently
      jsonk-backed from process start; `json_last_error()`/
      `json_last_error_msg()` stay compatible since jsonk's replacement
      wrappers deliberately write to ext/json's own `JSON_G(error_code)`,
      not jsonk's separate error state.
    - **jsonk wiring**: `--enable-jsonk` (config.m4 requires
      `vendor/simdjson/simdjson.h` and `vendor/yyjson/yyjson.h` to already
      exist, checked before `buildconf`). Both are vendored as flat
      amalgamated files with no build step of their own (simdjson.h +
      simdjson.cpp as GitHub release assets; yyjson.h + yyjson.c fetched
      directly from the tagged ref) — fetched straight into
      `ext/jsonk/vendor/{simdjson,yyjson}/` by `compile/php/Dockerfile`
      itself, the same layout jsonk's own `vendor/build/stage.sh` produces
      for a native dev build, but *not* via a separate
      `compile/lib*/Dockerfile` + Makefile target the way every other
      third-party lib in this pipeline is built — there's no real
      compilation to do ahead of time, so it didn't need one. `matrix.json`
      gained `libraries.simdjson`/`libraries.yyjson` entries anyway (pinned
      `4.6.11`/`0.13.0`, matching jsonk's own matrix.json) purely so
      `getMatrixVersion()` has one place to resolve them from, and
      `extensions.jsonk` (repo, sourceTemplate, `v0.1.0`), following the
      yaml/mdhtml pattern (CLAUDE.md decisions 34/37). `cli.mjs`'s
      `IMPLEMENTED_EXTENSIONS` gained `jsonk: 'WITH_JSONK'`; `build.js`
      forwards `WITH_JSONK`/`JSONK_EXT_VERSION`/`SIMDJSON_VERSION`/
      `YYJSON_VERSION` as `--build-arg`s (decision 34's lesson: both files
      need updating, or a flag cli.mjs generates gets silently dropped).
      No `LIB_TARGETS_BY_EXTENSION` entry needed (no compile/Makefile
      target exists for it).
    - **This is the pipeline's first C++ extension** (config.m4 calls
      `PHP_REQUIRE_CXX()`, sets `CXXFLAGS="$CXXFLAGS -std=c++17"` —
      simdjson 4.x requires C++17). De-risked, not just hoped: JSPI mode's
      existing `-fwasm-exceptions -sSUPPORT_LONGJMP=wasm` flags (decision
      6, always on in this pipeline) mean real C++ exception handling is
      already available if simdjson's amalgamation needs it internally;
      confirmed `jsonk_decode.cpp` (the one C++ translation unit in the
      extension) itself only uses simdjson's non-throwing `.get()`-style
      DOM API (`el.get_bool().get(b)`, etc.), not `throw`/`catch`, by
      reading the actual file rather than assuming. Left
      `PHP_ADD_LIBRARY(stdc++, 1, JSONK_SHARED_LIBADD)` (config.m4's own
      native-Linux workaround for a real RTTI-symbol double-free bug on
      real Linux builds) untouched rather than guessing whether an
      Emscripten equivalent is needed — this pipeline's hand-rolled final
      `emcc` link step doesn't consume automake `SHARED_LIBADD` variables
      the way a normal native build's linker invocation would, so whether
      this matters at all here is unconfirmed and left for the first real
      build to reveal, per this project's own established methodology
      (fix concretely-observed build failures, don't pre-patch guesses).
    - **apcu wiring**: `--enable-apcu`, no external lib, no vendoring —
      pure php-src-bundled-style sources fetched fresh like
      exif/mbstring/sockets. `matrix.json`'s pre-existing `extensions.apcu`
      entry (a data-only placeholder ported from php-static-autobuilder,
      decision 24/28, carrying the user's own earlier note: "on va
      probablement la mettre dans le core si la mémoire persiste dans la
      vm de php-wasm" — a direct prediction of this exact moment) gained
      real `repo`/`sourceTemplate`/`versions` fields matching the
      yaml/mdhtml shape. **Real bug in that placeholder found and fixed
      while wiring it**: it listed `dependencies: ["igbinary"]`, ported
      from php-static-autobuilder's own matrix.json — checked against
      apcu's actual, current `config.m4` (`krakjoe/apcu` tag `v5.1.24`+)
      and found no `PHP_ADD_EXTENSION_DEP` on igbinary at all; igbinary is
      only an optional faster serializer APCu detects at runtime if
      present, never a hard configure dependency. Removed the field rather
      than carry the wrong claim forward.
    - **Checked apcu's own cross-compile safety, learning from the cmark
      incident (decision 34)**: apcu's `config.m4` runs two `AC_RUN_IFELSE`
      pthread-capability probes (rwlock support, mutex `PTHREAD_PROCESS_
      SHARED` support) that *execute* a compiled test binary — the same
      shape that broke `ext/cmark`'s `AC_TRY_RUN` under cross-compilation
      and needed a real patch. Read the actual macro calls this time
      before assuming a patch was needed again: both already supply a real
      4th ("action if cross-compiling") argument that assumes success
      (native rwlock/mutex support, `-lpthread` added) — so, unlike cmark,
      **no patch is needed for configure to complete**. What's genuinely
      unconfirmed (needs a real build, not more reading): whether
      Emscripten's non-`-pthread` build actually resolves the
      `pthread_mutex*`/`pthread_rwlock*`/`PTHREAD_PROCESS_SHARED` symbols
      that "assumed yes" path links against, or whether the shared-memory
      cache (`apc_mmap.c`/`apc_shm.c`, `mmap`-based by default via
      `--disable-apcu-mmap`'s inverse) runs into the same "Emscripten's
      mmap/munmap support is incomplete" issue this pipeline already works
      around for its own core (`compile/php-wasm-memory-storage`, see the
      Extraction/Context sections) — a real, known risk flagged here
      rather than discovered blind.
    - **The actual open question this was added to answer, restated
      precisely**: whether APCu's shared cache genuinely persists across
      separate `PHP.run()` calls within the *same* `php-wasm` runtime
      instance (MINIT runs once per process; RINIT/RSHUTDOWN per request
      boundary) — plausible in principle since APCu's cache lives in
      module-global state set up at MINIT, not torn down per-request, but
      unconfirmed until actually tested against a real build.
    - **✅ Real build attempted, two real bugs found and fixed (2026-09-16),
      third rebuild in progress as of this writing.** `node compile/cli.mjs
      --quiet --dry-run` had already confirmed the full arg chain resolves
      correctly; the first *real* (non-dry-run) build surfaced what
      dry-run couldn't:
      1. **jsonk's `vendor/simdjson/simdjson.h` existence check resolves
         against the wrong directory for this repo's static, whole-tree
         build**: `configure: error: simdjson not found at
         vendor/simdjson/simdjson.h -- run vendor/build/stage.sh first`.
         Root cause: `config.m4`'s guard (and its `PHP_ADD_INCLUDE`/
         `PHP_NEW_EXTENSION` source paths) are plain relative paths,
         correct for jsonk's own documented standalone `phpize` build
         (cwd == `ext/jsonk` when `configure` runs there) but not for a
         full-`php-src`-tree build, where the *one* generated `./configure`
         script runs with cwd == `php-src/` — so the same relative path
         resolves to `php-src/vendor/simdjson/simdjson.h` instead. Fixed
         *here* (not in jsonk's own `config.m4`) by also copying the
         already-vendored `ext/jsonk/vendor/{simdjson,yyjson}/` directories
         to `php-src/vendor/{simdjson,yyjson}/` right after fetching them —
         covers the guard check and, defensively, any other relative
         reference the same macros might make, without knowing for certain
         which of PHP's build macros resolve relative to `$ext_srcdir` vs.
         the tree root. Documented on the jsonk side too, per the user's
         request: `php-jsonk`'s own `CLAUDE.md` (decision 20) and
         `README.md` ("Building from source") now describe this exact
         caveat for anyone else statically linking jsonk into a full
         `php-src` tree, without changing jsonk's own code (documentation
         only, as asked) — the root-cause fix (`config.m4` using
         `$ext_srcdir`-relative paths) is noted there as a follow-up, not
         attempted.
      2. **apcu's `apc_shm.c` fails the final `php.wasm` link with
         `undefined symbol: shmget/shmat/shmctl/shmdt`** — real SysV IPC
         syscalls that Emscripten's musl-derived libc *declares* (so the
         compile step itself doesn't fail) but never implements, and this
         repo's final link always uses `-s ERROR_ON_UNDEFINED_SYMBOLS=1`.
         Confirmed via `krakjoe/apcu`'s real `apc_shm.c`: unlike
         `apc_mmap.c`, it has no `#ifdef APC_MMAP`-style guard at all —
         `apc_shm_attach()`/`apc_shm_detach()` are unconditionally compiled
         and referenced by `apc_sma.c`'s runtime mmap-vs-shm dispatch, even
         though the default build (`--enable-apcu-mmap`, on by default,
         decision 43's earlier text already noted this) should never
         actually call them. Fixed with a new
         `patches/apcu/apcu-emscripten-shm-stub.patch` (first entry in that
         folder) wrapping the four-syscall implementation in
         `#if !defined(__EMSCRIPTEN__)` and replacing it with a
         `zend_error_noreturn`-on-attach / no-op-on-detach stub otherwise —
         a fatal error if ever actually reached is the honest behavior,
         since reaching it would mean the mmap backend was unexpectedly
         bypassed. Verified the same way as every other patch in this repo
         (decision 35's lesson): generated via a real `diff -ru` against a
         pristine `v5.1.28` download, then `git apply --no-index --check`
         against a **third**, separately downloaded pristine copy before
         trusting it. Wired into `compile/php/Dockerfile` right alongside
         the existing `patches/sockets/` application (same `cd php-src &&
         git apply --no-index` step), unconditionally (matches sockets'
         own always-applied pattern, since `ext/apcu`'s source is always
         fetched regardless of `WITH_APCU`, only `--enable`/`--disable` is
         conditional). `patches/README.md` updated to list it as a second
         real example.
      - Both bugs were found by running the actual pipeline, not by static
        analysis, consistent with every other "real build surfaces a real
        bug" entry in this file (decisions 17-20, 24-26, 32, 34-35, 42).
        The apcu-persistence question (previous bullet) and jsonk's real
        C++ link are still open until this third rebuild (with both fixes
        applied) either succeeds or surfaces the next real issue.
      3. **Third real bug: a pipeline-wide, cross-cutting build-environment
         gap, not a jsonk or apcu bug at all — `em++` was never patched the
         same way `emcc` was.** The third rebuild attempt got past both
         fixes above and reached the final `emcc`/`wasm-opt` link step,
         which failed with `[wasm-validator error in function
         zif_jsonk_decode/zif_jsonk_validate/zif_jsonk_json_decode_
         replacement] call param types must match` — every call site to
         `jsonk_decode_impl` (the one function defined in `jsonk_decode.cpp`,
         the pipeline's first real C++ translation unit), always on
         argument index 2 and 3 (`flags`/`depth`, both `zend_long`).
         Diagnosed concretely, not guessed: `zend_long` is `int64_t`
         exactly when `__x86_64__` is defined (confirmed straight from
         `Zend/zend_long.h`'s real `PHP-8.5.10` source — `#if
         defined(__x86_64__) || ... # define ZEND_ENABLE_ZVAL_LONG64 1`),
         and this whole pipeline relies on `-D__x86_64__` being injected
         into every compile via the `EMCC_FLAGS` env var
         (`compile/base-image/Dockerfile`'s `emcc-for-php-wasm.sh` wrapper).
         Spun up a throwaway container from the already-built
         `kirigami-php-wasm:base` image to check directly: `emcc` had been
         replaced with the wrapper (dated to this repo's own build), but
         `em++` was still the untouched original emsdk file (dated to the
         emsdk release itself) — `em++` is a **genuinely separate
         script/entry point** in emsdk, not a symlink to `emcc`, and the
         original patch (whenever it was written, predates this session)
         only ever touched `emcc`. So every `.cpp` file compiled in this
         pipeline has *always* silently missed `EMCC_FLAGS` (and
         `EMCC_SKIP`) entirely — `-D__x86_64__` included — meaning
         `jsonk_decode.cpp` was compiled with `zend_long` as plain
         `int32_t` (the non-`__x86_64__` branch), while every C caller
         (correctly compiled with `-D__x86_64__` via the patched `emcc`)
         assumed `int64_t` — a real 32-vs-64-bit ABI mismatch between one
         C++ translation unit and the rest of the build, caught by
         `wasm-opt`'s post-link validator (not a silent corruption, at
         least — Binaryen refused to emit a binary it couldn't validate).
         This was invisible before now simply because jsonk is this
         pipeline's first real C++ extension — nothing before it (GD,
         ImageMagick, curl, etc.) is C++, and `intl`'s own "this is used by
         intl (which links C++ code)" comments elsewhere in
         `compile/php/Dockerfile` were never actually exercised (intl is
         still `mode: off`). **Not a jsonk bug** — confirmed by reading
         jsonk_decode.cpp/.h side by side, the declaration and definition
         agree exactly; the user's own initial framing (fix it in
         `php-jsonk`, commit, tag) doesn't apply here, no change was made
         to that repo for this one. Fixed in `compile/base-image/Dockerfile`
         (the "Patch emcc..." `RUN <<EOF` block now also backs up
         `em++`/`em++.py` to `em++2`/`em++2.py` and installs the same
         wrapper as `em++`) and `compile/base-image/emcc-for-php-wasm.sh`
         (the hardcoded final `.../emcc2 "${args[@]}" ...` call changed to
         `"$(dirname "$0")/$(basename "$0")2" ...` — resolves which
         original binary to re-invoke from its own invocation name, so one
         script file correctly serves as both `emcc` and `em++`).
         **Consequence**: this is a `base-image` change, so the next build
         must rebuild `kirigami-php-wasm:base` for real (not from Docker's
         cache) — and every lib image built `FROM` it loses its own cache
         too, even though none of their actual `RUN` steps changed
         (Docker's layer cache is keyed by lineage, not just content) — a
         real, one-time cost of fixing something this early in the
         pipeline, not a sign anything else is wrong. A fourth rebuild
         (all three fixes applied) is in progress.
      4. **Fourth real bug, also unrelated to jsonk/apcu/em++: `WITH_OPCACHE
         = no` was never actually build-tested since PHP 8.5's "OPcache
         made non-optional" RFC, and it's missing `--disable-opcache-jit`.**
         The fourth attempt got past the em++ fix and a transient network
         502 fetching libwebp (unrelated, just retried), then failed
         *earlier* than the jsonk/apcu issues, inside `emmake make -j14
         libphp.la` itself: `error: "JIT not supported on this platform"`
         from `ext/opcache/jit/zend_jit.h`, compiling
         `zend_accelerator_module.c`/`zend_file_cache.c`/`zend_persist.c`.
         The user asked directly whether an existing opcache patch had been
         erased — checked concretely (git log, `patches/`, `git diff`):
         nothing was touched or deleted; `jit_stubs.c` (lines ~989-1012,
         present since the original extraction commit) is intact and
         solves a *different* problem (supplying no-op link-time symbols
         for the JIT functions whose real bodies get emptied out by this
         Dockerfile's very first step — used at the *final* `emcc` link,
         never reached yet in this failure). Root cause, confirmed by
         reading `ext/opcache/config.m4` from the exact `php-8.5.10` tag:
         `PHP_ARG_ENABLE([opcache-jit], ..., [yes], [no])` runs
         unconditionally (not gated behind `--enable`/`--disable-opcache`
         at all, matching the RFC), defaults to JIT enabled, and its
         `AS_CASE([$host_cpu], ...)` arch check incorrectly concludes "real
         x86_64" here because the *main* PHP `./configure` invocation
         (unlike every third-party lib Dockerfile's own configure call) is
         never given an explicit `--host wasm32-unknown-emscripten` —
         autoconf's `config.guess` falls back to the actual Docker host's
         real x86_64 CPU. That sets `HAVE_JIT`/`-DIR_TARGET_X64 -DIR_PHP`
         (confirmed present in the real failing compile command in the
         build log) and pulls in `zend_jit.h`, whose own `#if
         defined(__x86_64__) || ...` needs the *compiler* to also define
         `__x86_64__` — true throughout the rest of this pipeline only via
         `EMCC_FLAGS`, which is exported for the `emmake make -j14
         libphp.la` step but apparently doesn't reach this particular
         compile (not fully root-caused *why* — could be it, could be
         something else in this one path — but the fix doesn't require
         knowing: `--disable-opcache-jit` is JIT's own dedicated escape
         hatch, skips the `$host_cpu` check entirely, and this project's
         `WITH_OPCACHE=yes` branch already relies on exactly this flag).
         **Why this is surfacing only now**: the original "full end-to-end
         build succeeded" milestone (Context/Current-status sections)
         predates decision 22, which is what first flipped `opcache` to
         `mode: off` — nothing in this repo's history shows a full rebuild
         with opcache off actually completing since. Fixed by adding
         `--disable-opcache-jit --disable-huge-code-pages` to the `else`
         (opcache-off) branch too, matching what the `yes` branch already
         passes — `compile/php/Dockerfile`'s "Patch OPcache config.m4"
         block. A fifth rebuild is in progress.
      5. **Fifth real bug, also unrelated to jsonk/apcu: only 7 of 22 lib
         targets had decision 24's `STRIP_SO` cleanup, and the base-image
         rebuild's cache invalidation was the first thing to actually
         re-trigger a from-scratch `libpng16` build and expose it.** The
         fifth attempt got past the opcache fix and reached the real
         `sapi/cli/php` link (itself only reachable because `emmake make
         -j14 libphp.la` had failed for an unrelated reason and fallen
         through to `|| emmake make -j14`, the default "all" target —
         `sapi/cli/php` is supposed to be excluded via a `sed`-based
         Makefile-rule removal a few steps earlier, whose own debug `grep`
         output in this build's log shows *why* it's always been a no-op
         for PHP 8.5.10: that Makefile spells the rule via a
         `SAPI_CLI_PATH` variable, never a literal `sapi/cli/php:` line —
         a separate, pre-existing, not-yet-fixed gap, left alone here since
         it isn't what actually broke the build). The real failure: `wasm-
         ld: error: attempted static link of dynamic object
         /root/lib/lib/libpng16.so` — the exact failure shape decision 24
         already diagnosed and fixed generically (`STRIP_SO`, a Makefile
         macro removing stray `.so`/`.so.*` files that upstream `make
         install` steps produce alongside the `.a` this pipeline actually
         wants), but that decision scoped the fix to "only libz_jspi and
         libopenssl_jspi... the only libs actually consumed as a build-time
         dependency by other lib Dockerfiles" — an assumption later
         decisions quietly outgrew (libssh2/nghttp2/libyaml/libsodium/
         libcmark-gfm picked up their own `STRIP_SO` calls over time, per
         `git grep`) but never applied comprehensively, and `libpng16`
         specifically was never covered despite being consumed by both
         `libgd` and `libImageMagick`'s own Dockerfiles *and* swept into
         `compile/php/Dockerfile`'s shared `/root/lib`. This had been
         silently latent because the base image (and therefore every lib
         built `FROM` it) had stayed cached since whenever `libpng16` was
         last actually rebuilt — this session's `em++` fix (bug 3) was the
         first thing to force a real `kirigami-php-wasm:base` rebuild,
         which cascaded into rebuilding `libpng16` for real and finally
         producing the stray `.so` decision 24's original narrow fix never
         protected against. Fixed by extending `STRIP_SO` to all 22 lib
         targets in `compile/Makefile` (was 7) rather than patching
         `libpng16` alone, since the same latent gap applies equally to
         every other still-uncovered target the next time its cache gets
         invalidated.
         - **That fix alone wasn't enough — a sixth attempt hit a second,
           related half of the same bug**: `emcc2: error:
           /root/install/lib/libpng16.so: No such file or directory
           ("/root/install/lib/libpng16.so" was expected to be an input
           file...)`. The `.so` really was gone (`STRIP_SO` worked), but
           `libpng16.la` — libtool's own metadata sidecar, left untouched
           by the original `STRIP_SO` definition — was still sitting next
           to the `.a`, and its `dlname='libpng16.so.16'`/
           `library_names='libpng16.so.16.58.0 libpng16.so.16
           libpng16.so'`/`libdir='/root/install/lib'` fields (verified by
           reading the actual `.la` file) still point at the now-deleted
           `.so`, at a `libdir` that isn't even this shared `/root/lib`
           pool to begin with (it's copied verbatim from wherever
           `libpng16` was originally built, inside its *own* Docker
           container). Libtool, relinking `sapi/cli/php` and noticing the
           `.la` "was moved" (a warning visible in the log for
           `libyaml.la`/`libiconv.la` too), tries to honor that stale
           metadata instead of just using the real `.a` sitting right next
           to it. Fixed by widening `STRIP_SO`'s own definition to also
           `rm -f $(1)/lib/*.la` — one change now protects all 22 targets,
           rather than needing every future `.la`-producing lib to be
           special-cased. Also manually deleted 18 already-stale `.la`
           files (`libcurl`, `libiconv`, `libpng16`, `libsodium`,
           `libssh2`, `libwebp`, `libxml2`, `libyaml`, `nghttp2`,
           `oniguruma`) left over from before this fix existed — Make
           considers their `.a` targets already up to date, so it would
           never have re-run the (now-fixed) recipe to clean them up on
           its own. A seventh rebuild is in progress.
      6. **Sixth real bug, back in jsonk territory: simdjson's own x86 SIMD
         backend detection reaches unavailable real SSE intrinsics under
         Emscripten — fixed for real this time in `php-jsonk` itself, not
         worked around here.** The seventh attempt got past the `.la` fix
         and actually reached `em++` compiling `jsonk_decode.cpp`/
         `vendor/simdjson/simdjson.cpp` — proof bug 3's `em++` patch works
         — then failed on a genuinely new error: `.../compat/emmintrin.h:
         "SSE2 instruction set not enabled"` / the same for `xmmintrin.h`.
         Root cause: simdjson.h sees `__x86_64__` (this pipeline's own
         `-D__x86_64__`, forced solely to make `zend_long` 64-bit, not a
         real-hardware claim) and picks a genuine x86 SIMD implementation,
         pulling in `<emmintrin.h>`; Emscripten's own compat shims for that
         header (confirmed by reading the real file inside a throwaway
         container from `kirigami-php-wasm:base`) guard themselves behind
         `__SSE__`, which clang only predefines given an explicit
         `-msimd128` — never implied by `-D__x86_64__` alone. **Fixed in
         `php-jsonk` itself** (the user, having granted direct edit access
         earlier for the vendor-path issue, explicitly repeated "va
         modifier le source directement... et commit/tag" for this class
         of bug too): `config.m4` now adds `-msimd128` to `CXXFLAGS`,
         guarded to `case $CXX in *em++*)` so it never reaches jsonk's own
         primary native-PECL build target. Committed, version bumped to
         `0.1.1`, tagged `v0.1.1`, and pushed to `php-kirigami/php-jsonk`.
         `matrix.json`'s `extensions.jsonk.versions` here gained `"v0.1.1"`.
      7. **Seventh real bug: `v0.1.1`'s `-msimd128` fix was insufficient —
         it only satisfied `emmintrin.h`'s own guard, then hit
         `<x86intrin.h>`'s further sub-includes.** The eighth attempt
         (v0.1.1) got past the SSE-not-enabled errors but failed on
         `ia32intrin.h`/`ammintrin.h`: undeclared `__builtin_ia32_crc32*`/
         `rdtscp`/`wbinvd` and "This header is only meant to be used on x86
         and x64 architecture" — real x86 CPUID/RDTSC/AMD-only SSE4a
         builtins with no wasm equivalent, confirmed by directly testing
         `em++ -msimd128 -msse4.2 -include x86intrin.h` in a throwaway
         container from `kirigami-php-wasm:base` (still fails, no flag
         combination helps). Diagnosed the real structure by fetching
         simdjson.h/.cpp directly and reading them: simdjson defaults
         `SIMDJSON_IMPLEMENTATION_HASWELL`/`WESTMERE` to "on" whenever its
         own `SIMDJSON_IS_X86_64` is true (unconditionally true here, since
         that's exactly what `-D__x86_64__` triggers), compiling those x86
         backends' source — reachable via `<x86intrin.h>` — even though
         they'd never be selected as the "builtin" implementation without
         real `-mavx2`/etc. **Real fix, verified with standalone `em++ -c`
         tests before touching the real pipeline**: `-DSIMDJSON_
         IMPLEMENTATION_ICELAKE=0 -DSIMDJSON_IMPLEMENTATION_HASWELL=0
         -DSIMDJSON_IMPLEMENTATION_WESTMERE=0` (all three properly
         `#ifndef`-guarded, confirmed compiles clean) plus `-DSIMDJSON_
         EXPERIMENTAL_HAS_SSE2=0` for `simdjson.h`'s own separate SSE2 code
         path (same `#ifndef` shape). **One piece couldn't be fixed via any
         `-D` flag**: `simdjson.cpp`'s `detect_supported_architectures()`
         x86 branch is gated by a *raw*, unguarded `#elif defined(__x86_64__)
         || defined(_M_AMD64)` whose own body re-`#define`s
         `SIMDJSON_IS_X86_64 1` unconditionally — confirmed by testing that
         a command-line `-DSIMDJSON_IS_X86_64=0` gets silently overwritten
         back to `1` by that line. That branch emits real x86 `cpuid`/
         `xgetbv` inline asm. Since `php-wasm-compiler` downloads
         `simdjson.cpp` directly (matrix.json's own `simdjson` entry,
         independent of jsonk's `vendor/build/stage.sh`), the fix for
         *this* one piece lives here, not in `php-jsonk`: a new
         `patches/simdjson/emscripten-skip-x86-cpuid-detection.patch`
         (generated via `diff -ru` against a pristine `v4.6.11` download,
         verified with `git apply --no-index --check` against a fresh
         third copy) adds `&& !defined(__EMSCRIPTEN__)` to that one
         condition, falling through to simdjson's own existing portable
         `instruction_set::DEFAULT` branch. Applied in
         `compile/php/Dockerfile` right after `simdjson.cpp` is fetched
         (`cd .../vendor/simdjson && git apply --no-index`), before the
         copy to `php-src/vendor/` so both locations get the patched file.
         All four `-D` defines went into `php-jsonk`'s own `config.m4`
         (committed, version bumped to `0.1.2`, tagged `v0.1.2`, pushed —
         see that repo's `CLAUDE.md` decision 21's correction for the full
         writeup). `matrix.json`'s `extensions.jsonk.versions` gained
         `"v0.1.2"`; `patches/README.md` updated to list this as a third
         real example. Verified end-to-end via standalone `em++`
         compiles of both `simdjson.cpp` (patched) and a `simdjson.h`-
         including test file, both clean, before spending a ninth full
         pipeline rebuild on it.
    - **✅✅ Ninth rebuild succeeded end-to-end (2026-09-16) — real PHP
      execution smoke-tested, not just a green Docker exit code.** Produced
      `node-builds/8-5/php_8_5.js` (~394KB) + `node-builds/8-5/8_5_10/
      php_8_5.wasm` (~26.5MB, valid `\0asm` + `dylink.0` header). Tested
      with a throwaway `@php-wasm/universal@3.1.53` script (same technique
      as decision 30), against the real build, not a simulation:
      - `get_loaded_extensions()` lists both `jsonk` and `apcu` (alongside
        every other static extension — `yaml`, `mdhtml`, `sockets`, etc.).
      - `jsonk_decode()`/`jsonk_encode()` round-trip correctly.
      - `ini_get('jsonk.replace_json_functions')` reads `1` (the flipped
        default took effect) and plain `json_encode()`/`json_decode()`
        calls are genuinely jsonk-backed — confirmed by output, not just
        by the ini value being set.
      - `apcu_store()`/`apcu_fetch()` work within a single request.
      - **The actual question this whole decision was opened to answer**:
        `apcu_store('persisted_key', ...)` in one `php.run()` call, then
        `apcu_fetch('persisted_key')` in a **separate, later** `php.run()`
        call on the *same* `PHP` instance, correctly returned the value
        set in the first call. **APCu's cache does persist across separate
        `PHP.run()` calls within the same php-wasm runtime instance** —
        confirmed real, not theoretical. The earlier open risk (does
        Emscripten's mmap backend even give APCu a stable arena) turned
        out to be a non-issue in practice.
      - **Unexpected, real finding while checking this**: `Zend OPcache`
        already appears in `get_loaded_extensions()`, and
        `ini_get('opcache.enable')` reads `1` with a working
        `opcache_get_status()` — **despite `config.yaml`'s `opcache: {
        mode: 'off' }`**. Root cause is exactly decision 43 bug 4's own
        finding: PHP 8.5's "OPcache made non-optional" RFC means the
        accelerator core (and its default-on `opcache.enable`) is always
        active regardless of `--disable-opcache` — that flag (and the
        `--disable-opcache-jit --disable-huge-code-pages` this decision
        added to it) only ever controlled the JIT/huge-pages sub-features,
        never the base accelerator's own on/off state. **This directly
        answers the user's "once the build succeeds, we reactivate
        opcache" follow-up (2026-09-16): there is nothing to reactivate,
        it was never actually off.** `config.yaml`'s `opcache: mode: off`
        and its own comment ("Be sure to disable OPcache if not needed")
        are now misleading for this PHP version and should be revisited —
        not done in this pass, flagged here for the next time opcache
        itself is touched.
      - This closes out the entire jsonk+apcu addition (decision 43): all
        seven real bugs found during this session's rebuild cycle (jsonk
        vendor-path resolution, apcu SysV shm, the pipeline-wide `em++`
        patching gap, opcache-jit, `STRIP_SO` coverage + stale `.la`
        metadata, and simdjson's x86 SIMD/cpuid detection — the last one
        needed two separate correction passes) are fixed, committed, and
        verified against a real, successful, fully-tested build.
    - **Queued, explicitly conditional on the apcu-persistence question
      above resolving "yes" (user, 2026-09-16): add `igbinary` (a faster
      binary serializer PECL extension) and configure it as APCu's
      serializer.** APCu detects and uses igbinary automatically when it's
      loaded (no config wiring needed on APCu's side beyond it being
      present — `apcu.serializer` becomes settable once igbinary registers
      itself as a serializer via `php_serialize_register_handler()`).
      "Set it so it's the serializer" most likely means defaulting
      `apcu.serializer=igbinary` for this build (an ini-default override,
      same shape as jsonk's `jsonk.replace_json_functions` flip earlier in
      this decision) rather than leaving it opt-in — to be confirmed with
      the user when this is actually picked up. Not started: no
      `igbinary` entry anywhere yet (`config.yaml`, `matrix.json`,
      `compile/php/Dockerfile`, `cli.mjs`, `build.js`). Deliberately
      sequenced after the apcu-persistence question — no point wiring a
      serializer for a cache that might not actually persist across
      `PHP.run()` calls in the first place (also confirmed relevant on the
      consumer side while discussing this: `../kirigami/packages/php-wasm/
      index.js` keeps `runtime`/`runtimeNetwork` as module-level
      singletons, `_getPHPRuntime()`/`_getPHPRuntimeWithNetwork()` called
      only once — so MINIT genuinely runs once per Node process there,
      the actual precondition APCu's persistence needs; the open question
      is now narrowed to whether Emscripten's mmap gives APCu a stable
      arena in the first place, not whether the same runtime instance is
      reused).
    - **Queued, not started**: the user separately floated ("il faudrait
      éventuellement que le Dockerfile soit généré par la config et la
      matrice") generating `compile/php/Dockerfile` itself from
      `config.yaml` + `matrix.json` instead of hand-maintaining ARG/RUN
      blocks per extension (this decision added four more such blocks by
      hand, the same way decisions 34/37/41/42 each did). Explicitly not
      attempted in this session — "éventuellement" (framed as a future
      possibility, not an immediate instruction) and a Dockerfile-generator
      is a real architectural change to a 900+-line, build-critical file
      that deserves its own dedicated pass rather than being bundled into
      an unrelated extension-wiring change. Worth scoping later: it would
      need to account for the file's current mix of purely mechanical
      blocks (jsonk/apcu/yaml/mdhtml/sockets's own `--enable-x`/`--disable-x`
      pairs are already near-identical boilerplate) and genuinely bespoke
      ones (GD's PHP-version-conditional external-vs-bundled branch, the
      libxml2 PHP<7.4 sed patches, imagick's PHP7-vs-8 buildconf branch)
      that a naive template probably can't collapse safely.

44. **A real, pre-existing (unrelated to jsonk/apcu) `imagick` bug spotted
    by the user directly in `phpinfo()`: "imagick module version" showed
    the literal string `@PACKAGE_VERSION@` instead of a real version
    number (2026-09-16).** Confirmed by reading `Imagick/imagick`'s real
    `php_imagick.h`: `#define PHP_IMAGICK_VERSION "@PACKAGE_VERSION@"`,
    with the extension's own comment admitting it's deliberate — the
    placeholder is meant to be substituted by PECL's own `pecl package`
    tool (reading `package.xml`'s `<version><release>` tag) at official
    release-tarball-build time. This pipeline never runs that tool — it
    just does a plain `git clone` of the extension's repo (`compile/php/
    Dockerfile`'s `IMAGICK_BRANCH="master"` for PHP 8.x) — so the raw
    placeholder string survived all the way into the compiled binary,
    silently, since nothing about this affects actual functionality
    (`ImageMagickVersion`/`getVersion()` etc. read from the real
    ImageMagick C library, already correctly reported elsewhere in the
    same `phpinfo()` table — only the wrapper *extension's own* version
    string was wrong). Almost certainly present in every build of this
    pipeline going back to the original Playground extraction; just never
    actually looked at closely until now.
    - **Also fixed the underlying non-reproducibility this bug rode in
      on**: `IMAGICK_BRANCH="master"` tracked a floating branch with no
      pin at all — the exact same class of gap decision 11 already
      flagged for `oniguruma`, just never flagged for `imagick` before.
      Pinned to `3.8.1` (verified via `gh api` as the real latest tagged
      release, not a guess) instead. `matrix.json`'s existing `imagick`
      entry (previously data-only — `repo`/`switch`/`libraries` only, no
      `versions`) gained a real `sourceTemplate` + `versions: ["3.8.1"]`,
      following the yaml/mdhtml/jsonk/apcu convention.
    - **Fix**: `compile/php/Dockerfile`'s imagick block now clones
      `--branch "$IMAGICK_EXT_VERSION"` (a new `ARG`, resolved from
      `matrix.json` via `cli.mjs`/`build.js` exactly like every other
      GitHub-sourced extension) instead of the hardcoded `"master"` for
      PHP 8.x, and immediately sed-replaces `@PACKAGE_VERSION@` with that
      same pinned version string in the freshly cloned `php_imagick.h`
      before it gets copied into `php-src/ext/imagick`. The PHP 7.x branch
      (`IMAGICK_BRANCH="3.7.0"`, dead code for this project — PHP 8.5 only,
      decision 16) was deliberately left untouched, including its own
      identical `@PACKAGE_VERSION@` exposure — out of scope, not the
      target platform.
    - **✅ Rebuilt and re-verified — pinning worked, but the substitution
      itself had a real, second bug of its own, found and fixed the same
      day.** The pin to `3.8.1` built cleanly (no functionality lost
      moving off `master`). But `ReflectionExtension('imagick')-
      >getVersion()` came back as the literal string
      `"3.8.1PACKAGE_VERSION3.8.1"` — not the expected clean `"3.8.1"`.
      Root cause: `replace.sh` shells out to `perl -pi.bak -e "$1"`, and
      Perl treats a bare `@` inside an `-e` pattern string as **array
      interpolation** (`@PACKAGE_VERSION` parsed as "interpolate the array
      named `PACKAGE_VERSION`", silently expanding to empty since no such
      array exists), not a literal character — confirmed directly with a
      standalone `perl -pi.bak -e 's/@PACKAGE_VERSION@/3.8.1/g'` test
      reproducing the exact same garbled output byte-for-byte. Every other
      `/root/replace.sh` sed-style call in this Dockerfile happens to
      target patterns with no bare `@` in them, so this specific gotcha
      never surfaced before. Fixed by escaping both `@` as `\@` in the
      pattern. Verified with the same standalone `perl` reproduction
      before spending a rebuild on it, then confirmed for real: the
      rebuilt `php.wasm`'s `imagick` extension now reports a clean
      `"3.8.1"`.

45. **The `mode: shared` `__stack_pointer` LinkError (decisions 32/42, open
    for months) is fully resolved — `sodium`, `ftp`, and `mysqlnd`+`mysqli`
    all now load and run correctly (2026-09-16).** Two independent bugs,
    found and fixed one real failure at a time, per this project's own
    established methodology.
    - **Root cause of the LinkError, found via a dedicated research
      subagent**: `-Wl,--export=__stack_pointer`/`--export=__table_base`
      (decision 32's original candidate fix, sitting unverified in the
      Dockerfile since some earlier session) does **not** survive
      Binaryen's post-link `wasm-metadce`/`wasm-opt` dead-code-elimination
      pass under `-sMAIN_MODULE=2 -O3` — confirmed by Emscripten's own
      maintainer on `emscripten-core/emscripten#25952`, filed against our
      exact pinned `4.0.19`: "the whole point of `-sMAIN_MODULE=2` is that
      it only keeps alive things you ask for" via `-s EXPORTED_FUNCTIONS`,
      not arbitrary `-Wl,` flags. Also confirmed WordPress Playground's own
      current upstream `compile/php/Dockerfile` (fetched live via `gh api`)
      has the **identical** gap — no `--export=` for these symbols at all —
      so this was never "Playground solves it and we don't," it's a bug
      neither pipeline had actually fixed; Playground's own core build
      just never happens to load a side module complex enough to need it.
      Real fix: route both symbols through the *existing*
      `.JS_ABI_EXPORTS` → `-s EXPORTED_FUNCTIONS` pipeline (the same
      mechanism already used for JSPI's own `___wasm_setjmp` family) —
      `___stack_pointer`/`___table_base`, with the mandatory extra `_`
      prefix `EXPORTED_FUNCTIONS` requires. Removed the dead `-Wl,--export=`
      lines. Verified directly with a throwaway `emcc -sMAIN_MODULE=2`
      test against `kirigami-php-wasm:base` (not a guess) before touching
      the real Dockerfile.
    - **Rebuilt and re-tested against `sodium.so`/`ftp.so`/`mysqlnd.so`/
      `mysqli.so`: `__stack_pointer`/`__table_base` resolved, `ftp` loaded
      immediately, but `sodium.so` and `mysqlnd.so` each hit a *different*,
      real missing-symbol error** (`atoll` for mysqlnd, `emscripten_asm_
      const_int` for sodium — libsodium's `randombytes_js.c` uses
      `EM_ASM_INT` for JS-side randomness). Both are the same underlying
      class of bug as `__stack_pointer`, just for different reasons:
      - `atoll` (and, one rebuild later, `free`/`strlen`): genuine, ordinary
        libc functions this main module only ever calls **internally**
        (direct wasm calls need no export) — wasm-ld's DCE drops the
        export even though the function itself stays compiled in. Fixed
        the same way: added to `.JS_ABI_EXPORTS`. Confirmed via
        `WebAssembly.Module.exports()` before/after: `atoll` went from
        absent to present, and `mergeLibSymbols()` (the generated JS's own
        runtime, already reads every real wasm export of a freshly
        instantiated module into `wasmImports`) picked it up automatically
        — no JS patch needed for real exports.
      - `emscripten_asm_const_int`: **not** a real C symbol at all — a pure
        JS-side Emscripten runtime helper, only ever generated into the
        output JS when *something in this exact link* uses `EM_ASM`, which
        this main module's own code never does. Forcing it into
        `EXPORTED_FUNCTIONS` (same mechanism) does make Emscripten generate
        the `_emscripten_asm_const_int` JS function, but — verified
        directly by inspecting the generated JS — it produces **no real
        wasm export**, so `mergeLibSymbols()` has nothing to copy into
        `wasmImports` for it. Fixed with a companion JS patch: a new
        `/root/replace.sh` call in the existing post-emcc cleanup block,
        inserting `wasmImports["emscripten_asm_const_int"] =
        _emscripten_asm_const_int;` right before Emscripten's own stable
        `// include: postamble.js` marker (always immediately follows the
        generated `wasmImports = {...}` literal, confirmed structurally
        stable and extension-set-independent — unlike matching the
        literal's own last property, which varies per build).
      - Found each of these **empirically, one real runtime failure at a
        time** (a temporary `console.warn` injected into the generated
        `resolveSymbol()`/`reportUndefinedSymbols()` on an already-built
        `php_8_5.js`, re-run against the test script, no rebuild needed
        just to *find* the next symbol) — deliberately not guessed from a
        static import-list diff of every `.so`, which would have
        over-fixed for symbols some code paths never actually reach.
    - **✅ `sodium` fully verified end-to-end**: `get_loaded_extensions()`
      lists it, and `sodium_crypto_secretbox()` genuinely encrypts data.
      This closes decision 32's original blocker for good.
    - **A second, unrelated bug found for `mysqli`+`mysqlnd`**: once the
      main-module export gaps above were fixed, `mysqlnd.so` itself loaded
      fine, but `mysqli.so` failed differently — `bad export type for
      'mysqlnd_global_stats': undefined (undefined)`, a genuine
      cross-side-module **data** symbol (`PHPAPI MYSQLND_STATS *
      mysqlnd_global_stats`, real, correctly defined in `mysqlnd_
      connection.c`) that mysqli.c needs from mysqlnd once both are
      loaded. Root cause, found by instrumenting the already-built JS
      (`loadDynamicLibrary()`'s own call order), not guessed: PHP loads
      shared extensions by scanning `PHP_INI_SCAN_DIR` and processing every
      `.ini` file it finds **in alphabetical filename order** — our own
      `packages/phpext-mysqli/index.js` generator (decision 42) resolved
      `mysqlnd` before `mysqli` into its returned *array*, correctly
      reflecting `bundleExtensions` order, but that array order has no
      bearing on the `.ini` filenames `resolvePHPExtension()` derives from
      each extension's bare `name` — `"mysqli.ini"` sorts alphabetically
      *before* `"mysqlnd.ini"` (`i` < `n`), the wrong way around, so
      `mysqli.so` was always being loaded first regardless of our careful
      array ordering. Fixed in `cli.mjs`'s `extensionIndexJs()` generator
      (not in `@php-wasm/universal`, which we don't control): when a
      package bundles more than one manifest, each resolved extension's
      `iniPath` is rewritten with a zero-padded numeric prefix matching its
      position in the array (`00-mysqlnd.ini`, `01-mysqli.ini`) — the `.so`
      path/content is untouched, only the ini filename's sort position
      changes, general enough to cover any future multi-manifest bundle,
      not just this one pair.
    - **✅ `mysqli`/`mysqlnd` fully verified end-to-end too**: load order
      confirmed correct (`mysqlnd.so` instantiated before `mysqli.so`),
      both appear in `get_loaded_extensions()`, and `mysqli_init()` returns
      a real `mysqli` object.
    - **Also queued, not yet acted on**: while debugging why a `compile-
      extension` build felt unusually slow, found (by reading its actual
      installed source, not guessing) that `@php-wasm/compile-extension`
      always builds its **own**, separate `playground-php-wasm:base` image
      from Docker assets it fetches from `WordPress/wordpress-playground`
      and caches locally (`~/.cache/php-wasm/compile-extension/docker-
      assets/<hash>/php-wasm/`) — confirmed via direct diff that this
      cached `compile/base-image/Dockerfile` differs from ours by exactly
      the em++ patch (decision 43's fix) and is otherwise byte-identical,
      meaning this tool's own side-module builds have never had that fix.
      No CLI flag or env var exists to point it at our own
      `kirigami-php-wasm:base` instead. Two fix paths identified, neither
      attempted yet: (a) seed its local asset cache directly with our own
      `compile/base-image/{Dockerfile,emcc-for-php-wasm.sh,replace.sh,
      replace-across-lines.sh}` + `compile/php/php8.5.patch` (a "poison the
      cache" trick — since the rest of the file is byte-identical, Docker's
      own layer cache should reuse our already-built layers under the new
      tag almost instantly) — cheap, no fork to maintain, but relies on
      this tool's internal cache-path scheme staying stable across its own
      version bumps; (b) fork `@php-wasm/compile-extension` for a real
      `--base-image` override — more robust, more to maintain. Leaning
      toward (a) first as a cheap experiment.

46. **`@kirigami/phpext-*` packages no longer depend on `@php-wasm/universal`
    at runtime — the user's explicit direction ("Éliminer les dépendances
    des extensions à php-universal"), after deciding `@kirigami/php-wasm`
    will do its own `.so`-staging and `php.ini` writing rather than relying
    on a shared library both sides depend on differently (2026-09-16).**
    Also: decision 45's option (a) ("poison the cache") implemented for
    real, and two more real missing-ABI-symbol bugs found and fixed —
    plus a new permanent tool to find such gaps without a rebuild per
    symbol.
    - **`extensionIndexJs()`/`extensionIndexDts()`/`extensionPackageJson()`
      in `cli.mjs` rewritten**: the generated `index.js`'s default-exported
      `register(phpVersion)` no longer imports `resolvePHPExtension` — it
      just reads each manifest (still the on-disk format
      `@php-wasm/compile-extension` itself produces, kept as-is per the
      user's "on peut garder les manifestes quand même") and returns plain
      `{ name, soPath }` objects (absolute path on disk) in load order. The
      caller (`@kirigami/php-wasm`, per the user's own description of its
      intended flow: "il va copier les .so dans sa vm et les mettre dans
      php.ini avant de lancer le tout") owns 100% of the actual `.so`
      staging and `php.ini` generation. `extensionPackageJson()`'s
      `dependencies` field dropped entirely — these packages now ship with
      zero npm dependencies. `packages/phpext-{sodium,ftp,mysqli}/`
      regenerated by hand to match (their `.so`/`manifest*.json`/
      `.buildhash` untouched — `computeBuildHash()` only hashes those, so
      no version bump was needed for this glue-code-only change); root
      `package-lock.json` and `compile/package-lock.json` re-synced via
      `npm install` after removing the now-unused dependency, clearing
      `@php-wasm/universal` from the root workspace tree entirely.
    - Verified with a hand-rolled loader (mirroring exactly what
      `@kirigami/php-wasm` is meant to do: `FS.mkdirTree` an extensions
      dir, `FS.writeFile` each `.so` + a numbered `extension=...` `.ini`
      per `register()`'s own array order, set `PHP_INI_SCAN_DIR`, then
      `loadPHPRuntime()`) — no `resolvePHPExtension`/
      `withResolvedPHPExtensions` involved at all. `@php-wasm/universal`
      itself is still used, but only as a `compile/` **devDependency**
      (added to `compile/package.json`, pinned `3.1.53` to match
      `@php-wasm/compile-extension`'s own pin) for this repo's own internal
      dev/test tooling — never shipped to consumers.
    - **Two more real, concretely-observed missing-ABI-symbol bugs found**,
      same shape as decision 45's `atoll`/`free`/`strlen`/
      `emscripten_asm_const_int` (and an already-in-progress, not-yet-
      documented `stdin`/`stderr` fix from the same investigation,
      committed together with these): `strcmp` (`mysqlnd.so` calls it
      directly, DCE drops the export) and `memset` (found on the *second*
      rebuild, after fixing `strcmp` — `mysqlnd.so`/`mysqli.so`). Both added
      to `.JS_ABI_EXPORTS` in `compile/php/Dockerfile`, same mechanism as
      every prior entry in that block.
    - **New: `compile/check-shared-extension-symbols.mjs`** — directly
      answers the user's follow-up question ("est-ce qu'il y a moyen de
      vérifier s'il y a d'autres fonctions manquantes pour pas rebuild à
      chaque fois qu'on en trouve une?"). Root cause of the
      one-rebuild-per-symbol pattern: `reportUndefinedSymbols()` in the
      generated `php_*.js` (Emscripten's own dylink runtime) iterates the
      GOT (Global Offset Table) and `throw`s on the *first* unresolved
      required entry — so every previous missing-symbol hunt needed a real
      Docker rebuild just to discover the *next* one. This script instead
      makes a throwaway patched copy of the **already-built** `php_*.js`
      (`.replaceAll()` on that exact throw site, verified present first,
      loud error if the marker's gone — same "patch third-party text we
      don't own, fail loudly if missing" shape `replace.sh` already uses
      throughout this pipeline), turning the throw into "log the symbol
      name to a `Set`, set `entry.value = 0`, `continue`" so the loop
      finishes instead of aborting. It then loads every installed
      `packages/phpext-*/` extension (via their own new dependency-free
      `register()`) against that patched copy in one run and prints every
      currently-missing symbol at once. Confirmed effective immediately:
      found `memset` in one run with zero rebuilds after the `strcmp` fix
      had already landed, and confirmed a clean "no missing ABI exports"
      pass once both were in place. Documented caveat: this only catches
      symbols resolved *eagerly* via the GOT at `dlopen()` time (every
      symbol found in this pipeline so far, decisions 32/45/46) — a small
      number of imports are resolved *lazily* (only when actually called,
      via a JS proxy stub) and won't surface here unless the exercised code
      path actually calls them during the script's own smoke test — a
      strong first pass, not an absolute guarantee.
    - **✅ Re-verified end-to-end after both fixes**: the hand-rolled-loader
      test (`sodium`, `ftp`, `mysqlnd`+`mysqli`) and the static-core test
      (`jsonk`, `apcu`, `imagick`, `sockets`, `mdhtml`, `yaml`, `curl`,
      `gd`, `openssl`, `sqlite3`, `xml`) both pass cleanly against the
      freshly rebuilt `php.wasm` — the same functional checks decisions 43
      and 45 already established, now also proving the dependency-free
      `register()` contract works in practice, not just in theory.

47. **`pdo_mysql` added as a second `mode: shared` extension bundling the
    same internal `mysqlnd` — the user's follow-up direction to keep going
    on `mode: shared` extensions that ship with php-src itself (2026-09-16).
    Built and runtime-verified on the first real attempt** (after one
    codegen-only fix found before wasting a build on it).
    - `compile/extensions/pdo_mysql/`: vendored complete from the
      `PHP-8.5.10` tag (`gh api .../contents/ext/pdo_mysql`, decision 40's
      "always get the real file list" lesson) — `CREDITS`, `config.w32`,
      `tests/` excluded, same convention as `mysqli`.
    - **Real, concretely-confirmed autoconf gotcha, fixed before building**:
      `config.m4` declares itself via `PHP_ARG_WITH([pdo-mysql], ...)`
      (no default -> "no"), not `PHP_ARG_ENABLE` — but `@php-wasm/compile-
      extension`'s `build-in-docker.sh` always passes a hardcoded
      `--enable-${EXTENSION_NAME}`, which autoconf silently ignores for a
      `WITH`-style flag. Without `configArgs: '--with-pdo-mysql=mysqlnd'`
      in `config.yaml`, the extension would have built a `.so` that never
      actually compiled PDO_MYSQL in. Caught by reading the real
      `config.m4` before the first build attempt, not discovered by a
      failed build.
    - **Same `mysqlnd` dependency shape as `mysqli`** (decision 42):
      `PHP_ADD_EXTENSION_DEP(pdo_mysql, mysqlnd)`, `php_pdo_mysql_int.h`
      `#include`s `ext/mysqlnd/mysqlnd_debug.h` directly — headers staged
      the same way (`cp compile/extensions/mysqli/ext/mysqlnd/*.h
      compile/extensions/pdo_mysql/ext/mysqlnd/`), `bundleExtensions:
      [mysqlnd]` reuses the same internal package (no second mysqlnd
      built/published). Also declares `PHP_ADD_EXTENSION_DEP(pdo_mysql,
      pdo)` — needs **no** staging at all: `pdo`'s own `config.m4` defaults
      to enabled and compile-extension's `Dockerfile.ext` never disables
      it, so its headers land under `/usr/local/include/php/ext/pdo/`
      automatically during the compile-extension build's own base-image
      setup, and `pdo` itself is already `mode: static` in this repo's
      core (no runtime bundling needed, only the compile-time headers).
    - **Real build failure, fixed before it cost a second wasted rebuild**:
      first attempt failed with `make: *** [.../mysql_sql_parser.c] Error
      127` / `-o: command not found`. Root cause: `mysql_sql_parser.re`
      needs `re2c` to generate `mysql_sql_parser.c`, and `config.m4`'s
      `PHP_ADD_MAKEFILE_FRAGMENT` does pull in the vendored `Makefile.frag`
      with the right `%.c: %.re` rule — but a plain `phpize`-based
      standalone build (what compile-extension runs) never sets the
      `$(RE2C)` make variable at all (that detection only happens in a
      full `buildconf` of the whole php-src tree), so the rule's command
      expanded to a bare ` -o mysql_sql_parser.c mysql_sql_parser.re` with
      no program name. This is exactly why official PHP release tarballs
      ship this extension with `mysql_sql_parser.c` pre-generated — fixed
      the same way: generated it once via `docker run --rm -v
      <dir>:/src -w /src kirigami-compile-extension:base re2c -o
      mysql_sql_parser.c mysql_sql_parser.re` (re2c already installed in
      the base image, decision 46) and committed the output alongside the
      `.re` source, documented in `compile/extensions/pdo_mysql/
      PROVENANCE.md`.
    - **✅ Runtime-verified on the very next attempt** (no further
      surprises): `compile/check-shared-extension-symbols.mjs` found zero
      missing ABI exports, and the hand-rolled-loader test confirmed
      `PDO::getAvailableDrivers()` lists `mysql` after loading `mysqlnd.so`
      then `pdo_mysql.so` — proof the driver actually registers with the
      static-core `PDO` class, not just that the `.so` loads.
    - **Real, useful gap found and fixed in the tooling itself while
      testing this**: with two packages (`phpext-mysqli` and
      `phpext-pdo_mysql`) each bundling their own copy of `mysqlnd`, both
      `check-shared-extension-symbols.mjs` and the hand-rolled-loader test
      script would try to load `mysqlnd.so` twice (once per package),
      producing a harmless-but-noisy `PHP Warning: Module "mysqlnd" is
      already loaded`. Both scripts fixed to dedupe `register()` entries by
      `name`, keeping only the first occurrence — a real design note for
      `@kirigami/php-wasm`'s own future scanner too (decision 39): it must
      dedupe by extension name across every installed `@kirigami/phpext-*`
      package, not assume each one's bundle is independent.
    - Config.yaml's old `pdo_mysql: { mode: 'off' }` entry (in the
      "targeted, not wired yet" list) removed — replaced by the real
      `mode: shared` entry.

48. **`dba` added as a third `mode: shared`, no-external-lib extension
    (only the bundled `cdb`/`inifile`/`flatfile` backends); the symbol-check
    tool's real blind spot (lazy-resolved symbols) found and fixed for good
    via a new `smoke-test.php`-per-extension convention (2026-09-16).**
    - **`dba` wiring**: vendored complete from the `PHP-8.5.10` tag
      (`gh api ".../git/trees/PHP-8.5.10?recursive=1"` filtered to
      `ext/dba/`, decision 40's lesson) — every `dba_*.c` backend file is
      vendored even for libraries this build never links (qdbm/gdbm/ndbm/
      db1-4/dbm/tcadb/lmdb all default to `no` in `config.m4`, but
      `PHP_NEW_EXTENSION` compiles every backend file unconditionally, each
      one `#ifdef`-guarded internally). No `configArgs` needed at all —
      unlike `pdo_mysql`, `dba`'s own `config.m4` logic already turns the
      three bundled-source backends (`cdb`/`inifile`/`flatfile`, no
      external lib) on by default the moment `--enable-dba` is passed,
      which `@php-wasm/compile-extension`'s own hardcoded
      `--enable-${EXTENSION_NAME}` already provides for free. Full
      reasoning in `compile/extensions/dba/PROVENANCE.md`.
    - **The symbol-check tool's real gap, found by the user pointing out it
      "doesn't seem to work well"**: `check-shared-extension-symbols.mjs`
      (decision 47) only patched the EAGER (GOT) throw site — but `dba`
      needed three more real ABI exports (`strcasecmp`, `atoi`, `memcmp`),
      every one of them resolved LAZILY (only when actually CALLED, via the
      dynamic linker's JS proxy stub — `resolveGlobalSymbol()`/
      `resolveSymbol()`'s stub-function path, not `reportUndefinedSymbols()`'s
      GOT loop), and none of them surfaced from the tool's own default smoke
      test (a bare `get_loaded_extensions()` call touches no extension-
      specific code at all). Each one needed a separate, ad-hoc, throwaway
      debug script and a full rebuild to find — exactly the "one rebuild
      per symbol" problem the tool was built to solve in the first place,
      just for a resolution path it didn't cover yet.
    - **Real fix, not a workaround**: patched the second resolution site too
      (`resolved ||= resolveSymbol(prop); return resolved(...args);` →
      logs `prop` to the same `Set` and returns `0` instead of calling
      `undefined`), and — the actual structural fix — added a
      `compile/extensions/<name>/smoke-test.php` convention: a small, real
      PHP script exercising that extension's actual functions (not just
      `function_exists()` checks), which the tool now discovers and runs,
      **per extension, in its own fresh isolated runtime boot** (so one
      extension's crash can't block or corrupt another's result, and
      per-package `register()` output no longer needs cross-package
      deduping either — each boot only ever loads one package's own
      entries). Wrote real ones for all five extensions built so far:
      `sodium` (real encrypt/decrypt round-trip — this exact call is what
      originally surfaced decisions 32/45's `__stack_pointer`/`free`/
      `emscripten_asm_const_int` gaps), `ftp` (every declared function
      reachable — deliberately does NOT call `ftp_connect()` for real,
      since this diagnostic's bare hand-rolled loader has no socket layer
      at all, unlike `@kirigami/php-wasm`'s own Node-side SOCKFS proxy,
      decision 22 — a real connect attempt could hang instead of failing
      fast), `mysqli` (`mysqli_init()` instanceof check), `pdo_mysql`
      (`PDO::getAvailableDrivers()` lists `mysql`), and `dba` (the exact
      `dba_open`/`dba_insert`/`dba_fetch` flatfile round-trip that found
      all three missing symbols). The tool now also prints a note listing
      any checked package with no `smoke-test.php` yet, so this doesn't
      silently regress for the next extension added without one.
    - **✅ Re-verified clean after all three fixes + one real process
      hiccup**: a rebuild was accidentally started twice concurrently
      (a background job from before this fix that hadn't actually finished
      yet, plus a fresh retry started without realizing the first was still
      running) — both tried to `docker run --name kirigami-php-wasm-tmp`,
      the second failing with a container-name conflict. Not a real bug:
      the first one's own log confirmed a clean `EXIT_CODE=0`, and its
      timestamp was newer than the failed second attempt's own recovery
      attempt — the check-shared-extension-symbols.mjs tool's own report
      (a clean "No missing ABI exports found" with all five
      `smoke-test.php`s passing) was the actual confirmation it worked,
      not either process's own reported exit code. **Lesson**: check for
      an actually-running Docker build (`docker ps`) before assuming a
      background rebuild died just because its own redirected log looks
      incomplete at the moment of checking — it may simply still be
      writing.

49. **Batch of six more no-external-lib, php-src-bundled `mode: shared`
    extensions attempted (`posix`, `pcntl`, `shmop`, `sysvshm`, `sysvmsg`,
    `sysvsem`) — only `posix` kept; the other five tried for real and
    deliberately dropped, each for a concretely-confirmed reason, not
    suspicion (2026-09-16).**
    - **A real `matrix.json` gap found and fixed first**: the user pointed
      out the matrix didn't cover all of PHP's built-in extensions.
      Confirmed concretely: `posix`/`pcntl`/`sysvmsg`/`sysvsem` were
      entirely missing from `matrix.json`'s `extensions` map (while
      `shmop`/`sysvshm`/`ftp`/`dba` were already present as data-only
      placeholders). Root cause: this matrix was originally ported from
      php-static-autobuilder's own matrix.json (Windows-only, targets a
      static CLI `.exe`), which never lists these POSIX-only extensions at
      all since they don't exist on Windows PHP builds — the gap carried
      over silently when ported. Added all four.
    - **New `matrix.json` convention: a `"disabled": true` field** (the
      user's own request, "ajoute un champ disabled"), distinct from the
      free-text `note` — a structured, queryable marker for "tried and
      rejected", as opposed to "not yet attempted" (`mode: 'off'` in
      config.yaml) or "actively wired" (no flag). Applied to `pcntl`,
      `shmop`, `sysvshm`, `sysvmsg`, `sysvsem` below.
    - **`posix` — kept, works**: vendored complete from the `PHP-8.5.10`
      tag, `PHP_ARG_ENABLE` (no `configArgs` needed). One real symbol gap
      found and fixed: `posix_getpid()` calls `getpid()` directly, but this
      main module is built with `-Wl,--wrap=getpid` for
      `EMSCRIPTEN_ENVIRONMENT=node` — so there is no plain `_getpid` export
      at all (a bare `echo '_getpid' >> .JS_ABI_EXPORTS` fails the link
      outright: "undefined exported symbol"), only the wrapped
      `___wrap_getpid`, which is already generated as
      `Module["___wrap_getpid"]` regardless of `.JS_ABI_EXPORTS`. Needed
      the same manual `wasmImports` seeding as `emscripten_asm_const_int`
      (decision 45) — but **not** at the same insertion point: the first
      attempt aliased it at the `// include: postamble.js` marker (like
      `emscripten_asm_const_int`) and the build succeeded, but
      `posix_getpid()` still returned `0` at runtime — found by actually
      testing, not assumed — because unlike `emscripten_asm_const_int` (a
      plain JS helper available synchronously), `___wrap_getpid` is
      `wasmExports["__wrap_getpid"]`, a **real wasm export only assigned
      after the main module finishes instantiating**, and the postamble
      marker's code runs *before* that (it's what kicks instantiation off).
      Fixed by inserting the alias immediately after Emscripten's own real,
      unconditionally-generated assignment line instead
      (`___wrap_getpid = Module["___wrap_getpid"] = wasmExports["__wrap_getpid"];`)
      — a textually stable, deterministic anchor point, same "structurally
      stable, not extension-set-dependent" reasoning decision 45 already
      used for its own marker choice, just a different marker for a
      different reason (timing, not aliasing).
    - **`pcntl` — tried, dropped**: found and fixed one real cross-compile
      false positive (`HAVE_GETCPUID` wrongly detected by `AC_CHECK_FUNCS`,
      pulling in Solaris's `<sys/processor.h>` — fixed with a direct edit
      to the vendored `pcntl.c`, `&& !defined(__EMSCRIPTEN__)` on that one
      `#if` guard, same shape as decision 42's `sockets` `AF_PACKET` fix),
      then hit a second build failure not worth chasing. More
      fundamentally, per the user's own doubt raised mid-session: its
      entire feature set (`fork()`, `waitpid()`, inter-process signals) has
      no real equivalent in a single-instance WASM VM with no real OS
      process model — even a successful build would ship functions that
      silently do nothing useful or fail unpredictably. Removed entirely
      (config.yaml, `compile/extensions/pcntl/`, `packages/phpext-pcntl/`),
      same treatment as `ext/cmark` (decision 40).
    - **`shmop`/`sysvshm`/`sysvmsg`/`sysvsem` — all four tried, all four
      dropped**: each vendored cleanly and **compiled** successfully via
      `compile-extension` on the first try (no source-level problems at
      all) — the real wall was the main `php.wasm` link step, one symbol
      at a time, under real time pressure (the user needed Docker back for
      something else). `check-shared-extension-symbols.mjs`'s own report
      of "1 missing export" per run turned out to be misleading: `.JS_ABI_
      EXPORTS` entries are alphabetically sorted before the final `emcc`
      link, and `-Wundefined -Werror` stops at the *first* undefined
      symbol it hits in that sorted order, not all of them — so each
      apparent "just this one symbol" fix actually only unmasked the
      *next* alphabetically-later undefined symbol, discovered one real
      rebuild at a time: `msgctl` → `msgget` → `semctl` → `shmat`. At that
      point, rather than continuing the same one-symbol-per-rebuild cycle,
      concluded (and confirmed sufficiently, not just guessed) that **none**
      of `shmget`/`shmat`/`shmctl`/`shmdt`/`msgget`/`msgsnd`/`msgrcv`/
      `msgctl`/`semget`/`semop`/`semctl` exist as real exportable symbols
      under this Emscripten toolchain — generalizing decision 43's own
      finding (apcu's optional shm backend hit the identical wall) from
      "the shm calls" to "the entire SysV IPC family". With no underlying
      syscall available at all for any of it, every function in all four
      extensions would be permanently broken. Removed entirely (config.yaml,
      `compile/extensions/{shmop,sysvshm,sysvmsg,sysvsem}/`,
      `packages/phpext-{shmop,sysvshm,sysvmsg,sysvsem}/`), same treatment
      as `pcntl`/`cmark`.
    - **Lesson for `check-shared-extension-symbols.mjs` itself, not yet
      acted on**: its "found N missing ABI exports" report from a single
      run is only reliable when every reported symbol is independent of
      the others — it does NOT mean "these are the only N problems," since
      a real linker failure (as opposed to this tool's own lazy/GOT
      instrumentation) still stops at the first `-Werror` hit. A future
      improvement would be teaching the tool to also validate its findings
      against a real (if slower) `emcc -c` dry-compile pass, or accepting
      this as an inherent limitation of diagnosing without paying for the
      full link every time.
    - **✅ Re-verified end-to-end after the final rebuild**: both
      `check-shared-extension-symbols.mjs` (all 6 remaining shared
      extensions: `sodium`, `ftp`, `mysqli`, `pdo_mysql`, `posix`, `dba` —
      zero missing exports) and the static-core regression test (15/15,
      unchanged) pass cleanly.

## Current status

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

## Environment

- **Severely resource-constrained host** (discovered 2026-09-12, the hard
  way): Windows 10 machine with only **8GB total RAM** (often <1.2GB free
  under normal load) and a Windows page file *manually fixed* at ~3.7GB
  (not auto-managed) — combined virtual memory is tight for back-to-back
  heavy C/C++ Docker builds (ImageMagick, PHP itself). The **C: drive is
  also small** (~118GB) and can fill up fast: Docker Desktop's WSL2 data
  disk (`%LOCALAPPDATA%\Docker\wsl\disk\docker_data.vhdx`) only grows, never
  auto-shrinks even after `docker system prune`, and `Optimize-VHD` (the
  proper compaction cmdlet) needs **local administrator rights**, which
  Claude Code's shell does not have here.
  - Tonight's incident history, worst first: (1) the daemon returned
    500/502 on every call ("Docker Desktop is unable to start") after hours
    of continuous builds — fixed by killing every `*docker*` process and
    relaunching `Docker Desktop.exe`; (2) a `docker build` crashed with a
    raw Go runtime panic — a one-off, a plain retry worked; (3) the real
    root cause underneath both: **the C: drive dropped to 240MB free**,
    causing `fork/exec ...docker-credential-desktop.exe: The paging file is
    too small for this operation to complete.` — a disk-space error
    disguised as a memory error. Fixed by `wsl --shutdown` +
    `wsl --unregister docker-desktop` (worked; `docker-desktop-data` gave
    `WSL_E_DISTRO_NOT_FOUND` despite `docker_data.vhdx` still existing on
    disk as an 18.8GB orphan) + directly deleting that orphaned `.vhdx` file
    once nothing held it open, freeing ~18GB instantly. Confirmed safe: nothing
    in a Docker image/build-cache is irreplaceable, it's all reproducible
    from this repo's Dockerfiles — losing it just means the next build
    starts cold. **User explicitly approved this specific deletion** (asked
    via AskUserQuestion first — Claude Code's auto-mode classifier itself
    flagged `wsl --unregister` as an irreversible deletion needing
    confirmation, correctly).
  - **Practical takeaway for future sessions**: if a build fails with a
    Docker daemon/API error (500/502, "unable to start", a paging-file
    error, or a raw Go panic) rather than a normal compile failure, **check
    free disk space on C: first** (`Get-CimInstance Win32_LogicalDisk
    -Filter "DeviceID='C:'"`), not just `docker info`/memory. Below a
    couple GB free, Windows itself starts failing in confusing ways
    (spawning helper processes, growing the page file) that look like
    Docker bugs but aren't. `docker system prune` alone does **not** free
    host disk space if the win is inside a WSL2 `.vhdx` — the file doesn't
    shrink on its own; either `Optimize-VHD` (needs admin) or removing/
    recreating the distro (works without admin, but destructive — confirm
    with the user first, as we did) is required to actually reclaim it on
    the Windows side.
- Sibling repos useful for reference (do not modify from here):
  - `C:\projects\kirigami\php-wasm-builder` — **deleted entirely (2026-09-12)**,
    including its `.git` (7.2GB out of 8.3GB total — the working tree alone
    had 26,371 uncommitted changes from the earlier physical-deletion trim,
    never committed). No longer needed for reference: everything relevant
    was already extracted into this repo (see "Extraction done" above), and
    the disk-space crisis earlier tonight (see incident history below) made
    reclaiming the space worth more than keeping it around. Recoverable if
    ever needed via `git clone https://github.com/php-kirigami/php-wasm-builder.git`
    (the user's own fork; `upstream` was `WordPress/wordpress-playground.git`).
  - `C:\projects\kirigami\kirigami` — the Kirigami framework itself
    (`todo.md` documents this project).
  - `C:\projects\kirigami\kiribuild` — Kirigami's build tool (GitHub Pages
    via Actions); likely final consumer of the php-wasm package produced
    here.

## Next steps (not done yet)

- Precisely scope which PHP extensions/versions to support (currently
  follows the fork as-is — todo.md explicitly says "to be scoped
  separately").
- Wire `config.yaml`'s `libraries:` versions into the actual build.
- Implement automatic "latest" version resolution per library.
- Wire the `patches/` convention into the lib Dockerfiles.
- Write the first GitHub Actions workflow (Docker build + matrix + npm
  publish + release asset attachment).
- **Queued (2026-09-15), not started**: use PIE (php/pie, PHP's official
  PECL successor) via Packagist's public API
  (`https://repo.packagist.org/p2/<vendor>/<package>.json`) as a version
  source for extensions that have migrated to it — e.g. `yaml`'s real
  upstream is `pecl/yaml` on Packagist — instead of scraping pecl.php.net
  or hand-running `gh api` like decision 34 did. Verified this isn't
  universal: `krakjoe/cmark` (last real commit 2019, predates PIE
  entirely) almost certainly has no Packagist/PIE listing, so this needs a
  new `sourceType: "packagist"` in `matrix.json` alongside the existing
  `github-release`/`github-tag`, with GitHub direct as the fallback for
  extensions PIE doesn't cover — not a wholesale replacement.
- **Floated (2026-09-16), not scoped**: generate `compile/php/Dockerfile`
  from `config.yaml` + `matrix.json` instead of hand-maintaining an
  ARG/fetch/configure-flag block per extension (decision 43's jsonk/apcu
  addition, like decisions 34/37/41/42 before it, each added another
  near-identical hand-written block). Needs its own scoping pass — see
  decision 43's note on the mechanical-vs-bespoke block split before
  attempting it.
- **Requested (2026-09-16), for the next full build**: add `php-navicat`
  (wired into `config.yaml`/`matrix.json`/`compile/php/Dockerfile` as
  `mode: static` this session, but never actually build-tested in this
  pipeline — only native-build-tested in its own repo) and `igbinary`
  (not wired in at all yet — see decision 43's queued note on making it
  APCu's default serializer) to the static core build.
