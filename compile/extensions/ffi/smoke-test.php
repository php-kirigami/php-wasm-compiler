<?php
// Exercised by compile/check-shared-extension-symbols.mjs. FFI::cdef()
// with no library resolves symbols from what's already loaded (dlsym on
// RTLD_DEFAULT), i.e. the core php.wasm's own exports. Each check reaches
// a different part of libffi's wasm32 port (src/wasm/ffi.c): a plain
// ffi_call through its EM_JS trampoline, then a closure (a PHP callable
// passed as a C function pointer), which goes through
// ffi_closure_alloc_js/convertJsFunctionToWasm.

function fail(string $message): never {
	echo $message, "\n";
	exit(1);
}

if (!extension_loaded('ffi')) {
	fail('ffi not loaded');
}
if (ini_get('ffi.enable') !== '1') {
	fail("ffi.enable is '" . ini_get('ffi.enable') . "', expected '1' (the SAPI is not \"cli\", so \"preload\" would block the FFI API)");
}

// Every C function declared here must be exported by the core (see
// .JS_ABI_EXPORTS in compile/php/Dockerfile), or cdef() fails to resolve it.
// new()/cast()/type() are called on this instance: their static forms are
// deprecated since PHP 8.3.
$libc = FFI::cdef('
	size_t strlen(const char *s);
	void qsort(void *base, size_t nmemb, size_t size, int (*compar)(const void *, const void *));
');

// Pure data: no libffi call involved.
$buf = $libc->new('int[4]');
for ($i = 0; $i < 4; $i++) {
	$buf[$i] = [3, 1, 4, 1][$i];
}
if (FFI::sizeof($buf) !== 16) {
	fail('FFI::sizeof(int[4]) = ' . FFI::sizeof($buf));
}

// ffi_call through the EM_JS trampoline.
if ($libc->strlen('kirigami') !== 8) {
	fail('strlen() via FFI returned ' . var_export($libc->strlen('kirigami'), true));
}

// A closure: qsort() calls back into PHP for each comparison.
$libc->qsort($buf, 4, FFI::sizeof($libc->type('int')), function ($a, $b) use ($libc) {
	$x = $libc->cast('int *', $a)[0];
	$y = $libc->cast('int *', $b)[0];
	return $x <=> $y;
});
$sorted = [$buf[0], $buf[1], $buf[2], $buf[3]];
if ($sorted !== [1, 1, 3, 4]) {
	fail('qsort() with a PHP callback produced ' . json_encode($sorted));
}

echo "ffi smoke test OK\n";
