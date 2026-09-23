<?php
// Exercised by compile/check-shared-extension-symbols.mjs. A real
// tidy_parse_string()/tidy_clean_repair() round-trip, not just loading the
// module -- same "exercise the real code path" reasoning as dba's own
// smoke test (CLAUDE.md decision 47's follow-up).
$tidy = tidy_parse_string('<html><body><p>unclosed', null, 'utf8');
$tidy->cleanRepair();
$html = tidy_get_output($tidy);

if (!str_contains($html, '<html>') || !str_contains($html, '</html>')) {
	echo "tidy_parse_string()/cleanRepair() did not produce well-formed HTML: " . var_export($html, true) . "\n";
	exit(1);
}
