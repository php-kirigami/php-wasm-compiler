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
$chart = (new FastChart\LineChart())
	->setSize(400, 250)
	->setTitle('Kirigami')
	->setSeries([3, 1, 4, 1, 5, 9, 2, 6]);
$svg = $chart->renderSvg();
if (!str_contains($svg, '<svg') || !str_contains($svg, 'Kirigami')) {
	fail('LineChart::renderSvg() output has no title text');
}
if (!str_starts_with($chart->renderPng(), "\x89PNG\r\n\x1a\n")) {
	fail('LineChart::renderPng() output is not a PNG');
}

echo "fastchart smoke test OK\n";
