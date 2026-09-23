<?php
// Exercised by compile/check-shared-extension-symbols.mjs. No ODBC driver
// is shipped (unixODBC's driver manager only, see PROVENANCE.md), so a
// successful connection is impossible here by design -- this exercises the
// driver manager's own code paths instead, each of which can only fail:
//
// 1. A DSN lookup: reads odbc.ini/odbcinst.ini through unixODBC's own ini
//    parser (fopen/fgets/getenv/...), then fails with IM002 ("Data source
//    name not found").
// 2. A DSN-less connection naming a driver by path: the driver manager
//    tries to load it through libltdl (lt_dlopen -> dlopen), the path a
//    real driver would take, and fails with unixODBC's own SQLSTATE 01000
//    ("Can't open lib ... : file not found") -- not the ODBC spec's IM003.

$conn = @odbc_connect('NoSuchDSN', 'user', 'pass');
if ($conn !== false) {
	echo "odbc_connect() unexpectedly succeeded for a nonexistent DSN.\n";
	exit(1);
}
$state = odbc_error();
if ($state !== 'IM002') {
	echo "odbc_connect(NoSuchDSN): expected SQLSTATE IM002, got '$state': " . odbc_errormsg() . "\n";
	exit(1);
}

$conn = @odbc_connect('DRIVER=/nonexistent/libdriver.so;DATABASE=x', 'user', 'pass');
if ($conn !== false) {
	echo "odbc_connect() unexpectedly succeeded with a nonexistent driver.\n";
	exit(1);
}
$state = odbc_error();
if ($state !== '01000' || !str_contains(odbc_errormsg(), "Can't open lib")) {
	echo "odbc_connect(DRIVER=...): expected SQLSTATE 01000 (Can't open lib), got '$state': " . odbc_errormsg() . "\n";
	exit(1);
}

echo "odbc smoke test OK (" . ODBC_TYPE . ")\n";
