<?php
// Exercised by compile/check-shared-extension-symbols.mjs. Real calls into
// musl's libintl functions (lazy symbols, invisible to load-only checking),
// plus the ngettext family, which only exists at the PHP level when
// config.m4's HAVE_NGETTEXT/HAVE_DNGETTEXT/HAVE_DCNGETTEXT probes succeed
// (see config.yaml's gettext entry).
foreach (['ngettext', 'dngettext', 'dcngettext', 'bind_textdomain_codeset'] as $fn) {
	if (!function_exists($fn)) {
		echo "$fn() is missing -- config.m4's probe for it failed\n";
		exit(1);
	}
}

@mkdir('/tmp/locale', 0777, true);

$checks = [
	'bindtextdomain' => is_string(bindtextdomain('messages', '/tmp/locale')),
	// musl is UTF-8-only: its bind_textdomain_codeset() always returns NULL
	// (false at the PHP level), so only check the call itself works.
	'bind_textdomain_codeset' => in_array(bind_textdomain_codeset('messages', 'UTF-8'), ['UTF-8', false], true),
	'textdomain' => textdomain('messages') === 'messages',
	'gettext' => gettext('Hello') === 'Hello' && _('World') === 'World',
	'dgettext' => dgettext('messages', 'Hello') === 'Hello',
	'dcgettext' => dcgettext('messages', 'Hello', LC_MESSAGES) === 'Hello',
	'ngettext' => ngettext('one', 'many', 1) === 'one' && ngettext('one', 'many', 2) === 'many',
	'dngettext' => dngettext('messages', 'one', 'many', 2) === 'many',
	'dcngettext' => dcngettext('messages', 'one', 'many', 2, LC_MESSAGES) === 'many',
];
foreach ($checks as $name => $ok) {
	if (!$ok) {
		echo "$name() returned an unexpected result\n";
		exit(1);
	}
}
