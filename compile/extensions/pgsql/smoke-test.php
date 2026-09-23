<?php
// Exercised by compile/check-shared-extension-symbols.mjs. No real
// PostgreSQL server is available in this check's isolated runtime boot, so
// this only exercises what's safe without one: pg_connect() against a
// closed local port (127.0.0.1:1, effectively guaranteed to refuse the
// connection immediately -- no real server needed, no hang) still runs
// libpq's real fe-connect.c connection-establishment code path (socket(),
// connect(), ...), which is exactly where dba's own three lazily-resolved
// missing symbols (CLAUDE.md decision 47's follow-up) were found -- a bare
// function_exists() check would miss anything only reached from inside
// that path.
$conn = @pg_connect('host=127.0.0.1 port=1 connect_timeout=1', PGSQL_CONNECT_FORCE_NEW);
if ($conn !== false) {
	echo "pg_connect() unexpectedly succeeded against a closed port.\n";
	exit(1);
}
