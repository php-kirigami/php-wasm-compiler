<?php
// Exercised by compile/check-shared-extension-symbols.mjs. No SQL Server is
// available in this check's isolated runtime boot, so this connects to a
// closed local port (127.0.0.1:1): FreeTDS still runs its real db-lib login
// and TDS socket code (dbinit, dblogin, tds_open_socket, ...) before failing
// -- a bare driver-registration check would miss anything only reached from
// inside that path.

if (!in_array('dblib', PDO::getAvailableDrivers(), true)) {
	echo "PDO driver 'dblib' not registered: " . implode(', ', PDO::getAvailableDrivers()) . "\n";
	exit(1);
}

try {
	new PDO('dblib:host=127.0.0.1:1;dbname=master', 'user', 'pass', [PDO::ATTR_TIMEOUT => 1]);
	echo "new PDO('dblib:...') unexpectedly succeeded against a closed port.\n";
	exit(1);
} catch (PDOException $e) {
	// Expected: SQLSTATE[01002] "Adaptive Server connection failed" (or a
	// similar db-lib connection error) -- anything but a crash.
}

echo "pdo_dblib smoke test OK\n";
