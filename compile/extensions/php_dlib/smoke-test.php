<?php
// Exercised by compile/check-shared-extension-symbols.mjs. Covers the
// dlib-backed paths that need no model file: chinese_whispers (clustering),
// dlib_vector_length, and dlib_face_detection() on a PNG and a JPEG written
// with gd (the HOG detector plus dlib's bundled libpng/libjpeg). The images
// hold no face, so a successful run returns an empty array, not false.

function fail(string $message): never {
	echo $message, "\n";
	exit(1);
}

if (!extension_loaded('php_dlib')) {
	fail('php_dlib not loaded');
}

if (dlib_chinese_whispers([[0, 0], [0, 1], [1, 0], [1, 1]]) !== [0, 0]) {
	fail('chinese_whispers: two connected nodes should share a label');
}
if (dlib_chinese_whispers([[0, 0], [1, 1]]) !== [0, 1]) {
	fail('chinese_whispers: two separate nodes should differ');
}

$len = dlib_vector_length([0.0, 0.0], [3.0, 4.0]);
if (abs($len - 5.0) > 1e-9) {
	fail("vector_length: $len");
}

$img = imagecreatetruecolor(160, 160);
imagefilledrectangle($img, 0, 0, 159, 159, imagecolorallocate($img, 200, 200, 200));
imagefilledellipse($img, 80, 80, 90, 90, imagecolorallocate($img, 40, 40, 160));
foreach (['png' => 'imagepng', 'jpg' => 'imagejpeg'] as $ext => $write) {
	$path = tempnam(sys_get_temp_dir(), 'dlib') . ".$ext";
	$write($img, $path);
	$faces = dlib_face_detection($path);
	unlink($path);
	if ($faces !== []) {
		fail("face_detection ($ext): " . var_export($faces, true));
	}
}

echo "php_dlib smoke test OK\n";
