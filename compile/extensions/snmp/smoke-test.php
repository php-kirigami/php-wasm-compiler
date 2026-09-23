<?php
// Exercised by compile/check-shared-extension-symbols.mjs. There's no SNMP
// agent in this check's isolated runtime, so this runs libnetsnmp's
// library init, library settings, then a request to a closed port (UDP:
// ext/snmp takes the first ':' as the port separator, so it has no way to
// pick another transport).

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

// A real request to a closed port: net-snmp's session, transport and
// synchronous response loop, which must give up at its 100 ms timeout
// (it once hung: the datagram socket looked readable forever while every
// read gave EAGAIN). The failure is a warning and false, or an
// SNMPException; only a result, or not returning, is wrong.
$session = new SNMP(SNMP::VERSION_2c, '127.0.0.1:1', 'public', 100000, 0);
$session->exceptions_enabled = SNMP::ERRNO_ANY;
$started = microtime(true);
try {
	$result = @$session->get('.1.3.6.1.2.1.1.1.0');
	if ($result !== false) {
		fail('SNMP::get() unexpectedly returned ' . var_export($result, true) . ' from a closed port.');
	}
} catch (SNMPException $e) {
	// Expected: a timeout or connection error.
}
if (microtime(true) - $started > 10) {
	fail('SNMP::get() took more than 10 s to give up');
}
$session->close();

echo "snmp smoke test OK\n";
