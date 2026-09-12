/* Internal self-test fixture for the mode:shared pipeline (CLAUDE.md
 * decision 30/31) — not a real, publishable extension. */
#include "php.h"

ZEND_BEGIN_ARG_INFO(arginfo_kirigami_abi_probe, 0)
ZEND_END_ARG_INFO()

PHP_FUNCTION(kirigami_abi_probe)
{
	RETURN_LONG(42);
}

static const zend_function_entry kirigami_abi_probe_functions[] = {
	PHP_FE(kirigami_abi_probe, arginfo_kirigami_abi_probe)
	PHP_FE_END
};

zend_module_entry kirigami_abi_probe_module_entry = {
	STANDARD_MODULE_HEADER,
	"kirigami_abi_probe",
	kirigami_abi_probe_functions,
	NULL, NULL, NULL, NULL, NULL,
	"0.1.0",
	STANDARD_MODULE_PROPERTIES
};

ZEND_GET_MODULE(kirigami_abi_probe)
