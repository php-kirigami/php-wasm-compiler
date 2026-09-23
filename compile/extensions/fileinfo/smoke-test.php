<?php
// Exercised by compile/check-shared-extension-symbols.mjs. Detects types
// from buffers and from a real file, through the compiled-in magic
// database (data_file.c): libmagic's softmagic (binary signatures),
// is_json and ascmagic paths, plus the stream-based file path.

function fail(string $message): never {
	echo $message, "\n";
	exit(1);
}

if (!extension_loaded('fileinfo')) {
	fail('fileinfo not loaded');
}

$finfo = new finfo(FILEINFO_MIME_TYPE);

$png = "\x89PNG\r\n\x1a\n\0\0\0\rIHDR\0\0\0\x01\0\0\0\x01\x08\x06\0\0\0\x1f\x15\xc4\x89";
$cases = [
	'image/png' => $png,
	'application/json' => '{"kirigami": [1, 2, 3]}',
	'text/plain' => "Hello, world.\nThis is plain text.\n",
	'application/pdf' => "%PDF-1.7\n%\xe2\xe3\xcf\xd3\n1 0 obj\n<< >>\nendobj\n",
];
foreach ($cases as $expected => $buffer) {
	$type = $finfo->buffer($buffer);
	if ($type !== $expected) {
		fail("finfo::buffer() returned " . var_export($type, true) . ", expected '$expected'");
	}
}

$path = sys_get_temp_dir() . '/fileinfo-smoke-test.png';
file_put_contents($path, $png);
$type = mime_content_type($path);
unlink($path);
if ($type !== 'image/png') {
	fail('mime_content_type() returned ' . var_export($type, true) . ", expected 'image/png'");
}

$description = (new finfo())->buffer($png);
if (!is_string($description) || !str_contains($description, 'PNG image data')) {
	fail('finfo::buffer() description was ' . var_export($description, true));
}

echo "fileinfo smoke test OK\n";
