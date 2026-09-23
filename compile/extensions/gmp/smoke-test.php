<?php
// Exercised by compile/check-shared-extension-symbols.mjs. Real arithmetic,
// string conversion and primality calls into the vendored libgmp, not just
// loading the module.
$checks = [
	'gmp_pow' => gmp_strval(gmp_pow(2, 100)) === '1267650600228229401496703205376',
	'operators' => (string)(gmp_init('123456789012345678901234567890') * 3) === '370370367037037036703703703670',
	'bases' => gmp_strval(gmp_init('ff', 16), 2) === '11111111',
	'gmp_prob_prime' => gmp_prob_prime('170141183460469231731687303715884105727') > 0,
];
foreach ($checks as $name => $ok) {
	if (!$ok) {
		echo "$name returned an unexpected result\n";
		exit(1);
	}
}
