# patches/

Convention (inspired by `patches/` + `create-patches.bat` in
[php-static-autobuilder](https://github.com/ZmotriN/php-static-autobuilder)):
one subfolder per library or per PHP version, holding `.patch` files applied
automatically during the build, so a one-off compilation hack no longer
means hand-editing a Dockerfile.

```
patches/
  php-8.5/*.patch        # already partly the case: compile/php/php8.5.patch
  libcurl/*.patch
  oniguruma/*.patch
  ...
```

**Status: wired up for php-src extensions, not yet for third-party libs.**
`compile/php/Dockerfile` applies `patches/<ext>/*.patch` (via `git apply
--no-index`, `cd`'d into the right directory first) for extensions that are
part of — or vendored into — the main PHP source tree: `patches/sockets/`
(CLAUDE.md decision 42 — `ext/sockets` assumes Linux kernel raw-socket/BPF
headers are present whenever `AF_PACKET`/`SO_ATTACH_REUSEPORT_CBPF` are
defined, which isn't true under Emscripten's partial POSIX emulation) and
`patches/apcu/` (jsonk/apcu decision — `apc_shm.c` unconditionally compiles
real SysV shm syscalls Emscripten declares but never implements, an
undefined-symbol link failure) and `patches/simdjson/` (same decision —
`simdjson.cpp`'s runtime CPU-feature detection is gated by a raw, un-
overridable `#elif defined(__x86_64__)` that emits real x86 `cpuid`/
`xgetbv` inline asm; applied to `ext/jsonk/vendor/simdjson/simdjson.cpp`
right after it's fetched, since simdjson.cpp is downloaded directly here,
not vendored by `php-jsonk` itself) are the current real examples;
`patches/cmark/` (CLAUDE.md decisions 34-35, removed along with `ext/cmark`
itself in decision 40) was an earlier one.
The third-party lib Dockerfiles (`compile/lib*/Dockerfile`) still apply
their compilation hacks inline (`sed`/`replace.sh`) — nothing reads this
folder for those yet. Still to do: add, in each lib Dockerfile, a
`COPY ./patches/<lib>/ ...` + `git apply`/`patch` step before
`configure`/`make`, the same way `compile/php/Dockerfile` now does for
php-src extensions.
