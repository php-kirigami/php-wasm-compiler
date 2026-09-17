# php-wasm-compiler

Our own PHP → WASM compiler (Kirigami ecosystem), replacing the
`php-wasm-builder` fork of WordPress Playground. See
[docs/CONTEXT.md](docs/CONTEXT.md) for the full background.

This file is the entry point only. Detailed, evolving content lives under
[docs/](docs/), split by topic:

- [docs/CONTEXT.md](docs/CONTEXT.md) — project purpose, background, current
  focus.
- [docs/DECISIONS.md](docs/DECISIONS.md) — the numbered architecture
  decision log. Read before proposing a different approach to something
  already settled there.
- [docs/STATUS.md](docs/STATUS.md) — what's actually built and verified
  vs. still missing.
- [docs/ROADMAP.md](docs/ROADMAP.md) — larger, not-yet-scheduled
  initiatives.
- [docs/TODO.md](docs/TODO.md) — concrete next actions.
- [docs/BUGS.md](docs/BUGS.md) — currently open bugs, if any.
- [docs/INSTRUCTIONS.md](docs/INSTRUCTIONS.md) — operational notes
  (environment quirks, tooling, sibling repos) to read before working in
  this repo.

## Core conventions

- **Language**: conversation with the user is in French; everything
  committed to this repo (code, comments, CLI text, error messages, docs)
  is in English. See the global convention in `~/.claude/CLAUDE.md`.
- **Node.js**: this repo's ecosystem convention is ESM-only, Node >= 24.
- **JSPI only, no Asyncify** (decision 6 in DECISIONS.md).
- **Node.js target only, no browser build** (decision 7).
- **PHP 8.5** is the target version (decision 16).
- Keep this file at or under ~200 lines. New durable content goes into the
  appropriate `docs/*.md` file instead of growing this one — create a new
  topic file under `docs/` rather than overloading an existing one if
  nothing fits.

## Repo layout

- `compile/` — Docker build pipeline (base image, PHP Dockerfile, per-lib
  Dockerfiles, `cli.mjs`/`build.js` orchestrator).
- `config.yaml` — single source of truth for PHP versions, extensions
  (`mode: static|shared|off`), library versions, build options.
- `matrix.json` — known versions/sources for each vendored third-party
  library.
- `patches/` — per-dependency patch convention (not yet wired into every
  Dockerfile — see [docs/TODO.md](docs/TODO.md)).
- `node-builds/` — build output (gitignored).
- `packages/` — assembled npm package(s) (in progress).
