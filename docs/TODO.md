# TODO

Concrete next actions. For larger, not-yet-scheduled initiatives, see
[ROADMAP.md](ROADMAP.md); for what's already done, see
[STATUS.md](STATUS.md).

## Next (2026-09-23)

- [ ] Build fastchart (`node compile/cli.mjs compile-extension fastchart`)
      and run its smoke test; first use of `vendorLibs[].archives`, of
      the `wasm-pkgconfig/` shims and of `#embed` for the default font.
- [ ] anydoc (`hosmelq/ext-anydoc`, Rust via ext-php-rs): a Rust build path
      for compile-extension (toolchain with `wasm32-unknown-emscripten`,
      PIC std via `-Zbuild-std`, `panic=abort` or wasm EH to match the
      core, cargo target/output wired into its `pie/Makefile.frag`).
      Watch `rayon` (thread pool, no pthreads here) in `pdf-inspector`.
- [ ] Fix the snmp hang (docs/BUGS.md), then restore a real request in
      its smoke test; test SNMP against a real agent through the runtime's
      UDP relay.
- [ ] Remove the stale gettext/... entries from docs/BUGS.md (fixed: all
      shared extensions pass their smoke tests now).

- [ ] Wire `config.yaml`'s `libraries:` versions into the actual build
      (`cli.mjs`/`build.js` currently ignore them — the libs are built at
      whatever version their own Dockerfile pins). Decide whether to
      resolve this at the `make`/Makefile level instead.
- [ ] Implement automatic "latest" version resolution per third-party
      library (today only documented in `matrix.json`, not scraped).
- [ ] Wire the `patches/` convention into the third-party lib Dockerfiles
      (only `compile/php/Dockerfile` applies patches today).
- [ ] Assemble the core `mode: static` npm package from the raw
      `node-builds/8-5/` build output (`index.js`, `runtime/runtime.js`,
      `index.d.ts`), matching the shape documented in
      `../kirigami/packages/php-wasm/README.md`.
- [ ] Start the `@kirigami/php-wasm`-side auto-detection/auto-load scan
      for `mode: shared` extensions (ownership resolved — see DECISIONS.md
      decision 39 — but the scan code itself hasn't been started).
- [ ] Write the first GitHub Actions workflow (Docker build + matrix +
      npm publish + release asset attachment).
