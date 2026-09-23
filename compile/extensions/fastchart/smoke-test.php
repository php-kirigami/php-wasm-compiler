<?php
// Exercised by compile/check-shared-extension-symbols.mjs. The Symbol
// family (QrCode, Code128) first, which needs no font, in every output
// format, each through a different library: SVG (plutosvg), PNG (libpng),
// JPEG (libjpeg-turbo), WebP (libwebp), the rasterized ones through
// plutovg. Then a chart with a title, which needs FreeType and the default
// font embedded by wasm_default_font.c (see PROVENANCE.md, "Fonts").

function fail(string $message): never {
	echo $message, "\n";
	exit(1);
}

if (!extension_loaded('fastchart')) {
	fail('fastchart not loaded');
}

$qr = (new FastChart\QrCode())
	->setData('https://php-kirigami.github.io')
	->setEcc(FastChart\QrCode::ECC_M)
	->setSize(200, 200);

$svg = $qr->renderSvg();
if (!str_contains($svg, '<svg')) {
	fail('QrCode::renderSvg() did not return SVG: ' . substr($svg, 0, 80));
}

$checks = [
	'PNG' => [$qr->renderPng(), "\x89PNG\r\n\x1a\n"],
	'JPEG' => [$qr->renderJpeg(), "\xFF\xD8\xFF"],
	'WebP' => [$qr->renderWebp(), 'RIFF'],
];
foreach ($checks as $format => [$bytes, $magic]) {
	if (!str_starts_with($bytes, $magic)) {
		fail("QrCode render$format() output doesn't start with the $format signature");
	}
}

$png = (new FastChart\Code128())
	->setData('KIRIGAMI-2026')
	->setShowText(false)
	->setSize(300, 80)
	->renderPng();
if (!str_starts_with($png, "\x89PNG\r\n\x1a\n")) {
	fail('Code128::renderPng() output is not a PNG');
}

// A real chart with text: it only works through the default font
// wasm_default_font.c installs when the module loads.
if (!is_file('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf')) {
	fail('the embedded default font was not installed');
}
// fastchart draws text as glyph outlines (paths), not <text>, so the title
// can't be searched for: the same chart is rendered with and without it,
// and the title has to change both the SVG and the PNG.
$make = fn() => (new FastChart\LineChart())
	->setSize(400, 250)
	->setSeries([3, 1, 4, 1, 5, 9, 2, 6]);
$plain = $make();
$titled = $make()->setTitle('Kirigami');
$svg = $titled->renderSvg();
if (!str_contains($svg, '<svg')) {
	fail('LineChart::renderSvg() did not return SVG');
}
if ($svg === $plain->renderSvg()) {
	fail('LineChart::renderSvg(): the title changed nothing (no text drawn)');
}
$png = $titled->renderPng();
if (!str_starts_with($png, "\x89PNG\r\n\x1a\n")) {
	fail('LineChart::renderPng() output is not a PNG');
}
if ($png === $plain->renderPng()) {
	fail('LineChart::renderPng(): the title changed nothing (no text drawn)');
}

echo "fastchart smoke test OK\n";
