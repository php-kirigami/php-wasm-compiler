# TODO

Concrete next actions. For larger, not-yet-scheduled initiatives, see
[ROADMAP.md](ROADMAP.md); for what's already done, see
[STATUS.md](STATUS.md).

## Next (2026-09-23)

- [ ] Split library compilation from core compilation: a `build-libs` command
      (the `*_jspi` targets from `computeRequiredLibTargets`) and a `build`
      that only builds the core and fails clearly when a required `.a` is
      missing, instead of running `make` itself. Lets a core failure retry
      without re-checking every lib, and lets CI cache the libs separately.
      Planned shape (2026-10-01): `cli.mjs libs` and `cli.mjs php` as
      sub-commands, the default `build` chaining both (CI unchanged), plus a
      `--skip-libs` flag on `build` to iterate on extensions. Add a DECISIONS.md
      entry when done. Note: `saveConfig` (interactive mode) re-serializes
      config.yaml with `stringifyYaml` and strips every comment — only ever run
      the CLI with `--quiet` here, and consider fixing or removing that save path.
- [ ] Verify the static `jsonpath` (supermetrics-public/pecl-jsonpath v3.1.0,
      added 2026-10-01): the build passes and configure reports it enabled,
      but nothing has run it yet. Check `extension_loaded('jsonpath')` and a
      real query on `node-builds/8-5/`. Also check whether the
      `Failed opening '.../extensions/*/*.so'` line in the build log is benign.
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
