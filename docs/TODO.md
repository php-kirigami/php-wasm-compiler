# TODO

Concrete next actions. For larger, not-yet-scheduled initiatives, see
[ROADMAP.md](ROADMAP.md); for what's already done, see
[STATUS.md](STATUS.md).

## Next (2026-09-23)

- [ ] Fix or remove `saveConfig` in `cli.mjs` (interactive mode): it
      re-serializes config.yaml with `stringifyYaml` and strips every comment
      (decision 68). Until then, only run the CLI with `--quiet`. The libs/core
      split itself is done (`libs`, `php`, `build --skip-libs`, decision 68).
- [ ] Check whether the `Failed opening '.../extensions/*/*.so'` line in the
      core build log (seen 2026-10-01, build still succeeds) is benign.
- [ ] Keep a smoke test for the static `aura`/`translit` (decision 67): the
      check was a throwaway script, and `compile/extensions/<name>/smoke-test.php`
      only covers `mode: shared` packages today.
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
