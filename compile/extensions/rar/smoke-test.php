<?php
// Exercised by compile/check-shared-extension-symbols.mjs. A real archive,
// latest_winrar.rar from php-rar's own tests/ (712 bytes: 1.txt = "11111",
// 2.txt = "22222"), written to the VM's filesystem, then read three ways:
// RarEntry::getStream() (unrar's in-memory extraction), RarEntry::extract()
// (unrar writing files itself, through the core's libc), and the rar://
// stream wrapper.

function fail(string $message): never {
	echo $message, "\n";
	exit(1);
}

if (!extension_loaded('rar')) {
	fail('rar not loaded');
}

$dir = sys_get_temp_dir() . '/rar-smoke-test';
@mkdir($dir);
$archive = "$dir/latest_winrar.rar";
file_put_contents($archive, base64_decode('UmFyIRoHAJvXc0gADQAAAAAAAAAO6nSAkCoAEQAAAAUAAAACwHHeoOlQyzAdNQUAIAAAADEudHh0APC0c1CnGDEBd3Ou/w4Av4hn9qn/1OMddJCQKgAQAAAABQAAAALeGKlF7VDLMB01BQAgAAAAMi50eHQAsEQ7F4ewks10fk4AAL+IZ/ap/9TzwnoAwDYAAgIAAAICAAACSwLzSwAAAAAdMAIAAAAAAFJSUHJvdGVjdCsBAAAAAQAAAAAAAADcLlJhciEaBwCb13NIAA0AAAAAAAAADup0gJAqABEAAAAFAAAAAsBx3qDpUMswHTUFACAAAAAxLnR4dADwtHNQpxgxAXdzrv8OAL+IZ/ap/9TjHXSQkCoAEAAAAAUAAAAC3hipRe1QyzAdNQUAIAAAADIudHh0ALBEOxeHsJLNdH5OAAC/iGf2qf/UAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAxD17AEAHAA=='));

$expected = ['1.txt' => '11111', '2.txt' => '22222'];

$rar = RarArchive::open($archive);
if ($rar === false) {
	fail('RarArchive::open() failed');
}
$found = [];
foreach ($rar->getEntries() as $entry) {
	$found[$entry->getName()] = stream_get_contents($entry->getStream());
}
ksort($found);
if ($found !== $expected) {
	fail('getStream() contents: ' . json_encode($found));
}

$entry = $rar->getEntry('2.txt');
if (!$entry || !$entry->extract($dir)) {
	fail('RarEntry::extract() failed');
}
if (file_get_contents("$dir/2.txt") !== '22222') {
	fail('extracted 2.txt has the wrong contents');
}
$rar->close();

if (file_get_contents("rar://$archive#1.txt") !== '11111') {
	fail('rar:// wrapper read failed');
}

unlink("$dir/2.txt");
unlink($archive);
rmdir($dir);

echo "rar smoke test OK\n";
