<?php
// Exercised by compile/check-shared-extension-symbols.mjs. Touches every
// part of ICU intl.so links: locale data from the compiled-in data
// (formatting in a non-root locale is the real proof the ~32 MB data is
// there, since a missing locale silently falls back to root), collation,
// normalization, transliteration, IDNA, break iteration, message
// formatting, and ext/date interop (IntlDateFormatter/IntlCalendar, which
// call into the core's static ext/date).

function fail(string $message): never {
	echo $message, "\n";
	exit(1);
}

// NBSP and narrow NBSP (CLDR's French grouping separator) as plain spaces.
function spaces(string $s): string {
	return str_replace(["\u{00A0}", "\u{202F}"], ' ', $s);
}

if (!extension_loaded('intl')) {
	fail('intl not loaded');
}

// Normalizer must be intl's own: php-norm (static) defers to it
// (ZEND_MOD_OPTIONAL("intl"), see docs/DECISIONS.md).
$normalizerExt = (new ReflectionClass('Normalizer'))->getExtensionName();
if ($normalizerExt !== 'intl') {
	fail("Normalizer comes from '$normalizerExt', expected 'intl'");
}

$checks = [
	'NumberFormatter fr_FR' => [
		spaces((new NumberFormatter('fr_FR', NumberFormatter::DECIMAL))->format(1234567.891)),
		'1 234 567,891',
	],
	'NumberFormatter currency de_DE' => [
		spaces((new NumberFormatter('de_DE', NumberFormatter::CURRENCY))->formatCurrency(1234.5, 'EUR')),
		'1.234,50 €',
	],
	'NumberFormatter spellout en' => [
		(new NumberFormatter('en', NumberFormatter::SPELLOUT))->format(42),
		'forty-two',
	],
	'IntlDateFormatter ja_JP' => [
		(new IntlDateFormatter('ja_JP', IntlDateFormatter::LONG, IntlDateFormatter::NONE, 'UTC'))->format(0),
		'1970年1月1日',
	],
	'Locale::getDisplayLanguage' => [
		Locale::getDisplayLanguage('de', 'fr'),
		'allemand',
	],
	'Normalizer NFC' => [
		Normalizer::normalize("e\u{0301}", Normalizer::FORM_C),
		"\u{00E9}",
	],
	'Transliterator' => [
		Transliterator::create('Any-Latin; Latin-ASCII')->transliterate('Ελληνικά'),
		'Ellenika',
	],
	'idn_to_ascii' => [
		idn_to_ascii('bücher.example'),
		'xn--bcher-kva.example',
	],
	'grapheme_strlen' => [
		grapheme_strlen("e\u{0301}👍🏽"),
		2,
	],
	'MessageFormatter plural' => [
		MessageFormatter::formatMessage('en', '{n, plural, one{# item} other{# items}}', ['n' => 3]),
		'3 items',
	],
	'IntlChar::charName' => [
		IntlChar::charName('ß'),
		'LATIN SMALL LETTER SHARP S',
	],
];
foreach ($checks as $name => [$actual, $expected]) {
	if ($actual !== $expected) {
		fail("$name: got " . var_export($actual, true) . ', expected ' . var_export($expected, true));
	}
}

$words = ['zèbre', 'Zoo', 'école', 'eau'];
(new Collator('fr_FR'))->sort($words);
if ($words !== ['eau', 'école', 'zèbre', 'Zoo']) {
	fail('Collator fr_FR sort: ' . json_encode($words, JSON_UNESCAPED_UNICODE));
}

$it = IntlBreakIterator::createWordInstance('en');
$it->setText('Hello, wide world');
$words = [];
foreach ($it->getPartsIterator() as $part) {
	if (trim($part, " ,") !== '') {
		$words[] = $part;
	}
}
if ($words !== ['Hello', 'wide', 'world']) {
	fail('IntlBreakIterator words: ' . json_encode($words));
}

$cal = IntlCalendar::createInstance('UTC', 'en_US');
$cal->setTime(0);
$dt = $cal->toDateTime();
if (!$dt instanceof DateTime || $dt->format('Y-m-d') !== '1970-01-01') {
	fail('IntlCalendar::toDateTime() did not round-trip through ext/date');
}

echo "intl smoke test OK\n";
