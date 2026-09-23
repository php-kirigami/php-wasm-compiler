<?php
// Exercised by compile/check-shared-extension-symbols.mjs.
//
// A real ldap_bind() attempt against a closed local port WAS tried here
// (matching pgsql's/pdo_pgsql's own smoke-test.php approach) and is what
// actually found the connection-path symbols now baked into
// compile/php/Dockerfile's .JS_ABI_EXPORTS (strtol/socket/fcntl/
// setsockopt/inet_ntop/connect/freeaddrinfo/time/poll/shutdown/close, on
// top of the earlier wctomb/wcstombs/mbtowc/mbstowcs/strchr/fopen/getuid/
// siprintf/snprintf/gai_strerror batch) -- but a real connect() attempt
// hangs this check's own minimal runtime boot indefinitely: it has no
// SOCKFS/Node-side networking proxy wired in (unlike the full
// @kirigami/php-wasm runtime.js), so the underlying blocking syscall never
// resolves, and PHP-level options like LDAP_OPT_NETWORK_TIMEOUT can't help
// -- the timeout mechanism itself needs a working event loop to fire,
// which never happens either. So this only calls ldap_connect() (lazy,
// never opens a socket by itself) -- real coverage of the connection-path
// symbols above already happened once and is preserved in
// .JS_ABI_EXPORTS; re-triggering it on every check run isn't worth the
// hang risk.
$conn = ldap_connect('127.0.0.1', 1);
if ($conn === false) {
	echo "ldap_connect() unexpectedly returned false.\n";
	exit(1);
}

ldap_set_option($conn, LDAP_OPT_PROTOCOL_VERSION, 3);
