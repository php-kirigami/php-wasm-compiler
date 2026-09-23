/*
  +----------------------------------------------------------------------+
  | PHP Version 8.1+                                                     |
  +----------------------------------------------------------------------+
  | Copyright (c) 2024 Crazy Goat                                        |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_scanme_qr.h"
#include "scanme_qr.h"
#include "native_encoder.c"

/* {{{ scanmeqr_functions[] */

static const zend_function_entry scanmeqr_functions[] = {
    PHP_FE_END
};
/* }}} */

/* Declare module functions */
static PHP_MINIT_FUNCTION(scanmeqr);
static PHP_MINFO_FUNCTION(scanmeqr);

/* {{{ scanmeqr_module_entry */
zend_module_entry scanmeqr_module_entry = {
    STANDARD_MODULE_HEADER,
    "scanmeqr",                    /* Extension name */
    scanmeqr_functions,            /* Function entries */
    PHP_MINIT(scanmeqr),           /* Module init */
    NULL,                           /* Module shutdown */
    NULL,                           /* Request init */
    NULL,                           /* Request shutdown */
    PHP_MINFO(scanmeqr),           /* Module info */
    PHP_SCANME_QR_VERSION,          /* Extension version */
    STANDARD_MODULE_PROPERTIES
};

/* }}} */

#ifdef COMPILE_DL_SCANMEQR
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
#endif
ZEND_GET_MODULE(scanmeqr)
#endif

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(scanmeqr)
{
    /* Register NativeEncoder class */
    scanme_qr_register_native_encoder(NULL);

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION */
static PHP_MINFO_FUNCTION(scanmeqr)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "scanmeqr support", "enabled");
    php_info_print_table_row(2, "Extension version", PHP_SCANME_QR_VERSION);
    
    /* Get C library version */
    const char* lib_version = scanme_qr_version();
    php_info_print_table_row(2, "C library version", lib_version ? lib_version : "unknown");
    
    php_info_print_table_end();
}
/* }}} */
