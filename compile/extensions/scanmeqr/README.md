# qrcode-ext

The native QR encoder behind [ScanMePHP](https://github.com/crazy-goat/ScanMePHP), installable
with [PIE](https://github.com/php/pie). Extension version 0.5.2.

**This repository is generated.** The sources live in
[crazy-goat/ScanMePHP](https://github.com/crazy-goat/ScanMePHP) under `php-ext/` and
`clib/`, and every release overwrites this mirror with a single commit — issues and pull
requests belong there.

## Installing

```bash
composer require crazy-goat/scanmephp
pie install crazy-goat/qrcode-ext
```

Both halves are needed. The extension exposes a single internal class,
`CrazyGoat\ScanMePHP\NativeEncoderCore`, and its `encodeMatrix()` builds a
`CrazyGoat\ScanMePHP\Matrix` — so without the library loaded it can only throw. The library
in turn detects the extension and routes `NativeEncoder` through it automatically; with
neither the extension nor an FFI binary present it falls back to the pure-PHP encoder, so
installation is an optimisation and never a requirement.

Building needs a C++20 compiler (GCC 10+ / Clang 12+) and takes a few seconds. There is nothing
else to install: the C++ core is compiled into the extension rather than linked against a
separate shared library.

## What you get

The encoder runs about 10× faster than the pure-PHP path. On x86-64 the mask-penalty pass is
built three times over — a portable kernel plus AVX2 and AVX-512 variants — and the right one
is chosen at load time from the CPU's own feature bits, so one binary stays correct on any
machine that can run it. arm64 gets the portable kernel, which the compiler vectorises to NEON.

Prebuilt binaries for the common platforms are attached to every
[ScanMePHP release](https://github.com/crazy-goat/ScanMePHP/releases) if you would rather not
compile at all.

MIT.
