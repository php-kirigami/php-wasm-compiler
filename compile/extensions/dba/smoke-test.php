<?php
// Exercised by compile/check-shared-extension-symbols.mjs. This exact
// open/insert/fetch round-trip against the "flatfile" handler is what
// surfaced dba's real, lazily-resolved missing symbols (strcasecmp, atoi,
// memcmp -- CLAUDE.md decision 47's follow-up) -- a bare function_exists()
// or load-only check would have missed all three, since none of them are
// reached until dba's own handler code actually runs on real data.
$path = '/tmp/dba-smoke-test.db';
$db = dba_open($path, 'n', 'flatfile');
if ($db === false) {
	echo "dba_open() failed.\n";
	exit(1);
}

dba_insert('greeting', 'hello_dba', $db);
$value = dba_fetch('greeting', $db);
dba_close($db);

if ($value !== 'hello_dba') {
	echo "dba_fetch() returned " . var_export($value, true) . ", expected 'hello_dba'.\n";
	exit(1);
}
