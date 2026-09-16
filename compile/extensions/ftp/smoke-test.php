<?php
// Exercised by compile/check-shared-extension-symbols.mjs. Deliberately does
// NOT call ftp_connect() for real: this repo's own runtime (@kirigami/php-wasm's
// getPHPRuntimeWithNetwork(), see CLAUDE.md decision 22) is what wires up
// actual networking via a Node-side SOCKFS proxy -- a bare hand-rolled loader
// (what this diagnostic uses) has no socket layer at all, so a real connect
// attempt could hang instead of failing fast. Checking every declared
// function is reachable (not just ftp_connect) is still enough to prove the
// extension linked and its symbol table resolves.
foreach (['ftp_connect', 'ftp_login', 'ftp_pwd', 'ftp_get', 'ftp_put', 'ftp_close'] as $fn) {
	if (!function_exists($fn)) {
		fwrite(STDERR, "$fn() is not available.\n");
		exit(1);
	}
}
