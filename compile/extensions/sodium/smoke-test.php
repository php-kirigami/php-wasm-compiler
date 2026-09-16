<?php
// Exercised by compile/check-shared-extension-symbols.mjs -- a real
// encrypt/decrypt round-trip, not just a function_exists() check, since
// this is exactly the kind of call that first surfaced sodium's own
// missing __stack_pointer/__table_base/free/emscripten_asm_const_int
// symbols (CLAUDE.md decisions 32/45).
$key = sodium_crypto_secretbox_keygen();
$nonce = random_bytes(SODIUM_CRYPTO_SECRETBOX_NONCEBYTES);
$ciphertext = sodium_crypto_secretbox('secret message', $nonce, $key);
$plaintext = sodium_crypto_secretbox_open($ciphertext, $nonce, $key);

if ($plaintext !== 'secret message') {
	fwrite(STDERR, "sodium_crypto_secretbox round-trip failed: " . var_export($plaintext, true) . "\n");
	exit(1);
}
