<?php
// Exercised by compile/check-shared-extension-symbols.mjs. posix_* functions
// mostly map to real, Emscripten-implemented libc calls (unlike the SysV IPC
// extensions in this same batch), so this smoke test asserts real results
// rather than just tolerating failure.
$pid = posix_getpid();
if (!is_int($pid) || $pid <= 0) {
	echo "posix_getpid() returned unexpected: " . var_export($pid, true) . "\n";
	exit(1);
}

$cwd = posix_getcwd();
if (!is_string($cwd) || $cwd === '') {
	echo "posix_getcwd() returned unexpected: " . var_export($cwd, true) . "\n";
	exit(1);
}
