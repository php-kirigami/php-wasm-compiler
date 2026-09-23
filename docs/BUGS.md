# Bugs

Historical bugs found and fixed during development are recorded as dated
entries in the [DECISIONS.md](DECISIONS.md) decision log rather than
listed here, since each one is tied to the architectural decision it
prompted or resulted from.

Convention going forward: log a new bug here when it's found and still
open. Once fixed, move the entry into `DECISIONS.md` as a dated note (or
drop it if it's not architecturally significant) and remove it from this
file.

## Open

Found 2026-09-22 while verifying the PHP 8.5.11 core rebuild. None of these
are regressions from that rebuild: all three extensions had only ever been
checked load-only by `check-shared-extension-symbols.mjs` (no
`smoke-test.php`), which only catches eager/GOT symbols. Actually calling
their functions surfaces lazy symbols the core doesn't export. With the
unpatched loader, each fails with `resolved is not a function` on the first
real call.

- **`gettext` — missing libc ABI exports + `ngettext` family absent.**
  Calling `bindtextdomain()`/`textdomain()`/`gettext()` needs
  `_bindtextdomain`, `_textdomain`, `_gettext` in `.JS_ABI_EXPORTS`
  (`compile/php/Dockerfile`). Separately, `ngettext()` is undefined at the
  PHP level (`dgettext()` exists): `config.m4`'s
  `AC_CHECK_LIB([$GETTEXT_CHECK_IN_LIB], [ngettext], ...)` probe came back
  negative, so `HAVE_NGETTEXT`/`HAVE_DNGETTEXT`/`HAVE_DCNGETTEXT` (and
  likely `HAVE_BIND_TEXTDOMAIN_CODESET`) were never defined. This is the
  inverse of the `AC_CHECK_LIB` false positive pgsql hit (decision 54);
  the same `ac_cv_lib_<lib>_<func>=yes` pre-seeding in `configArgs` should
  work, once each function is confirmed present in musl.
- **`soap` — missing `bsearch`, `iconv_open`, `iconv`, `iconv_close`.**
  `SoapServer::handle()` fails with `Dump memory failed`. The cause is
  structural: the core `.wasm` exports no libxml2 symbols, so `soap.so`
  (`vendorLib: libxml2`) links in its **own full second copy of libxml2**
  (~1300 `xml*` exports in the `.so`). That copy's encoding code needs
  iconv from the core. Exporting the four symbols is the quick fix. The
  duplicated libxml2 (separate globals and init state from the core's
  `ext/libxml`) is worth a decision of its own.
- **`enchant` — 19 missing exports, then `function signature mismatch`.**
  glib/libenchant need `nl_langinfo`, `strncpy`, `bindtextdomain`,
  `bind_textdomain_codeset`, `pthread_cond_broadcast`, `textdomain`,
  `gettext`, `setlocale`, `iconv_open`, `iconv`, `iconv_close`, `open`,
  `fstat`, `read`, `fileno`, `isatty`, `strcat`, `localtime`, `strftime`
  just for `enchant_broker_init()` + `enchant_broker_describe()`. With the
  symbol-check stubs in place, the run then dies with a wasm `function
  signature mismatch`. That may only be a side effect of the stubs
  returning 0; re-check after the real exports are added.

Follow-up for all three: add a `compile/extensions/<name>/smoke-test.php`
so `check-shared-extension-symbols.mjs` catches these going forward.
