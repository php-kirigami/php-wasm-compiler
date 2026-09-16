<?php
// Exercised by compile/check-shared-extension-symbols.mjs. Deliberately
// does not connect to a real server -- PDO::getAvailableDrivers() alone
// proves the driver actually registered itself with the static-core PDO
// class (CLAUDE.md decision 47), which needs mysqlnd.so already loaded
// (PHP_ADD_EXTENSION_DEP(pdo_mysql, mysqlnd)).
if (!in_array('mysql', PDO::getAvailableDrivers(), true)) {
	fwrite(STDERR, "PDO::getAvailableDrivers() does not list 'mysql'.\n");
	exit(1);
}
