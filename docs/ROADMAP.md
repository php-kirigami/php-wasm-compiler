# Roadmap

Larger, not-yet-scheduled initiatives. For the concrete next actions, see [TODO.md](TODO.md); for what's already built and working, see [STATUS.md](STATUS.md).

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
- **Still open from earlier sessions**: `igbinary` becoming APCu's default
  serializer (decision 45's follow-up) is wired in
  `config.yaml`/`compile/php/Dockerfile`, and the 2026-09-17 rebuild
  (decision 51) confirms the pipeline builds clean with it enabled, but the
  actual runtime behavior (`apc.serializer` really resolving to `igbinary`,
  a stored value round-tripping through it) hasn't been checked yet — only
  `norm`/`navicat`/`jsonk`/`mdhtml` were smoke-tested that session (see
  STATUS.md).
- **Floated (2026-09-23), not scoped**: let a shared extension load a
  further WASM side module through `dlopen()`, from the VM filesystem.
  Three extensions are only half-useful without it, all for the same
  reason (decisions 57/58): `odbc`/`pdo_odbc` have a driver manager but no
  database driver (candidates: FreeTDS's own `src/odbc` driver, psqlODBC,
  sqliteodbc), and `enchant` has no spell-checking provider (Hunspell,
  which enchant loads with `g_module_open()`). `dlopen`/`dlerror` are
  already exported by the core. What's missing is a driver or provider
  actually built as a side module, and proof that a runtime `dlopen()` of
  it from inside another side module works under JSPI.
