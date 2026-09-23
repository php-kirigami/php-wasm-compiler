<?php
// Exercised by compile/check-shared-extension-symbols.mjs. No Firebird
// server is available in this check's isolated runtime boot, so this
// connects to a closed local port (127.0.0.1/1): libfbclient still runs its
// real attach path (yvalve's provider loop, the remote client's INET
// connection code) before failing -- a bare driver-registration check
// would miss anything only reached from inside that path.

if (!in_array('firebird', PDO::getAvailableDrivers(), true)) {
	echo "PDO driver 'firebird' not registered: " . implode(', ', PDO::getAvailableDrivers()) . "\n";
	exit(1);
}

try {
	new PDO('firebird:dbname=127.0.0.1/1:/tmp/smoke-test.fdb', 'SYSDBA', 'masterkey');
	echo "new PDO('firebird:...') unexpectedly succeeded against a closed port.\n";
	exit(1);
} catch (PDOException $e) {
	// Expected: a connection error (e.g. "Unable to complete network
	// request to host") -- anything but a crash.
}

echo "pdo_firebird smoke test OK\n";
