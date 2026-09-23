<?php
// Exercised by compile/check-shared-extension-symbols.mjs. A real non-WSDL
// SoapServer request/response round-trip: parses the request and serializes
// the response through soap.so's own bundled libxml2 copy, whose encoding
// layer is what needs iconv/bsearch from the core (load-only checking never
// reaches it).
function add($a, $b)
{
	return $a + $b;
}

$server = new SoapServer(null, ['uri' => 'urn:smoke']);
$server->addFunction('add');
$request = '<?xml version="1.0" encoding="UTF-8"?>'
	. '<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/" xmlns:ns1="urn:smoke">'
	. '<SOAP-ENV:Body><ns1:add><a>2</a><b>3</b></ns1:add></SOAP-ENV:Body></SOAP-ENV:Envelope>';

ob_start();
$server->handle($request);
$response = ob_get_clean();

if (!str_contains($response, 'addResponse') || !str_contains($response, '>5<')) {
	echo "SoapServer::handle() did not produce the expected response: " . var_export($response, true) . "\n";
	exit(1);
}

$fault = new SoapFault('Server', 'message');
if ($fault->getMessage() !== 'message' || !(new SoapVar('x', XSD_STRING)) instanceof SoapVar) {
	echo "SoapFault/SoapVar construction failed\n";
	exit(1);
}
