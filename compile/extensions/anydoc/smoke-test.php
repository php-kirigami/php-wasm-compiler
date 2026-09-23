<?php
// Exercised by compile/check-shared-extension-symbols.mjs. Converts three
// documents built on the fly, each through a different part of anydoc:
// CSV (its csv reader), a minimal DOCX made with the core's ZipArchive
// (zip + quick-xml), and a minimal one-page PDF (pdf-inspector/lopdf,
// which also brings in rayon: the VM has no threads, so this is the check
// that its thread pool degrades instead of aborting).

function fail(string $message): never {
	echo $message, "\n";
	exit(1);
}

if (!extension_loaded('anydoc')) {
	fail('anydoc not loaded');
}

// CSV.
$csv = "name,score\nAda,42\nLinus,7\n";
if (anydoc_format_from_bytes($csv) === null && anydoc_format_from_extension('csv') === null) {
	fail('no format detected for CSV');
}
$md = anydoc_to_markdown_bytes($csv, 'csv');
if (!str_contains($md, 'Ada') || !str_contains($md, '|')) {
	fail('CSV to Markdown: ' . var_export($md, true));
}

// DOCX.
$docxPath = sys_get_temp_dir() . '/anydoc-smoke-test.docx';
$zip = new ZipArchive();
$zip->open($docxPath, ZipArchive::CREATE | ZipArchive::OVERWRITE);
$zip->addFromString('[Content_Types].xml', '<?xml version="1.0" encoding="UTF-8"?><Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/><Default Extension="xml" ContentType="application/xml"/><Override PartName="/word/document.xml" ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/></Types>');
$zip->addFromString('_rels/.rels', '<?xml version="1.0" encoding="UTF-8"?><Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships"><Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="word/document.xml"/></Relationships>');
$zip->addFromString('word/document.xml', '<?xml version="1.0" encoding="UTF-8"?><w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:r><w:t>Hello from Kirigami</w:t></w:r></w:p></w:body></w:document>');
$zip->close();
$md = anydoc_to_markdown($docxPath);
unlink($docxPath);
if (!str_contains($md, 'Hello from Kirigami')) {
	fail('DOCX to Markdown: ' . var_export($md, true));
}

// PDF: one page, one line of text, with a correct xref table.
$objects = [
	'<< /Type /Catalog /Pages 2 0 R >>',
	'<< /Type /Pages /Kids [3 0 R] /Count 1 >>',
	'<< /Type /Page /Parent 2 0 R /MediaBox [0 0 300 144] /Contents 4 0 R /Resources << /Font << /F1 5 0 R >> >> >>',
	null, // content stream, below
	'<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>',
];
$stream = "BT /F1 18 Tf 36 72 Td (PDF from Kirigami) Tj ET";
$objects[3] = "<< /Length " . strlen($stream) . " >>\nstream\n$stream\nendstream";
$pdf = "%PDF-1.4\n";
$offsets = [];
foreach ($objects as $i => $body) {
	$offsets[] = strlen($pdf);
	$pdf .= ($i + 1) . " 0 obj\n$body\nendobj\n";
}
$xref = strlen($pdf);
$pdf .= "xref\n0 " . (count($objects) + 1) . "\n0000000000 65535 f \n";
foreach ($offsets as $offset) {
	$pdf .= sprintf("%010d 00000 n \n", $offset);
}
$pdf .= "trailer\n<< /Size " . (count($objects) + 1) . " /Root 1 0 R >>\nstartxref\n$xref\n%%EOF\n";
$md = anydoc_to_markdown_bytes($pdf, 'pdf');
if (!str_contains($md, 'PDF from Kirigami')) {
	fail('PDF to Markdown: ' . var_export($md, true));
}

echo "anydoc smoke test OK\n";
