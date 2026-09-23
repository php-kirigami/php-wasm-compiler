/*
  +----------------------------------------------------------------------+
  | NativeEncoderExt class implementation                                |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "zend_exceptions.h"
#include "zend_interfaces.h"
#include "ext/standard/info.h"
#include "php_scanme_qr.h"
#include "native_encoder.h"
#include "scanme_qr.h"

/* Object structure for NativeEncoderExt */
typedef struct {
    zend_object std;
} scanme_qr_native_encoder_object;

/* Class entry pointer */
zend_class_entry *scanme_qr_native_encoder_ce;

/* Forward declarations */
static zend_object *scanme_qr_native_encoder_create(zend_class_entry *class_type);
static void scanme_qr_native_encoder_free(zend_object *object);
static PHP_METHOD(NativeEncoderExt, encodeRaw);
static PHP_METHOD(NativeEncoderExt, encodeMatrix);

/* Arginfo for encodeRaw() */
ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_NativeEncoderExt_encodeRaw, 0, 2, IS_ARRAY, 0)
    ZEND_ARG_TYPE_INFO(0, url, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, errorCorrectionLevel, IS_OBJECT, 0)
ZEND_END_ARG_INFO()

/* Arginfo for encodeMatrix() */
ZEND_BEGIN_ARG_WITH_RETURN_OBJ_INFO_EX(arginfo_NativeEncoderExt_encodeMatrix, 0, 2, "CrazyGoat\\ScanMePHP\\Matrix", 0)
    ZEND_ARG_TYPE_INFO(0, url, IS_STRING, 0)
    ZEND_ARG_TYPE_INFO(0, errorCorrectionLevel, IS_OBJECT, 0)
ZEND_END_ARG_INFO()

/* Method entries */
static const zend_function_entry native_encoder_methods[] = {
    PHP_ME(NativeEncoderExt, encodeRaw, arginfo_NativeEncoderExt_encodeRaw, ZEND_ACC_PUBLIC)
    PHP_ME(NativeEncoderExt, encodeMatrix, arginfo_NativeEncoderExt_encodeMatrix, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

static zend_object_handlers scanme_qr_native_encoder_handlers;

zend_object *scanme_qr_create_native_encoder(void)
{
    return scanme_qr_native_encoder_create(scanme_qr_native_encoder_ce);
}

static zend_object *scanme_qr_native_encoder_create(zend_class_entry *class_type)
{
    scanme_qr_native_encoder_object *intern;

    intern = zend_object_alloc(sizeof(scanme_qr_native_encoder_object), class_type);
    zend_object_std_init(&intern->std, class_type);
    object_properties_init(&intern->std, class_type);

    intern->std.handlers = &scanme_qr_native_encoder_handlers;

    return &intern->std;
}

static void scanme_qr_native_encoder_free(zend_object *object)
{
    zend_object_std_dtor(object);
}

static int get_ecl_from_enum(zval *ecl_obj)
{
    zval retval;
    zval *value_prop = zend_read_property(Z_OBJCE_P(ecl_obj), Z_OBJ_P(ecl_obj), "value", sizeof("value") - 1, 1, &retval);
    
    if (!value_prop || Z_TYPE_P(value_prop) != IS_LONG) {
        return -1;
    }
    
    return (int)Z_LVAL_P(value_prop);
}

static PHP_METHOD(NativeEncoderExt, encodeRaw)
{
    zend_string *url;
    zval *ecl_obj;
    
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(url)
        Z_PARAM_OBJECT(ecl_obj)
    ZEND_PARSE_PARAMETERS_END();

    int ecl_value = get_ecl_from_enum(ecl_obj);
    if (ecl_value < 0) {
        zend_throw_exception(zend_ce_exception, "ErrorCorrectionLevel must be an integer backed enum", 0);
        return;
    }

    scanme_qr_result_t result;
    int ret = scanme_qr_encode(ZSTR_VAL(url), ZSTR_LEN(url), ecl_value, &result);

    if (ret != 0) {
        scanme_qr_result_free(&result);
        zend_throw_exception(zend_ce_exception, "Failed to encode QR code", ret);
        return;
    }

    array_init(return_value);
    add_assoc_long(return_value, "version", result.version);
    add_assoc_long(return_value, "size", result.size);
    
    zval data_array;
    array_init(&data_array);
    
    int total_modules = result.size * result.size;
    for (int i = 0; i < total_modules; i++) {
        add_next_index_bool(&data_array, result.modules[i] != 0);
    }
    
    add_assoc_zval(return_value, "data", &data_array);
    scanme_qr_result_free(&result);
}

static PHP_METHOD(NativeEncoderExt, encodeMatrix)
{
    zend_string *url;
    zval *ecl_obj;
    
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(url)
        Z_PARAM_OBJECT(ecl_obj)
    ZEND_PARSE_PARAMETERS_END();

    int ecl_value = get_ecl_from_enum(ecl_obj);
    if (ecl_value < 0) {
        zend_throw_exception(zend_ce_exception, "ErrorCorrectionLevel must be an integer backed enum", 0);
        return;
    }

    scanme_qr_matrix_t* matrix = scanme_qr_encode_matrix(ZSTR_VAL(url), ZSTR_LEN(url), ecl_value);
    if (!matrix) {
        zend_throw_exception(zend_ce_exception, "Failed to encode QR code", 0);
        return;
    }

    /* Find CrazyGoat\ScanMePHP\Matrix class */
    zend_string *class_name = zend_string_init("CrazyGoat\\ScanMePHP\\Matrix", sizeof("CrazyGoat\\ScanMePHP\\Matrix") - 1, 0);
    zend_class_entry *matrix_ce = zend_lookup_class(class_name);
    zend_string_release(class_name);
    if (!matrix_ce) {
        scanme_qr_matrix_free(matrix);
        zend_throw_exception(zend_ce_exception, "CrazyGoat\\ScanMePHP\\Matrix class not found", 0);
        return;
    }

    /* Hand the modules over as one '0'/'1' byte per module
       (Matrix::fromModuleString): no size*size zvals to build here, and it is
       the representation the renderers consume directly (substr/strtr/preg),
       so a bool[] would only be converted back to this string on first render. */
    uint32_t total_modules = (uint32_t)(matrix->size * matrix->size);
    zend_string *modules = zend_string_alloc(total_modules, 0);
    char *out = ZSTR_VAL(modules);
    for (uint32_t i = 0; i < total_modules; i++) {
        out[i] = matrix->data[i] ? '1' : '0';
    }
    out[total_modules] = '\0';

    /* Matrix::fromModuleString($version, $modules) */
    zval args[2];
    ZVAL_LONG(&args[0], matrix->version);
    ZVAL_STR(&args[1], modules);
    zend_call_method_with_2_params(NULL, matrix_ce, NULL, "frommodulestring", return_value, &args[0], &args[1]);
    zval_ptr_dtor(&args[1]);
    scanme_qr_matrix_free(matrix);
}

/*
 * Register NativeEncoder class
 */
void scanme_qr_register_native_encoder(zend_class_entry *parent_ce)
{
    zend_class_entry ce;

    INIT_CLASS_ENTRY(ce, "CrazyGoat\\ScanMePHP\\NativeEncoderCore", native_encoder_methods);
    scanme_qr_native_encoder_ce = zend_register_internal_class(&ce);
    scanme_qr_native_encoder_ce->create_object = scanme_qr_native_encoder_create;
    // Usuwamy flagę FINAL, żeby można było po niej dziedziczyć w PHP
    // scanme_qr_native_encoder_ce->ce_flags |= ZEND_ACC_FINAL;

    memcpy(&scanme_qr_native_encoder_handlers, &std_object_handlers, sizeof(zend_object_handlers));
    scanme_qr_native_encoder_handlers.free_obj = scanme_qr_native_encoder_free;
    scanme_qr_native_encoder_handlers.offset = XtOffsetOf(scanme_qr_native_encoder_object, std);
}

