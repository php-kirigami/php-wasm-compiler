<?php
// Exercised by compile/check-shared-extension-symbols.mjs. Same reasoning
// as compile/extensions/pgsql/smoke-test.php -- no real PostgreSQL server
// is available in this check's isolated runtime boot, so this connects
// against a closed local port (127.0.0.1:1, guaranteed to refuse
// immediately, no hang) to exercise pgsql_driver.c's real connection-
// establishment code path rather than just loading the module.
try {
	new PDO('pgsql:host=127.0.0.1;port=1;connect_timeout=1', '', '');
	echo "PDO connection unexpectedly succeeded against a closed port.\n";
	exit(1);
} catch (PDOException $e) {
	// Expected: the connection is refused.
}
