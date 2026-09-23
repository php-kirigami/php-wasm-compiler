<?php
// Exercised by compile/check-shared-extension-symbols.mjs. Adapted from
// the extension's own tests/002-encode-raw.phpt: encodeRaw() at every
// error-correction level (the whole C++ core: segmenting, Reed-Solomon,
// matrix placement, mask selection), plus a check of the finder pattern.
// NativeEncoderCore reads ->value off any int-backed enum, so
// crazy-goat/scanmephp itself isn't needed.

function fail(string $message): never {
	echo $message, "\n";
	exit(1);
}

if (!extension_loaded('scanmeqr')) {
	fail('scanmeqr not loaded');
}

enum Ecl: int { case Low = 0; case Medium = 1; case Quartile = 2; case High = 3; }

$encoder = new CrazyGoat\ScanMePHP\NativeEncoderCore();

$expected = [
	'Low' => [1, 21],
	'Medium' => [1, 21],
	'Quartile' => [1, 21],
	'High' => [2, 25],
];
foreach (Ecl::cases() as $ecl) {
	$r = $encoder->encodeRaw('HELLO WORLD', $ecl);
	[$version, $size] = $expected[$ecl->name];
	if ($r['version'] !== $version || $r['size'] !== $size || count($r['data']) !== $size * $size) {
		fail("{$ecl->name}: version {$r['version']}, size {$r['size']}, " . count($r['data']) . ' modules');
	}
}

// A version-1 symbol has a dark 7x7 finder in three corners: (0,0), (6,0)
// and (0,6) are dark, (7,7) just outside the separator is light.
$r = $encoder->encodeRaw('HELLO WORLD', Ecl::Low);
$at = fn(int $x, int $y) => $r['data'][$y * $r['size'] + $x] ? '#' : '.';
$finder = $at(0, 0) . $at(6, 0) . $at(0, 6) . $at(7, 7);
if ($finder !== '###.') {
	fail("finder pattern: $finder");
}

echo "scanmeqr smoke test OK\n";
