<?php
// Exercised by compile/check-shared-extension-symbols.mjs. No ODBC driver
// is shipped (unixODBC's driver manager only, see
// compile/extensions/odbc/PROVENANCE.md), so a successful connection is
// impossible here by design. Both attempts below go through the driver
// manager's real code paths and can only fail: a DSN lookup (odbc.ini/
// odbcinst.ini parsing, SQLSTATE IM002), then a DSN-less connection naming
// a driver by path (libltdl's lt_dlopen -> dlopen, unixODBC's own SQLSTATE
// 01000 "Can't open lib"). The SQLSTATE is read from the exception message:
// a PDOException thrown by the constructor carries errorInfo[0] = 0 here.

if (!in_array('odbc', PDO::getAvailableDrivers(), true)) {
	echo "PDO driver 'odbc' not registered: " . implode(', ', PDO::getAvailableDrivers()) . "\n";
	exit(1);
}

foreach (['odbc:NoSuchDSN' => 'IM002', 'odbc:DRIVER=/nonexistent/libdriver.so;DATABASE=x' => '01000'] as $dsn => $expected) {
	try {
		new PDO($dsn, 'user', 'pass');
		echo "new PDO('$dsn') unexpectedly succeeded.\n";
		exit(1);
	} catch (PDOException $e) {
		$state = preg_match('/SQLSTATE\[(\w+)\]/', $e->getMessage(), $m) ? $m[1] : '?';
		if ($state !== $expected) {
			echo "new PDO('$dsn'): expected SQLSTATE $expected, got '$state': " . $e->getMessage() . "\n";
			exit(1);
		}
	}
}

echo "pdo_odbc smoke test OK\n";
