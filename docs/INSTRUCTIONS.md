# Instructions for Claude

Operational notes Claude should read before working in this repo — environment quirks, tooling, and sibling-repo context that would otherwise cause repeated mistakes or wasted debugging time.

## Environment & tooling notes

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

- **phpize `config.h` pitfall for `mode: shared` extensions** (found
  2026-09-23 via mysqlnd's "Bad handshake"): under phpize, a config.m4's
  `AC_DEFINE`s land only in the extension's own generated `config.h`, not
  in the core's `main/php_config.h`. Any `.c` file that includes just
  `php.h` (no `config.h`, directly or through a local header) silently
  compiles as if those macros were undefined — in-tree php-src builds never
  show this since everything goes into `php_config.h`. Fix per extension by
  repeating the defines in `config.yaml`'s `extraCflags` (see mysqlnd).
  When vendoring a new extension, check each `.c` that tests a config.m4
  define actually reaches `config.h` first; a 2026-09-23 sweep of every
  `compile/extensions/*` found only mysqlnd affected.

- **`PHP_ADD_LIBRARY(<system lib>)` pitfall for `mode: shared` extensions**
  (found 2026-10-02 with php_dlib's `stdc++` and fann's `m`): a config.m4
  that adds `-lstdc++` or `-lm` makes libtool warn "linker path does not
  have real file for library -lX" and fall back to a static module, so the
  build ends with compile-extension's "Could not find a built .so under
  /build/modules" and no other hint. Emscripten has no such files (libm is
  in its libc, the C++ runtime comes from the `libcxx` vendorLib). Delete
  the `PHP_ADD_LIBRARY` line and note it in the extension's PROVENANCE.md.

- **`git apply` inside `/root/php-src` skips patches silently** (found
  2026-10-02 with `patches/aspect/`): php-src is itself a git repository, so a
  patch whose paths are relative to the extension (`a/aspect.c`) is read from
  the repository root, printed as "Skipped patch" and still exits 0. Apply
  with `git apply --no-index --directory=ext/<name>` from `/root/php-src`
  and `grep` for the change afterwards, so a skipped patch fails the build.
  The older `cd <dir> && git apply` patterns for `patches/simdjson/` and the
  others were not re-checked for this: confirm "Applied patch" in the build
  log before copying one.
