// Real bug in Emscripten's own dynamic-linker JS generation
// (src/lib/libdylink.js's addEmJs(), used to wire up EM_JS-defined
// functions in a side module at dlopen() time): it parses each C-style
// argument's type off an EM_JS function signature and strips pointer
// stars via a non-global `jsArg.replace('*', '')` -- for a double (or
// higher) pointer argument, e.g. `void **avalue`, only the FIRST '*' gets
// removed, leaving a literal '*' in the generated parameter name and
// producing invalid JS ("SyntaxError: Unexpected token '*'") the moment
// such a side module is loaded. Found via a real failure: libffi's own
// wasm32 EM_JS trampolines (ffi_call_js, used by ext/enchant's libffi
// dependency, CLAUDE.md decision 57) legitimately declare `void **avalue`.
// This is upstream Emscripten's own bug, unrelated to any specific
// vendored library, so it's patched once here (a global-replace regex)
// rather than worked around per extension.
import { readFileSync, writeFileSync } from 'node:fs';

const path = '/root/emsdk/upstream/emscripten/src/lib/libdylink.js';
const search = "jsArg.replace('*', '')";
const replacement = "jsArg.replace(/\\*/g, '')";

const contents = readFileSync(path, 'utf8');
if (!contents.includes(search)) {
	throw new Error(
		`Could not find ${JSON.stringify(search)} in ${path} -- Emscripten's own libdylink.js source may have changed; re-verify this patch is still needed/correct before relying on it.`
	);
}
writeFileSync(path, contents.split(search).join(replacement));
