<?php
// Exercised by compile/check-shared-extension-symbols.mjs. No spell-checking
// provider is vendored yet (see PROVENANCE.md), so this sticks to what works
// without one: broker setup (glib's charset/locale detection) and Enchant's
// built-in personal word list, which reads a real file and runs real
// suggestion code -- load-only checking reaches none of it.
$broker = enchant_broker_init();
if (!$broker instanceof EnchantBroker) {
	echo "enchant_broker_init() failed\n";
	exit(1);
}
if (!is_array(enchant_broker_describe($broker)) || !is_array(enchant_broker_list_dicts($broker))) {
	echo "enchant_broker_describe()/list_dicts() did not return arrays\n";
	exit(1);
}

$pwl = '/tmp/enchant-smoke-test.pwl';
file_put_contents($pwl, "hello\nworld\n");
$dict = enchant_broker_request_pwl_dict($broker, $pwl);
if (!$dict instanceof EnchantDictionary) {
	echo "enchant_broker_request_pwl_dict() failed: " . var_export(enchant_broker_get_error($broker), true) . "\n";
	exit(1);
}
if (!enchant_dict_check($dict, 'hello') || enchant_dict_check($dict, 'helo')) {
	echo "enchant_dict_check() gave the wrong answer\n";
	exit(1);
}
// A PWL-only dictionary never has suggestions, by design: libenchant 2.5.0
// removed its own PWL suggestion engine (its NEWS: "Enchant's mechanism for
// generating suggestions from personal wordlists is removed"), leaving
// suggestions to the provider (Hunspell/Aspell), none of which is vendored
// yet. So this only checks the call itself runs and returns an array.
if (!is_array(enchant_dict_suggest($dict, 'helo'))) {
	echo "enchant_dict_suggest() did not return an array\n";
	exit(1);
}
