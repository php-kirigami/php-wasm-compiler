<?php
// Exercised by compile/check-shared-extension-symbols.mjs. Deliberately
// does not connect to a real server (none available in this diagnostic's
// bare hand-rolled loader) -- mysqli_init() alone is enough to exercise
// mysqlnd's own object/handle setup code, which is what actually needs
// mysqlnd.so to be loaded first (CLAUDE.md decision 42).
$mysqli = mysqli_init();
if (!($mysqli instanceof mysqli)) {
	fwrite(STDERR, "mysqli_init() did not return a mysqli instance.\n");
	exit(1);
}
