<div align="center">

<img src="https://zmotrin.github.io/assets/kirigami/kirigami-logo-universal.svg" alt="Kirigami" width="400" />

---

# php-wasm-compiler

**Compiles PHP to WebAssembly (Docker + Emscripten) — Node.js/JSPI only.**  
Builds the `@kirigami/php-wasm` runtime for the **[Kirigami](https://github.com/php-kirigami)** static site generator.

[![License: GPL-2.0-or-later](https://img.shields.io/badge/license-GPL--2.0--or--later-yellow)](./LICENSE)
[![Node.js >=24.0.0](https://img.shields.io/badge/node-%3E%3D24.0.0-brightgreen)](https://nodejs.org)
[![Website](https://img.shields.io/badge/website-php--kirigami.github.io-1f6b4a)](https://php-kirigami.github.io)

</div>

---

## Overview

`php-wasm-compiler` builds our own PHP → WebAssembly toolchain, forked from the [WordPress Playground](https://github.com/WordPress/wordpress-playground) `@php-wasm/compile` pipeline (via [`php-kirigami/php-wasm-builder`](https://github.com/php-kirigami/php-wasm-builder)) and trimmed to exactly what Kirigami needs:

- ✅ **JSPI** (JavaScript Promise Integration) target only
- ✅ **Node.js** runtime only
- ❌ No browser build, no Asyncify

A single [`config.yaml`](config.yaml) drives the whole build — PHP versions, which extensions to compile in (statically today, as loadable Kirigami plugins later), and build options — with an interactive CLI or a `--quiet` flag for CI. See [`CLAUDE.md`](CLAUDE.md) for the full architecture and decision history, and [`NOTICE.md`](NOTICE.md) for the upstream code's provenance.

---

## Table of contents

- [php-wasm-compiler](#php-wasm-compiler)
  - [Overview](#overview)
  - [Table of contents](#table-of-contents)
  - [Requirements](#requirements)
  - [Configuration](#configuration)
  - [Building the third-party libraries](#building-the-third-party-libraries)
  - [Building PHP](#building-php)
  - [License](#license)
  - [Author](#author)

---

## Requirements

- Docker
- Node.js `>= 24.0.0`
- GNU Make — ships by default on Linux/macOS, but not on Windows (install separately, e.g. via Chocolatey/Scoop/MSYS2, or run from inside WSL)
- A POSIX shell + coreutils (`sh`, `mkdir -p`, `rm -rf`, `mv`) — also not default on Windows (Git for Windows/Git Bash or MSYS2 provide these; `cmd.exe` does not)
- On Windows: WSL with an Ubuntu distro that has Node `>= 24.0.0` installed

All of the above (except the WSL/Ubuntu/Node-in-WSL check, Windows-only) are checked automatically when you run `node compile/cli.mjs`.

---

## Configuration

All build options live in [`config.yaml`](config.yaml) (PHP versions, extensions, build options) — see also [`matrix.json`](matrix.json) for the known versions of each third-party library.

---

## Building the third-party libraries

Once, or after an update:

```bash
cd compile
npm install
make base-image
make all_jspi
```

---

## Building PHP

```bash
cd compile

# Interactive: review/edit config.yaml before running the build
node cli.mjs

# Silent (CI): use config.yaml as-is, no prompts
node cli.mjs --quiet

# Print the commands that would run, without building
node cli.mjs --quiet --dry-run
```

---

## License

`GPL-2.0-or-later` — inherited from the upstream WordPress Playground project. See [LICENSE](./LICENSE) for the full text, and [NOTICE.md](./NOTICE.md) for provenance details.

---

## Author

Maxime Larrivée-Roy, 2026
