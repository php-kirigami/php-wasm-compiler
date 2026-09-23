<?php
// Exercised by compile/check-shared-extension-symbols.mjs. There's no SNMP
// agent in this check's isolated runtime, so this runs libnetsnmp's
// library init, library settings and a session setup (UDP: ext/snmp takes
// the first ':' as the port separator, so it has no way to pick another
// transport), without sending a request (see below).

function fail(string $message): never {
	echo $message, "\n";
	exit(1);
}

if (!extension_loaded('snmp')) {
	fail('snmp not loaded');
}

// Library-side settings, no network involved.
snmp_set_valueretrieval(SNMP_VALUE_PLAIN);
if (snmp_get_valueretrieval() !== SNMP_VALUE_PLAIN) {
	fail('snmp_get_valueretrieval() did not round-trip');
}
snmp_set_oid_output_format(SNMP_OID_OUTPUT_NUMERIC);

// A session (net-snmp's session setup and transport open, a UDP socket),
// but no request: a request to an unreachable agent currently never
// returns in the WASM build (net-snmp keeps polling recvmsg() past its
// timeout), see docs/BUGS.md. Re-add a real request here once that's fixed.
$session = new SNMP(SNMP::VERSION_2c, '127.0.0.1:1', 'public', 100000, 0);
$session->valueretrieval = SNMP_VALUE_PLAIN;
if ($session->getErrno() !== SNMP::ERRNO_NOERROR) {
	fail('new SNMP() reported error ' . $session->getErrno() . ': ' . $session->getError());
}
$session->close();

echo "snmp smoke test OK\n";
