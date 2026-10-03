/* fann extension for PHP */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "ext/standard/info.h"
#include "php_fann.h"
#include "fann_arginfo.h"
#include "floatfann.h"
#include <limits.h>
#include <stdint.h>

static int le_fann_ann;
static int le_fann_train_data;
static HashTable fann_error_logs;

/* For compatibility with older PHP versions */
#ifndef ZEND_PARSE_PARAMETERS_NONE
#define ZEND_PARSE_PARAMETERS_NONE() \
	ZEND_PARSE_PARAMETERS_START(0, 0) \
	ZEND_PARSE_PARAMETERS_END()
#endif

static void php_fann_ann_dtor(zend_resource *res)
{
	zval *zfp = zend_hash_index_find(&fann_error_logs, res->handle);
	if (zfp != NULL && Z_TYPE_P(zfp) == IS_LONG) {
		FILE *fp = (FILE *) (uintptr_t) Z_LVAL_P(zfp);
		if (fp != NULL) {
			fclose(fp);
		}
		zend_hash_index_del(&fann_error_logs, res->handle);
	}
	struct fann *ann = (struct fann *) res->ptr;
	if (ann != NULL) {
		fann_destroy(ann);
	}
}

static void php_fann_train_data_dtor(zend_resource *res)
{
	zval *zfp = zend_hash_index_find(&fann_error_logs, res->handle);
	if (zfp != NULL && Z_TYPE_P(zfp) == IS_LONG) {
		FILE *fp = (FILE *) (uintptr_t) Z_LVAL_P(zfp);
		if (fp != NULL) {
			fclose(fp);
		}
		zend_hash_index_del(&fann_error_logs, res->handle);
	}
	struct fann_train_data *train_data = (struct fann_train_data *) res->ptr;
	if (train_data != NULL) {
		fann_destroy_train(train_data);
	}
}

static struct fann *php_fann_fetch_ann(zval *z_ann)
{
	return (struct fann *) zend_fetch_resource_ex(z_ann, "fann neural network", le_fann_ann);
}

static struct fann_train_data *php_fann_fetch_train_data(zval *z_train_data)
{
	return (struct fann_train_data *) zend_fetch_resource_ex(z_train_data, "fann training data", le_fann_train_data);
}

static struct fann_error *php_fann_fetch_error_context(zval *z_resource)
{
	return (struct fann_error *) zend_fetch_resource2_ex(
		z_resource,
		"fann resource",
		le_fann_ann,
		le_fann_train_data
	);
}

static bool php_fann_layers_from_array(zend_array *layers_arr, unsigned int **layers_out, unsigned int *num_layers_out, const char *arg_name)
{
	zend_ulong idx;
	zend_string *key;
	zval *val;
	unsigned int *layers;
	unsigned int pos = 0;
	uint32_t num_layers = zend_hash_num_elements(layers_arr);

	if (num_layers < 2) {
		zend_argument_value_error(1, "%s must contain at least 2 layers", arg_name);
		return false;
	}

	if (num_layers > UINT_MAX) {
		zend_throw_error(NULL, "Layer count is too large");
		return false;
	}

	layers = (unsigned int *) safe_emalloc(num_layers, sizeof(unsigned int), 0);
	ZEND_HASH_FOREACH_KEY_VAL(layers_arr, idx, key, val) {
		zend_long layer_size;

		if (key != NULL) {
			efree(layers);
			zend_argument_type_error(1, "%s must be a list of positive integers", arg_name);
			return false;
		}

		layer_size = zval_get_long(val);
		if (layer_size <= 0 || layer_size > UINT_MAX) {
			efree(layers);
			zend_argument_value_error(1, "%s must contain only positive integers in unsigned int range", arg_name);
			return false;
		}

		layers[pos++] = (unsigned int) layer_size;
	} ZEND_HASH_FOREACH_END();

	*layers_out = layers;
	*num_layers_out = (unsigned int) num_layers;
	return true;
}

static bool php_fann_layers_from_variadic(zval *z_layers, uint32_t layer_count, zend_long num_layers, unsigned int **layers_out, const char *fn_name)
{
	unsigned int i;
	unsigned int *layers;

	if (num_layers < 2 || num_layers > UINT_MAX) {
		zend_argument_value_error(1, "must be between 2 and %u", UINT_MAX);
		return false;
	}

	if ((zend_long) layer_count != num_layers) {
		zend_argument_value_error(1, "must match number of provided layer sizes for %s()", fn_name);
		return false;
	}

	layers = (unsigned int *) safe_emalloc((size_t) num_layers, sizeof(unsigned int), 0);
	for (i = 0; i < (unsigned int) num_layers; i++) {
		zend_long layer_size = zval_get_long(&z_layers[i]);
		if (layer_size <= 0 || layer_size > UINT_MAX) {
			efree(layers);
			zend_argument_value_error(2, "all layer sizes must be positive integers in unsigned int range");
			return false;
		}
		layers[i] = (unsigned int) layer_size;
	}

	*layers_out = layers;
	return true;
}

static zval *php_fann_register_ann(struct fann *ann, zval *return_value)
{
	ZVAL_RES(return_value, zend_register_resource((void *) ann, le_fann_ann));
	return return_value;
}

static zval *php_fann_register_train_data(struct fann_train_data *train_data, zval *return_value)
{
	ZVAL_RES(return_value, zend_register_resource((void *) train_data, le_fann_train_data));
	return return_value;
}

static bool php_fann_values_from_array(zend_array *input_arr, unsigned int expected_count, uint32_t arg_num, const char *arg_name, fann_type **out_values)
{
	fann_type *values;
	zval *val;
	unsigned int i = 0;

	if (zend_hash_num_elements(input_arr) != expected_count) {
		zend_argument_value_error(arg_num, "%s must contain exactly %u values", arg_name, expected_count);
		return false;
	}

	values = (fann_type *) safe_emalloc(expected_count, sizeof(fann_type), 0);
	ZEND_HASH_FOREACH_VAL(input_arr, val) {
		values[i++] = (fann_type) zval_get_double(val);
	} ZEND_HASH_FOREACH_END();

	*out_values = values;
	return true;
}

static bool php_fann_fetch_uint_from_array(zend_array *arr, const char *key, unsigned int *out)
{
	zval *zv = zend_hash_str_find(arr, key, strlen(key));
	zend_long val;
	if (zv == NULL) {
		return false;
	}
	val = zval_get_long(zv);
	if (val < 0 || val > UINT_MAX) {
		return false;
	}
	*out = (unsigned int) val;
	return true;
}

static bool php_fann_int_array_from_zend_array(zend_array *arr, enum fann_activationfunc_enum **out_values, unsigned int *out_count, uint32_t arg_num)
{
	uint32_t count = zend_hash_num_elements(arr);
	enum fann_activationfunc_enum *values;
	zval *zv;
	uint32_t i = 0;
	if (count == 0) {
		zend_argument_value_error(arg_num, "must not be empty");
		return false;
	}
	values = (enum fann_activationfunc_enum *) safe_emalloc(count, sizeof(enum fann_activationfunc_enum), 0);
	ZEND_HASH_FOREACH_VAL(arr, zv) {
		zend_long v = zval_get_long(zv);
		if (v < INT_MIN || v > INT_MAX) {
			efree(values);
			zend_argument_value_error(arg_num, "contains values outside int range");
			return false;
		}
		values[i++] = (enum fann_activationfunc_enum) v;
	} ZEND_HASH_FOREACH_END();
	*out_values = values;
	*out_count = count;
	return true;
}

static bool php_fann_float_array_from_zend_array(zend_array *arr, fann_type **out_values, unsigned int *out_count, uint32_t arg_num)
{
	uint32_t count = zend_hash_num_elements(arr);
	fann_type *values;
	zval *zv;
	uint32_t i = 0;
	if (count == 0) {
		zend_argument_value_error(arg_num, "must not be empty");
		return false;
	}
	values = (fann_type *) safe_emalloc(count, sizeof(fann_type), 0);
	ZEND_HASH_FOREACH_VAL(arr, zv) {
		values[i++] = (fann_type) zval_get_double(zv);
	} ZEND_HASH_FOREACH_END();
	*out_values = values;
	*out_count = count;
	return true;
}

/* {{{ string fann_version() */
PHP_FUNCTION(fann_version)
{
	ZEND_PARSE_PARAMETERS_NONE();
	RETURN_STRING(PHP_FANN_VERSION);
}
/* }}} */

/* {{{ resource|false fann_create_standard(int $num_layers, int ...$layers) */
PHP_FUNCTION(fann_create_standard)
{
	zend_long num_layers;
	zval *z_layers = NULL;
	uint32_t layer_count = 0;
	unsigned int *layers = NULL;
	struct fann *ann;

	ZEND_PARSE_PARAMETERS_START(2, -1)
		Z_PARAM_LONG(num_layers)
		Z_PARAM_VARIADIC('+', z_layers, layer_count)
	ZEND_PARSE_PARAMETERS_END();

	if (!php_fann_layers_from_variadic(z_layers, layer_count, num_layers, &layers, "fann_create_standard")) {
		RETURN_THROWS();
	}

	ann = fann_create_standard_array((unsigned int) num_layers, layers);
	efree(layers);

	if (ann == NULL) {
		RETURN_FALSE;
	}

	php_fann_register_ann(ann, return_value);
}
/* }}} */

/* {{{ resource|false fann_create_standard_array(array $layers) */
PHP_FUNCTION(fann_create_standard_array)
{
	zend_array *layers_arr;
	unsigned int *layers = NULL;
	unsigned int num_layers = 0;
	struct fann *ann;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_ARRAY_HT(layers_arr)
	ZEND_PARSE_PARAMETERS_END();

	if (!php_fann_layers_from_array(layers_arr, &layers, &num_layers, "layers")) {
		RETURN_THROWS();
	}

	ann = fann_create_standard_array(num_layers, layers);
	efree(layers);

	if (ann == NULL) {
		RETURN_FALSE;
	}

	php_fann_register_ann(ann, return_value);
}
/* }}} */

/* {{{ resource|false fann_create_sparse(float $connection_rate, int $num_layers, int ...$layers) */
PHP_FUNCTION(fann_create_sparse)
{
	double connection_rate;
	zend_long num_layers;
	zval *z_layers = NULL;
	uint32_t layer_count = 0;
	unsigned int *layers = NULL;
	struct fann *ann;

	ZEND_PARSE_PARAMETERS_START(3, -1)
		Z_PARAM_DOUBLE(connection_rate)
		Z_PARAM_LONG(num_layers)
		Z_PARAM_VARIADIC('+', z_layers, layer_count)
	ZEND_PARSE_PARAMETERS_END();

	if (connection_rate <= 0.0 || connection_rate > 1.0) {
		zend_argument_value_error(1, "must be > 0 and <= 1");
		RETURN_THROWS();
	}

	if (!php_fann_layers_from_variadic(z_layers, layer_count, num_layers, &layers, "fann_create_sparse")) {
		RETURN_THROWS();
	}

	ann = fann_create_sparse_array((float) connection_rate, (unsigned int) num_layers, layers);
	efree(layers);

	if (ann == NULL) {
		RETURN_FALSE;
	}

	php_fann_register_ann(ann, return_value);
}
/* }}} */

/* {{{ resource|false fann_create_sparse_array(float $connection_rate, array $layers) */
PHP_FUNCTION(fann_create_sparse_array)
{
	double connection_rate;
	zend_array *layers_arr;
	unsigned int *layers = NULL;
	unsigned int num_layers = 0;
	struct fann *ann;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_DOUBLE(connection_rate)
		Z_PARAM_ARRAY_HT(layers_arr)
	ZEND_PARSE_PARAMETERS_END();

	if (connection_rate <= 0.0 || connection_rate > 1.0) {
		zend_argument_value_error(1, "must be > 0 and <= 1");
		RETURN_THROWS();
	}

	if (!php_fann_layers_from_array(layers_arr, &layers, &num_layers, "layers")) {
		RETURN_THROWS();
	}

	ann = fann_create_sparse_array((float) connection_rate, num_layers, layers);
	efree(layers);

	if (ann == NULL) {
		RETURN_FALSE;
	}

	php_fann_register_ann(ann, return_value);
}
/* }}} */

/* {{{ resource|false fann_create_shortcut(int $num_layers, int ...$layers) */
PHP_FUNCTION(fann_create_shortcut)
{
	zend_long num_layers;
	zval *z_layers = NULL;
	uint32_t layer_count = 0;
	unsigned int *layers = NULL;
	struct fann *ann;

	ZEND_PARSE_PARAMETERS_START(2, -1)
		Z_PARAM_LONG(num_layers)
		Z_PARAM_VARIADIC('+', z_layers, layer_count)
	ZEND_PARSE_PARAMETERS_END();

	if (!php_fann_layers_from_variadic(z_layers, layer_count, num_layers, &layers, "fann_create_shortcut")) {
		RETURN_THROWS();
	}

	ann = fann_create_shortcut_array((unsigned int) num_layers, layers);
	efree(layers);

	if (ann == NULL) {
		RETURN_FALSE;
	}

	php_fann_register_ann(ann, return_value);
}
/* }}} */

/* {{{ resource|false fann_create_shortcut_array(array $layers) */
PHP_FUNCTION(fann_create_shortcut_array)
{
	zend_array *layers_arr;
	unsigned int *layers = NULL;
	unsigned int num_layers = 0;
	struct fann *ann;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_ARRAY_HT(layers_arr)
	ZEND_PARSE_PARAMETERS_END();

	if (!php_fann_layers_from_array(layers_arr, &layers, &num_layers, "layers")) {
		RETURN_THROWS();
	}

	ann = fann_create_shortcut_array(num_layers, layers);
	efree(layers);

	if (ann == NULL) {
		RETURN_FALSE;
	}

	php_fann_register_ann(ann, return_value);
}
/* }}} */

/* {{{ resource|false fann_create_from_file(string $configuration_file) */
PHP_FUNCTION(fann_create_from_file)
{
	char *configuration_file;
	size_t configuration_file_len;
	struct fann *ann;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_PATH(configuration_file, configuration_file_len)
	ZEND_PARSE_PARAMETERS_END();

	ann = fann_create_from_file(configuration_file);
	if (ann == NULL) {
		RETURN_FALSE;
	}

	php_fann_register_ann(ann, return_value);
}
/* }}} */

/* {{{ resource|false fann_copy(resource $ann) */
PHP_FUNCTION(fann_copy)
{
	zval *z_ann;
	struct fann *ann;
	struct fann *copy;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_ann)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	copy = fann_copy(ann);
	if (copy == NULL) {
		RETURN_FALSE;
	}

	php_fann_register_ann(copy, return_value);
}
/* }}} */

/* {{{ bool fann_destroy(resource $ann) */
PHP_FUNCTION(fann_destroy)
{
	zval *z_ann;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_ann)
	ZEND_PARSE_PARAMETERS_END();

	if (zend_fetch_resource_ex(z_ann, "fann neural network", le_fann_ann) == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	zend_list_close(Z_RES_P(z_ann));
	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_save(resource $ann, string $configuration_file) */
PHP_FUNCTION(fann_save)
{
	zval *z_ann;
	struct fann *ann;
	char *configuration_file;
	size_t configuration_file_len;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_PATH(configuration_file, configuration_file_len)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	RETURN_BOOL(fann_save(ann, configuration_file) == 0);
}
/* }}} */

/* {{{ int fann_save_to_fixed(resource $ann, string $configuration_file) */
PHP_FUNCTION(fann_save_to_fixed)
{
	zval *z_ann;
	struct fann *ann;
	char *configuration_file;
	size_t configuration_file_len;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_PATH(configuration_file, configuration_file_len)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	RETURN_LONG((zend_long) fann_save_to_fixed(ann, configuration_file));
}
/* }}} */

/* {{{ array|false fann_run(resource $ann, array $input) */
PHP_FUNCTION(fann_run)
{
	zval *z_ann;
	struct fann *ann;
	zend_array *input_arr;
	fann_type *input;
	fann_type *output;
	unsigned int num_input;
	unsigned int num_output;
	uint32_t i = 0;
	zval *val;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_ARRAY_HT(input_arr)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	num_input = fann_get_num_input(ann);
	if (zend_hash_num_elements(input_arr) != num_input) {
		zend_argument_value_error(2, "must contain exactly %u input values", num_input);
		RETURN_THROWS();
	}

	input = (fann_type *) safe_emalloc(num_input, sizeof(fann_type), 0);
	ZEND_HASH_FOREACH_VAL(input_arr, val) {
		input[i++] = (fann_type) zval_get_double(val);
	} ZEND_HASH_FOREACH_END();

	output = fann_run(ann, input);
	efree(input);

	if (output == NULL) {
		RETURN_FALSE;
	}

	num_output = fann_get_num_output(ann);
	array_init_size(return_value, num_output);
	for (i = 0; i < num_output; i++) {
		add_next_index_double(return_value, (double) output[i]);
	}
}
/* }}} */

/* {{{ void fann_print_parameters(resource $ann) */
PHP_FUNCTION(fann_print_parameters)
{
	zval *z_ann;
	struct fann *ann;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_ann)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	fann_print_parameters(ann);
}
/* }}} */

/* {{{ resource|false fann_create_train(int $num_data, int $num_input, int $num_output) */
PHP_FUNCTION(fann_create_train)
{
	zend_long num_data;
	zend_long num_input;
	zend_long num_output;
	struct fann_train_data *train_data;

	ZEND_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_LONG(num_data)
		Z_PARAM_LONG(num_input)
		Z_PARAM_LONG(num_output)
	ZEND_PARSE_PARAMETERS_END();

	if (num_data <= 0 || num_data > UINT_MAX) {
		zend_argument_value_error(1, "must be between 1 and %u", UINT_MAX);
		RETURN_THROWS();
	}
	if (num_input <= 0 || num_input > UINT_MAX) {
		zend_argument_value_error(2, "must be between 1 and %u", UINT_MAX);
		RETURN_THROWS();
	}
	if (num_output <= 0 || num_output > UINT_MAX) {
		zend_argument_value_error(3, "must be between 1 and %u", UINT_MAX);
		RETURN_THROWS();
	}

	train_data = fann_create_train((unsigned int) num_data, (unsigned int) num_input, (unsigned int) num_output);
	if (train_data == NULL) {
		RETURN_FALSE;
	}

	php_fann_register_train_data(train_data, return_value);
}
/* }}} */

/* {{{ resource|false fann_read_train_from_file(string $filename) */
PHP_FUNCTION(fann_read_train_from_file)
{
	char *filename;
	size_t filename_len;
	struct fann_train_data *train_data;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_PATH(filename, filename_len)
	ZEND_PARSE_PARAMETERS_END();

	train_data = fann_read_train_from_file(filename);
	if (train_data == NULL) {
		RETURN_FALSE;
	}

	php_fann_register_train_data(train_data, return_value);
}
/* }}} */

/* {{{ bool fann_save_train(resource $train_data, string $filename) */
PHP_FUNCTION(fann_save_train)
{
	zval *z_train_data;
	struct fann_train_data *train_data;
	char *filename;
	size_t filename_len;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(z_train_data)
		Z_PARAM_PATH(filename, filename_len)
	ZEND_PARSE_PARAMETERS_END();

	train_data = php_fann_fetch_train_data(z_train_data);
	if (train_data == NULL) {
		zend_throw_error(NULL, "Invalid fann training data resource");
		RETURN_THROWS();
	}

	RETURN_BOOL(fann_save_train(train_data, filename) == 0);
}
/* }}} */

/* {{{ int fann_save_train_to_fixed(resource $train_data, string $filename, int $decimal_point) */
PHP_FUNCTION(fann_save_train_to_fixed)
{
	zval *z_train_data;
	struct fann_train_data *train_data;
	char *filename;
	size_t filename_len;
	zend_long decimal_point;

	ZEND_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_RESOURCE(z_train_data)
		Z_PARAM_PATH(filename, filename_len)
		Z_PARAM_LONG(decimal_point)
	ZEND_PARSE_PARAMETERS_END();

	train_data = php_fann_fetch_train_data(z_train_data);
	if (train_data == NULL) {
		zend_throw_error(NULL, "Invalid fann training data resource");
		RETURN_THROWS();
	}

	RETURN_LONG((zend_long) fann_save_train_to_fixed(train_data, filename, (unsigned int) decimal_point));
}
/* }}} */

/* {{{ bool fann_destroy_train(resource $train_data) */
PHP_FUNCTION(fann_destroy_train)
{
	zval *z_train_data;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_train_data)
	ZEND_PARSE_PARAMETERS_END();

	if (zend_fetch_resource_ex(z_train_data, "fann training data", le_fann_train_data) == NULL) {
		zend_throw_error(NULL, "Invalid fann training data resource");
		RETURN_THROWS();
	}

	zend_list_close(Z_RES_P(z_train_data));
	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_train(resource $ann, array $input, array $desired_output) */
PHP_FUNCTION(fann_train)
{
	zval *z_ann;
	struct fann *ann;
	zend_array *input_arr;
	zend_array *desired_output_arr;
	fann_type *input;
	fann_type *desired_output;

	ZEND_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_ARRAY_HT(input_arr)
		Z_PARAM_ARRAY_HT(desired_output_arr)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	if (!php_fann_values_from_array(input_arr, fann_get_num_input(ann), 2, "input", &input)) {
		RETURN_THROWS();
	}
	if (!php_fann_values_from_array(desired_output_arr, fann_get_num_output(ann), 3, "desired_output", &desired_output)) {
		efree(input);
		RETURN_THROWS();
	}

	fann_train(ann, input, desired_output);

	efree(desired_output);
	efree(input);

	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_train_on_data(resource $ann, resource $train_data, int $max_epochs, int $epochs_between_reports, float $desired_error) */
PHP_FUNCTION(fann_train_on_data)
{
	zval *z_ann;
	zval *z_train_data;
	struct fann *ann;
	struct fann_train_data *train_data;
	zend_long max_epochs;
	zend_long epochs_between_reports;
	double desired_error;

	ZEND_PARSE_PARAMETERS_START(5, 5)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_RESOURCE(z_train_data)
		Z_PARAM_LONG(max_epochs)
		Z_PARAM_LONG(epochs_between_reports)
		Z_PARAM_DOUBLE(desired_error)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}
	train_data = php_fann_fetch_train_data(z_train_data);
	if (train_data == NULL) {
		zend_throw_error(NULL, "Invalid fann training data resource");
		RETURN_THROWS();
	}

	if (max_epochs <= 0 || max_epochs > UINT_MAX) {
		zend_argument_value_error(3, "must be between 1 and %u", UINT_MAX);
		RETURN_THROWS();
	}
	if (epochs_between_reports < 0 || epochs_between_reports > UINT_MAX) {
		zend_argument_value_error(4, "must be between 0 and %u", UINT_MAX);
		RETURN_THROWS();
	}
	if (desired_error < 0.0) {
		zend_argument_value_error(5, "must be greater than or equal to 0");
		RETURN_THROWS();
	}

	fann_train_on_data(
		ann,
		train_data,
		(unsigned int) max_epochs,
		(unsigned int) epochs_between_reports,
		(float) desired_error
	);

	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_train_on_file(resource $ann, string $filename, int $max_epochs, int $epochs_between_reports, float $desired_error) */
PHP_FUNCTION(fann_train_on_file)
{
	zval *z_ann;
	struct fann *ann;
	char *filename;
	size_t filename_len;
	zend_long max_epochs;
	zend_long epochs_between_reports;
	double desired_error;

	ZEND_PARSE_PARAMETERS_START(5, 5)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_PATH(filename, filename_len)
		Z_PARAM_LONG(max_epochs)
		Z_PARAM_LONG(epochs_between_reports)
		Z_PARAM_DOUBLE(desired_error)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	if (max_epochs <= 0 || max_epochs > UINT_MAX) {
		zend_argument_value_error(3, "must be between 1 and %u", UINT_MAX);
		RETURN_THROWS();
	}
	if (epochs_between_reports < 0 || epochs_between_reports > UINT_MAX) {
		zend_argument_value_error(4, "must be between 0 and %u", UINT_MAX);
		RETURN_THROWS();
	}
	if (desired_error < 0.0) {
		zend_argument_value_error(5, "must be greater than or equal to 0");
		RETURN_THROWS();
	}

	fann_train_on_file(
		ann,
		filename,
		(unsigned int) max_epochs,
		(unsigned int) epochs_between_reports,
		(float) desired_error
	);

	RETURN_TRUE;
}
/* }}} */

/* {{{ array|false fann_test(resource $ann, array $input, array $desired_output) */
PHP_FUNCTION(fann_test)
{
	zval *z_ann;
	struct fann *ann;
	zend_array *input_arr;
	zend_array *desired_output_arr;
	fann_type *input;
	fann_type *desired_output;
	fann_type *output;
	unsigned int i;
	unsigned int num_output;

	ZEND_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_ARRAY_HT(input_arr)
		Z_PARAM_ARRAY_HT(desired_output_arr)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	if (!php_fann_values_from_array(input_arr, fann_get_num_input(ann), 2, "input", &input)) {
		RETURN_THROWS();
	}
	if (!php_fann_values_from_array(desired_output_arr, fann_get_num_output(ann), 3, "desired_output", &desired_output)) {
		efree(input);
		RETURN_THROWS();
	}

	output = fann_test(ann, input, desired_output);

	efree(desired_output);
	efree(input);

	if (output == NULL) {
		RETURN_FALSE;
	}

	num_output = fann_get_num_output(ann);
	array_init_size(return_value, num_output);
	for (i = 0; i < num_output; i++) {
		add_next_index_double(return_value, (double) output[i]);
	}
}
/* }}} */

/* {{{ float fann_get_MSE(resource $ann) */
PHP_FUNCTION(fann_get_MSE)
{
	zval *z_ann;
	struct fann *ann;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_ann)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	RETURN_DOUBLE((double) fann_get_MSE(ann));
}
/* }}} */

/* {{{ bool fann_reset_MSE(resource $ann) */
PHP_FUNCTION(fann_reset_MSE)
{
	zval *z_ann;
	struct fann *ann;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_ann)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	fann_reset_MSE(ann);
	RETURN_TRUE;
}
/* }}} */

/* {{{ int fann_get_bit_fail(resource $ann) */
PHP_FUNCTION(fann_get_bit_fail)
{
	zval *z_ann;
	struct fann *ann;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_ann)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	RETURN_LONG((zend_long) fann_get_bit_fail(ann));
}
/* }}} */

/* {{{ bool fann_set_training_algorithm(resource $ann, int $training_algorithm) */
PHP_FUNCTION(fann_set_training_algorithm)
{
	zval *z_ann;
	struct fann *ann;
	zend_long training_algorithm;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_LONG(training_algorithm)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	if (training_algorithm < FANN_TRAIN_INCREMENTAL || training_algorithm > FANN_TRAIN_SARPROP) {
		zend_argument_value_error(2, "must be a valid FANN_TRAIN_* constant");
		RETURN_THROWS();
	}

	fann_set_training_algorithm(ann, (enum fann_train_enum) training_algorithm);
	RETURN_TRUE;
}
/* }}} */

/* {{{ int fann_get_training_algorithm(resource $ann) */
PHP_FUNCTION(fann_get_training_algorithm)
{
	zval *z_ann;
	struct fann *ann;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_ann)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	RETURN_LONG((zend_long) fann_get_training_algorithm(ann));
}
/* }}} */

/* {{{ bool fann_set_learning_rate(resource $ann, float $learning_rate) */
PHP_FUNCTION(fann_set_learning_rate)
{
	zval *z_ann;
	struct fann *ann;
	double learning_rate;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_DOUBLE(learning_rate)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	if (learning_rate <= 0.0) {
		zend_argument_value_error(2, "must be greater than 0");
		RETURN_THROWS();
	}

	fann_set_learning_rate(ann, (float) learning_rate);
	RETURN_TRUE;
}
/* }}} */

/* {{{ float fann_get_learning_rate(resource $ann) */
PHP_FUNCTION(fann_get_learning_rate)
{
	zval *z_ann;
	struct fann *ann;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_ann)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	RETURN_DOUBLE((double) fann_get_learning_rate(ann));
}
/* }}} */

/* {{{ bool fann_set_activation_function_hidden(resource $ann, int $activation_function) */
PHP_FUNCTION(fann_set_activation_function_hidden)
{
	zval *z_ann;
	struct fann *ann;
	zend_long activation_function;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_LONG(activation_function)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	if (activation_function < FANN_LINEAR || activation_function > FANN_COS) {
		zend_argument_value_error(2, "must be a valid FANN_* activation constant");
		RETURN_THROWS();
	}

	fann_set_activation_function_hidden(ann, (enum fann_activationfunc_enum) activation_function);
	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_set_activation_function_output(resource $ann, int $activation_function) */
PHP_FUNCTION(fann_set_activation_function_output)
{
	zval *z_ann;
	struct fann *ann;
	zend_long activation_function;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_LONG(activation_function)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	if (activation_function < FANN_LINEAR || activation_function > FANN_COS) {
		zend_argument_value_error(2, "must be a valid FANN_* activation constant");
		RETURN_THROWS();
	}

	fann_set_activation_function_output(ann, (enum fann_activationfunc_enum) activation_function);
	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_set_activation_function(resource $ann, int $activation_function, int $layer, int $neuron) */
PHP_FUNCTION(fann_set_activation_function)
{
	zval *z_ann; struct fann *ann; zend_long activation_function, layer, neuron;
	ZEND_PARSE_PARAMETERS_START(4,4)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_LONG(activation_function)
		Z_PARAM_LONG(layer)
		Z_PARAM_LONG(neuron)
	ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	if (activation_function < FANN_LINEAR || activation_function > FANN_COS) { zend_argument_value_error(2, "must be a valid FANN_* activation constant"); RETURN_THROWS(); }
	fann_set_activation_function(ann, (enum fann_activationfunc_enum) activation_function, (int) layer, (int) neuron);
	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_set_activation_function_layer(resource $ann, int $activation_function, int $layer) */
PHP_FUNCTION(fann_set_activation_function_layer)
{
	zval *z_ann; struct fann *ann; zend_long activation_function, layer;
	ZEND_PARSE_PARAMETERS_START(3,3)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_LONG(activation_function)
		Z_PARAM_LONG(layer)
	ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	if (activation_function < FANN_LINEAR || activation_function > FANN_COS) { zend_argument_value_error(2, "must be a valid FANN_* activation constant"); RETURN_THROWS(); }
	fann_set_activation_function_layer(ann, (enum fann_activationfunc_enum) activation_function, (int) layer);
	RETURN_TRUE;
}
/* }}} */

/* {{{ int fann_get_activation_function(resource $ann, int $layer, int $neuron) */
PHP_FUNCTION(fann_get_activation_function)
{
	zval *z_ann;
	struct fann *ann;
	zend_long layer;
	zend_long neuron;

	ZEND_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_LONG(layer)
		Z_PARAM_LONG(neuron)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	if (layer < 0 || layer > UINT_MAX) {
		zend_argument_value_error(2, "must be between 0 and %u", UINT_MAX);
		RETURN_THROWS();
	}
	if (neuron < 0 || neuron > UINT_MAX) {
		zend_argument_value_error(3, "must be between 0 and %u", UINT_MAX);
		RETURN_THROWS();
	}

	RETURN_LONG((zend_long) fann_get_activation_function(ann, (int) layer, (int) neuron));
}
/* }}} */

/* {{{ float fann_get_activation_steepness(resource $ann, int $layer, int $neuron) */
PHP_FUNCTION(fann_get_activation_steepness)
{
	zval *z_ann; struct fann *ann; zend_long layer, neuron;
	ZEND_PARSE_PARAMETERS_START(3,3)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_LONG(layer)
		Z_PARAM_LONG(neuron)
	ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	RETURN_DOUBLE((double) fann_get_activation_steepness(ann, (int) layer, (int) neuron));
}
/* }}} */

/* {{{ bool fann_set_activation_steepness(resource $ann, float $steepness, int $layer, int $neuron) */
PHP_FUNCTION(fann_set_activation_steepness)
{
	zval *z_ann; struct fann *ann; double steepness; zend_long layer, neuron;
	ZEND_PARSE_PARAMETERS_START(4,4)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_DOUBLE(steepness)
		Z_PARAM_LONG(layer)
		Z_PARAM_LONG(neuron)
	ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	fann_set_activation_steepness(ann, (fann_type) steepness, (int) layer, (int) neuron);
	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_set_activation_steepness_layer(resource $ann, float $steepness, int $layer) */
PHP_FUNCTION(fann_set_activation_steepness_layer)
{
	zval *z_ann; struct fann *ann; double steepness; zend_long layer;
	ZEND_PARSE_PARAMETERS_START(3,3)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_DOUBLE(steepness)
		Z_PARAM_LONG(layer)
	ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	fann_set_activation_steepness_layer(ann, (fann_type) steepness, (int) layer);
	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_set_activation_steepness_hidden(resource $ann, float $steepness) */
PHP_FUNCTION(fann_set_activation_steepness_hidden)
{
	zval *z_ann; struct fann *ann; double steepness;
	ZEND_PARSE_PARAMETERS_START(2,2) Z_PARAM_RESOURCE(z_ann) Z_PARAM_DOUBLE(steepness) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	fann_set_activation_steepness_hidden(ann, (fann_type) steepness);
	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_set_activation_steepness_output(resource $ann, float $steepness) */
PHP_FUNCTION(fann_set_activation_steepness_output)
{
	zval *z_ann; struct fann *ann; double steepness;
	ZEND_PARSE_PARAMETERS_START(2,2) Z_PARAM_RESOURCE(z_ann) Z_PARAM_DOUBLE(steepness) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	fann_set_activation_steepness_output(ann, (fann_type) steepness);
	RETURN_TRUE;
}
/* }}} */

/* {{{ int fann_get_num_input(resource $ann) */
PHP_FUNCTION(fann_get_num_input)
{
	zval *z_ann;
	struct fann *ann;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_ann)
	ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}
	RETURN_LONG((zend_long) fann_get_num_input(ann));
}
/* }}} */

/* {{{ int fann_get_num_output(resource $ann) */
PHP_FUNCTION(fann_get_num_output)
{
	zval *z_ann;
	struct fann *ann;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_ann)
	ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}
	RETURN_LONG((zend_long) fann_get_num_output(ann));
}
/* }}} */

/* {{{ int fann_get_total_neurons(resource $ann) */
PHP_FUNCTION(fann_get_total_neurons)
{
	zval *z_ann;
	struct fann *ann;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_ann)
	ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}
	RETURN_LONG((zend_long) fann_get_total_neurons(ann));
}
/* }}} */

/* {{{ int fann_get_total_connections(resource $ann) */
PHP_FUNCTION(fann_get_total_connections)
{
	zval *z_ann;
	struct fann *ann;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_ann)
	ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}
	RETURN_LONG((zend_long) fann_get_total_connections(ann));
}
/* }}} */

/* {{{ int fann_get_network_type(resource $ann) */
PHP_FUNCTION(fann_get_network_type)
{
	zval *z_ann;
	struct fann *ann;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_ann)
	ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}
	RETURN_LONG((zend_long) fann_get_network_type(ann));
}
/* }}} */

/* {{{ float fann_get_connection_rate(resource $ann) */
PHP_FUNCTION(fann_get_connection_rate)
{
	zval *z_ann;
	struct fann *ann;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_ann)
	ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}
	RETURN_DOUBLE((double) fann_get_connection_rate(ann));
}
/* }}} */

/* {{{ int fann_get_num_layers(resource $ann) */
PHP_FUNCTION(fann_get_num_layers)
{
	zval *z_ann;
	struct fann *ann;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_ann)
	ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}
	RETURN_LONG((zend_long) fann_get_num_layers(ann));
}
/* }}} */

/* {{{ array fann_get_layer_array(resource $ann) */
PHP_FUNCTION(fann_get_layer_array)
{
	zval *z_ann;
	struct fann *ann;
	unsigned int num_layers;
	unsigned int i;
	unsigned int *layers;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_ann)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	num_layers = fann_get_num_layers(ann);
	layers = (unsigned int *) safe_emalloc(num_layers, sizeof(unsigned int), 0);
	fann_get_layer_array(ann, layers);

	array_init_size(return_value, num_layers);
	for (i = 0; i < num_layers; i++) {
		add_next_index_long(return_value, (zend_long) layers[i]);
	}
	efree(layers);
}
/* }}} */

/* {{{ array fann_get_bias_array(resource $ann) */
PHP_FUNCTION(fann_get_bias_array)
{
	zval *z_ann;
	struct fann *ann;
	unsigned int num_layers;
	unsigned int i;
	unsigned int *bias;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_ann)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	num_layers = fann_get_num_layers(ann);
	bias = (unsigned int *) safe_emalloc(num_layers, sizeof(unsigned int), 0);
	fann_get_bias_array(ann, bias);

	array_init_size(return_value, num_layers);
	for (i = 0; i < num_layers; i++) {
		add_next_index_long(return_value, (zend_long) bias[i]);
	}
	efree(bias);
}
/* }}} */

/* {{{ array fann_get_connection_array(resource $ann) */
PHP_FUNCTION(fann_get_connection_array)
{
	zval *z_ann;
	struct fann *ann;
	unsigned int total_connections;
	unsigned int i;
	struct fann_connection *connections;

	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_ann)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	total_connections = fann_get_total_connections(ann);
	connections = (struct fann_connection *) safe_emalloc(total_connections, sizeof(struct fann_connection), 0);
	fann_get_connection_array(ann, connections);

	array_init_size(return_value, total_connections);
	for (i = 0; i < total_connections; i++) {
		zval conn;
		array_init_size(&conn, 3);
		add_assoc_long(&conn, "from_neuron", (zend_long) connections[i].from_neuron);
		add_assoc_long(&conn, "to_neuron", (zend_long) connections[i].to_neuron);
		add_assoc_double(&conn, "weight", (double) connections[i].weight);
		add_next_index_zval(return_value, &conn);
	}
	efree(connections);
}
/* }}} */

/* {{{ bool fann_set_weight_array(resource $ann, array $connections) */
PHP_FUNCTION(fann_set_weight_array)
{
	zval *z_ann;
	struct fann *ann;
	zend_array *connections_arr;
	struct fann_connection *connections;
	zval *z_conn;
	uint32_t i = 0;
	uint32_t count;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_ARRAY_HT(connections_arr)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}

	count = zend_hash_num_elements(connections_arr);
	if (count == 0) {
		RETURN_TRUE;
	}

	connections = (struct fann_connection *) safe_emalloc(count, sizeof(struct fann_connection), 0);

	ZEND_HASH_FOREACH_VAL(connections_arr, z_conn) {
		zend_array *row;
		unsigned int from_neuron;
		unsigned int to_neuron;
		zval *z_weight;
		if (Z_TYPE_P(z_conn) != IS_ARRAY) {
			efree(connections);
			zend_argument_type_error(2, "must contain arrays with keys from_neuron, to_neuron, weight");
			RETURN_THROWS();
		}
		row = Z_ARRVAL_P(z_conn);
		if (!php_fann_fetch_uint_from_array(row, "from_neuron", &from_neuron) ||
			!php_fann_fetch_uint_from_array(row, "to_neuron", &to_neuron)) {
			efree(connections);
			zend_argument_value_error(2, "each connection must provide unsigned int from_neuron and to_neuron");
			RETURN_THROWS();
		}
		z_weight = zend_hash_str_find(row, "weight", sizeof("weight") - 1);
		if (z_weight == NULL) {
			efree(connections);
			zend_argument_value_error(2, "each connection must provide weight");
			RETURN_THROWS();
		}
		connections[i].from_neuron = from_neuron;
		connections[i].to_neuron = to_neuron;
		connections[i].weight = (fann_type) zval_get_double(z_weight);
		i++;
	} ZEND_HASH_FOREACH_END();

	fann_set_weight_array(ann, connections, count);
	efree(connections);
	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_set_weight(resource $ann, int $from_neuron, int $to_neuron, float $weight) */
PHP_FUNCTION(fann_set_weight)
{
	zval *z_ann;
	struct fann *ann;
	zend_long from_neuron;
	zend_long to_neuron;
	double weight;

	ZEND_PARSE_PARAMETERS_START(4, 4)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_LONG(from_neuron)
		Z_PARAM_LONG(to_neuron)
		Z_PARAM_DOUBLE(weight)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}
	if (from_neuron < 0 || from_neuron > UINT_MAX) {
		zend_argument_value_error(2, "must be between 0 and %u", UINT_MAX);
		RETURN_THROWS();
	}
	if (to_neuron < 0 || to_neuron > UINT_MAX) {
		zend_argument_value_error(3, "must be between 0 and %u", UINT_MAX);
		RETURN_THROWS();
	}

	fann_set_weight(ann, (unsigned int) from_neuron, (unsigned int) to_neuron, (fann_type) weight);
	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_randomize_weights(resource $ann, float $min_weight, float $max_weight) */
PHP_FUNCTION(fann_randomize_weights)
{
	zval *z_ann;
	struct fann *ann;
	double min_weight;
	double max_weight;

	ZEND_PARSE_PARAMETERS_START(3, 3)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_DOUBLE(min_weight)
		Z_PARAM_DOUBLE(max_weight)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}
	if (min_weight > max_weight) {
		zend_argument_value_error(2, "must be <= argument #3 (max_weight)");
		RETURN_THROWS();
	}

	fann_randomize_weights(ann, (fann_type) min_weight, (fann_type) max_weight);
	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_init_weights(resource $ann, resource $train_data) */
PHP_FUNCTION(fann_init_weights)
{
	zval *z_ann;
	zval *z_train_data;
	struct fann *ann;
	struct fann_train_data *train_data;

	ZEND_PARSE_PARAMETERS_START(2, 2)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_RESOURCE(z_train_data)
	ZEND_PARSE_PARAMETERS_END();

	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}
	train_data = php_fann_fetch_train_data(z_train_data);
	if (train_data == NULL) {
		zend_throw_error(NULL, "Invalid fann training data resource");
		RETURN_THROWS();
	}

	fann_init_weights(ann, train_data);
	RETURN_TRUE;
}
/* }}} */

/* {{{ void fann_print_connections(resource $ann) */
PHP_FUNCTION(fann_print_connections)
{
	zval *z_ann;
	struct fann *ann;
	ZEND_PARSE_PARAMETERS_START(1, 1)
		Z_PARAM_RESOURCE(z_ann)
	ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) {
		zend_throw_error(NULL, "Invalid fann neural network resource");
		RETURN_THROWS();
	}
	fann_print_connections(ann);
}
/* }}} */

/* {{{ float fann_get_learning_momentum(resource $ann) */
PHP_FUNCTION(fann_get_learning_momentum)
{
	zval *z_ann;
	struct fann *ann;
	ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_RESOURCE(z_ann) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	RETURN_DOUBLE((double) fann_get_learning_momentum(ann));
}
/* }}} */

/* {{{ bool fann_set_learning_momentum(resource $ann, float $learning_momentum) */
PHP_FUNCTION(fann_set_learning_momentum)
{
	zval *z_ann; struct fann *ann; double v;
	ZEND_PARSE_PARAMETERS_START(2, 2) Z_PARAM_RESOURCE(z_ann) Z_PARAM_DOUBLE(v) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	fann_set_learning_momentum(ann, (float) v);
	RETURN_TRUE;
}
/* }}} */

/* {{{ int fann_get_train_error_function(resource $ann) */
PHP_FUNCTION(fann_get_train_error_function)
{
	zval *z_ann; struct fann *ann;
	ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_RESOURCE(z_ann) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	RETURN_LONG((zend_long) fann_get_train_error_function(ann));
}
/* }}} */

/* {{{ bool fann_set_train_error_function(resource $ann, int $train_error_function) */
PHP_FUNCTION(fann_set_train_error_function)
{
	zval *z_ann; struct fann *ann; zend_long v;
	ZEND_PARSE_PARAMETERS_START(2, 2) Z_PARAM_RESOURCE(z_ann) Z_PARAM_LONG(v) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	if (v < FANN_ERRORFUNC_LINEAR || v > FANN_ERRORFUNC_TANH) {
		zend_argument_value_error(2, "must be a valid FANN_ERRORFUNC_* constant");
		RETURN_THROWS();
	}
	fann_set_train_error_function(ann, (enum fann_errorfunc_enum) v);
	RETURN_TRUE;
}
/* }}} */

/* {{{ int fann_get_train_stop_function(resource $ann) */
PHP_FUNCTION(fann_get_train_stop_function)
{
	zval *z_ann; struct fann *ann;
	ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_RESOURCE(z_ann) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	RETURN_LONG((zend_long) fann_get_train_stop_function(ann));
}
/* }}} */

/* {{{ bool fann_set_train_stop_function(resource $ann, int $train_stop_function) */
PHP_FUNCTION(fann_set_train_stop_function)
{
	zval *z_ann; struct fann *ann; zend_long v;
	ZEND_PARSE_PARAMETERS_START(2, 2) Z_PARAM_RESOURCE(z_ann) Z_PARAM_LONG(v) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	if (v < FANN_STOPFUNC_MSE || v > FANN_STOPFUNC_BIT) {
		zend_argument_value_error(2, "must be a valid FANN_STOPFUNC_* constant");
		RETURN_THROWS();
	}
	fann_set_train_stop_function(ann, (enum fann_stopfunc_enum) v);
	RETURN_TRUE;
}
/* }}} */

/* {{{ float fann_get_bit_fail_limit(resource $ann) */
PHP_FUNCTION(fann_get_bit_fail_limit)
{
	zval *z_ann; struct fann *ann;
	ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_RESOURCE(z_ann) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	RETURN_DOUBLE((double) fann_get_bit_fail_limit(ann));
}
/* }}} */

/* {{{ bool fann_set_bit_fail_limit(resource $ann, float $bit_fail_limit) */
PHP_FUNCTION(fann_set_bit_fail_limit)
{
	zval *z_ann; struct fann *ann; double v;
	ZEND_PARSE_PARAMETERS_START(2, 2) Z_PARAM_RESOURCE(z_ann) Z_PARAM_DOUBLE(v) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	fann_set_bit_fail_limit(ann, (fann_type) v);
	RETURN_TRUE;
}
/* }}} */

#define FANN_WRAP_GET_FLOAT(fn_name, c_fn) \
PHP_FUNCTION(fn_name) { \
	zval *z_ann; struct fann *ann; \
	ZEND_PARSE_PARAMETERS_START(1,1) Z_PARAM_RESOURCE(z_ann) ZEND_PARSE_PARAMETERS_END(); \
	ann = php_fann_fetch_ann(z_ann); \
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); } \
	RETURN_DOUBLE((double) c_fn(ann)); \
}

#define FANN_WRAP_SET_FLOAT(fn_name, c_fn) \
PHP_FUNCTION(fn_name) { \
	zval *z_ann; struct fann *ann; double v; \
	ZEND_PARSE_PARAMETERS_START(2,2) Z_PARAM_RESOURCE(z_ann) Z_PARAM_DOUBLE(v) ZEND_PARSE_PARAMETERS_END(); \
	ann = php_fann_fetch_ann(z_ann); \
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); } \
	c_fn(ann, (float) v); \
	RETURN_TRUE; \
}

/* {{{ quickprop/rprop/sarprop parameter wrappers */
FANN_WRAP_GET_FLOAT(fann_get_quickprop_decay, fann_get_quickprop_decay)
FANN_WRAP_SET_FLOAT(fann_set_quickprop_decay, fann_set_quickprop_decay)
FANN_WRAP_GET_FLOAT(fann_get_quickprop_mu, fann_get_quickprop_mu)
FANN_WRAP_SET_FLOAT(fann_set_quickprop_mu, fann_set_quickprop_mu)
FANN_WRAP_GET_FLOAT(fann_get_rprop_increase_factor, fann_get_rprop_increase_factor)
FANN_WRAP_SET_FLOAT(fann_set_rprop_increase_factor, fann_set_rprop_increase_factor)
FANN_WRAP_GET_FLOAT(fann_get_rprop_decrease_factor, fann_get_rprop_decrease_factor)
FANN_WRAP_SET_FLOAT(fann_set_rprop_decrease_factor, fann_set_rprop_decrease_factor)
FANN_WRAP_GET_FLOAT(fann_get_rprop_delta_min, fann_get_rprop_delta_min)
FANN_WRAP_SET_FLOAT(fann_set_rprop_delta_min, fann_set_rprop_delta_min)
FANN_WRAP_GET_FLOAT(fann_get_rprop_delta_max, fann_get_rprop_delta_max)
FANN_WRAP_SET_FLOAT(fann_set_rprop_delta_max, fann_set_rprop_delta_max)
FANN_WRAP_GET_FLOAT(fann_get_rprop_delta_zero, fann_get_rprop_delta_zero)
FANN_WRAP_SET_FLOAT(fann_set_rprop_delta_zero, fann_set_rprop_delta_zero)
FANN_WRAP_GET_FLOAT(fann_get_sarprop_weight_decay_shift, fann_get_sarprop_weight_decay_shift)
FANN_WRAP_SET_FLOAT(fann_set_sarprop_weight_decay_shift, fann_set_sarprop_weight_decay_shift)
FANN_WRAP_GET_FLOAT(fann_get_sarprop_step_error_threshold_factor, fann_get_sarprop_step_error_threshold_factor)
FANN_WRAP_SET_FLOAT(fann_set_sarprop_step_error_threshold_factor, fann_set_sarprop_step_error_threshold_factor)
FANN_WRAP_GET_FLOAT(fann_get_sarprop_step_error_shift, fann_get_sarprop_step_error_shift)
FANN_WRAP_SET_FLOAT(fann_set_sarprop_step_error_shift, fann_set_sarprop_step_error_shift)
FANN_WRAP_GET_FLOAT(fann_get_sarprop_temperature, fann_get_sarprop_temperature)
FANN_WRAP_SET_FLOAT(fann_set_sarprop_temperature, fann_set_sarprop_temperature)
/* }}} */

/* {{{ bool fann_shuffle_train_data(resource $train_data) */
PHP_FUNCTION(fann_shuffle_train_data)
{
	zval *z_train_data;
	struct fann_train_data *train_data;
	ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_RESOURCE(z_train_data) ZEND_PARSE_PARAMETERS_END();
	train_data = php_fann_fetch_train_data(z_train_data);
	if (train_data == NULL) { zend_throw_error(NULL, "Invalid fann training data resource"); RETURN_THROWS(); }
	fann_shuffle_train_data(train_data);
	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_scale_input_train_data(resource $train_data, float $new_min, float $new_max) */
PHP_FUNCTION(fann_scale_input_train_data)
{
	zval *z_train_data; struct fann_train_data *train_data; double new_min, new_max;
	ZEND_PARSE_PARAMETERS_START(3, 3) Z_PARAM_RESOURCE(z_train_data) Z_PARAM_DOUBLE(new_min) Z_PARAM_DOUBLE(new_max) ZEND_PARSE_PARAMETERS_END();
	train_data = php_fann_fetch_train_data(z_train_data);
	if (train_data == NULL) { zend_throw_error(NULL, "Invalid fann training data resource"); RETURN_THROWS(); }
	fann_scale_input_train_data(train_data, (fann_type)new_min, (fann_type)new_max);
	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_scale_output_train_data(resource $train_data, float $new_min, float $new_max) */
PHP_FUNCTION(fann_scale_output_train_data)
{
	zval *z_train_data; struct fann_train_data *train_data; double new_min, new_max;
	ZEND_PARSE_PARAMETERS_START(3, 3) Z_PARAM_RESOURCE(z_train_data) Z_PARAM_DOUBLE(new_min) Z_PARAM_DOUBLE(new_max) ZEND_PARSE_PARAMETERS_END();
	train_data = php_fann_fetch_train_data(z_train_data);
	if (train_data == NULL) { zend_throw_error(NULL, "Invalid fann training data resource"); RETURN_THROWS(); }
	fann_scale_output_train_data(train_data, (fann_type)new_min, (fann_type)new_max);
	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_scale_train_data(resource $train_data, float $new_min, float $new_max) */
PHP_FUNCTION(fann_scale_train_data)
{
	zval *z_train_data; struct fann_train_data *train_data; double new_min, new_max;
	ZEND_PARSE_PARAMETERS_START(3, 3) Z_PARAM_RESOURCE(z_train_data) Z_PARAM_DOUBLE(new_min) Z_PARAM_DOUBLE(new_max) ZEND_PARSE_PARAMETERS_END();
	train_data = php_fann_fetch_train_data(z_train_data);
	if (train_data == NULL) { zend_throw_error(NULL, "Invalid fann training data resource"); RETURN_THROWS(); }
	fann_scale_train_data(train_data, (fann_type)new_min, (fann_type)new_max);
	RETURN_TRUE;
}
/* }}} */

/* {{{ resource|false fann_merge_train_data(resource $data1, resource $data2) */
PHP_FUNCTION(fann_merge_train_data)
{
	zval *z_data1, *z_data2;
	struct fann_train_data *data1, *data2, *merged;
	ZEND_PARSE_PARAMETERS_START(2, 2) Z_PARAM_RESOURCE(z_data1) Z_PARAM_RESOURCE(z_data2) ZEND_PARSE_PARAMETERS_END();
	data1 = php_fann_fetch_train_data(z_data1);
	data2 = php_fann_fetch_train_data(z_data2);
	if (data1 == NULL || data2 == NULL) { zend_throw_error(NULL, "Invalid fann training data resource"); RETURN_THROWS(); }
	merged = fann_merge_train_data(data1, data2);
	if (merged == NULL) { RETURN_FALSE; }
	php_fann_register_train_data(merged, return_value);
}
/* }}} */

/* {{{ resource|false fann_duplicate_train_data(resource $train_data) */
PHP_FUNCTION(fann_duplicate_train_data)
{
	zval *z_train_data; struct fann_train_data *train_data, *dup;
	ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_RESOURCE(z_train_data) ZEND_PARSE_PARAMETERS_END();
	train_data = php_fann_fetch_train_data(z_train_data);
	if (train_data == NULL) { zend_throw_error(NULL, "Invalid fann training data resource"); RETURN_THROWS(); }
	dup = fann_duplicate_train_data(train_data);
	if (dup == NULL) { RETURN_FALSE; }
	php_fann_register_train_data(dup, return_value);
}
/* }}} */

/* {{{ resource|false fann_subset_train_data(resource $train_data, int $pos, int $length) */
PHP_FUNCTION(fann_subset_train_data)
{
	zval *z_train_data; struct fann_train_data *train_data, *subset; zend_long pos, length;
	ZEND_PARSE_PARAMETERS_START(3, 3) Z_PARAM_RESOURCE(z_train_data) Z_PARAM_LONG(pos) Z_PARAM_LONG(length) ZEND_PARSE_PARAMETERS_END();
	train_data = php_fann_fetch_train_data(z_train_data);
	if (train_data == NULL) { zend_throw_error(NULL, "Invalid fann training data resource"); RETURN_THROWS(); }
	if (pos < 0 || pos > UINT_MAX) { zend_argument_value_error(2, "must be between 0 and %u", UINT_MAX); RETURN_THROWS(); }
	if (length < 0 || length > UINT_MAX) { zend_argument_value_error(3, "must be between 0 and %u", UINT_MAX); RETURN_THROWS(); }
	subset = fann_subset_train_data(train_data, (unsigned int) pos, (unsigned int) length);
	if (subset == NULL) { RETURN_FALSE; }
	php_fann_register_train_data(subset, return_value);
}
/* }}} */

/* {{{ int fann_length_train_data(resource $train_data) */
PHP_FUNCTION(fann_length_train_data)
{
	zval *z_train_data; struct fann_train_data *train_data;
	ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_RESOURCE(z_train_data) ZEND_PARSE_PARAMETERS_END();
	train_data = php_fann_fetch_train_data(z_train_data);
	if (train_data == NULL) { zend_throw_error(NULL, "Invalid fann training data resource"); RETURN_THROWS(); }
	RETURN_LONG((zend_long) fann_length_train_data(train_data));
}
/* }}} */

/* {{{ int fann_num_input_train_data(resource $train_data) */
PHP_FUNCTION(fann_num_input_train_data)
{
	zval *z_train_data; struct fann_train_data *train_data;
	ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_RESOURCE(z_train_data) ZEND_PARSE_PARAMETERS_END();
	train_data = php_fann_fetch_train_data(z_train_data);
	if (train_data == NULL) { zend_throw_error(NULL, "Invalid fann training data resource"); RETURN_THROWS(); }
	RETURN_LONG((zend_long) fann_num_input_train_data(train_data));
}
/* }}} */

/* {{{ int fann_num_output_train_data(resource $train_data) */
PHP_FUNCTION(fann_num_output_train_data)
{
	zval *z_train_data; struct fann_train_data *train_data;
	ZEND_PARSE_PARAMETERS_START(1, 1) Z_PARAM_RESOURCE(z_train_data) ZEND_PARSE_PARAMETERS_END();
	train_data = php_fann_fetch_train_data(z_train_data);
	if (train_data == NULL) { zend_throw_error(NULL, "Invalid fann training data resource"); RETURN_THROWS(); }
	RETURN_LONG((zend_long) fann_num_output_train_data(train_data));
}
/* }}} */

/* {{{ float fann_train_epoch(resource $ann, resource $train_data) */
PHP_FUNCTION(fann_train_epoch)
{
	zval *z_ann, *z_train_data; struct fann *ann; struct fann_train_data *train_data;
	ZEND_PARSE_PARAMETERS_START(2, 2) Z_PARAM_RESOURCE(z_ann) Z_PARAM_RESOURCE(z_train_data) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	train_data = php_fann_fetch_train_data(z_train_data);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	if (train_data == NULL) { zend_throw_error(NULL, "Invalid fann training data resource"); RETURN_THROWS(); }
	RETURN_DOUBLE((double) fann_train_epoch(ann, train_data));
}
/* }}} */

/* {{{ float fann_test_data(resource $ann, resource $train_data) */
PHP_FUNCTION(fann_test_data)
{
	zval *z_ann, *z_train_data; struct fann *ann; struct fann_train_data *train_data;
	ZEND_PARSE_PARAMETERS_START(2, 2) Z_PARAM_RESOURCE(z_ann) Z_PARAM_RESOURCE(z_train_data) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	train_data = php_fann_fetch_train_data(z_train_data);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	if (train_data == NULL) { zend_throw_error(NULL, "Invalid fann training data resource"); RETURN_THROWS(); }
	RETURN_DOUBLE((double) fann_test_data(ann, train_data));
}
/* }}} */

/* {{{ bool fann_scale_train(resource $ann, resource $train_data) */
PHP_FUNCTION(fann_scale_train)
{
	zval *z_ann, *z_train_data; struct fann *ann; struct fann_train_data *train_data;
	ZEND_PARSE_PARAMETERS_START(2, 2) Z_PARAM_RESOURCE(z_ann) Z_PARAM_RESOURCE(z_train_data) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann); train_data = php_fann_fetch_train_data(z_train_data);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	if (train_data == NULL) { zend_throw_error(NULL, "Invalid fann training data resource"); RETURN_THROWS(); }
	fann_scale_train(ann, train_data);
	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_descale_train(resource $ann, resource $train_data) */
PHP_FUNCTION(fann_descale_train)
{
	zval *z_ann, *z_train_data; struct fann *ann; struct fann_train_data *train_data;
	ZEND_PARSE_PARAMETERS_START(2, 2) Z_PARAM_RESOURCE(z_ann) Z_PARAM_RESOURCE(z_train_data) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann); train_data = php_fann_fetch_train_data(z_train_data);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	if (train_data == NULL) { zend_throw_error(NULL, "Invalid fann training data resource"); RETURN_THROWS(); }
	fann_descale_train(ann, train_data);
	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_set_input_scaling_params(resource $ann, resource $train_data, float $new_input_min, float $new_input_max) */
PHP_FUNCTION(fann_set_input_scaling_params)
{
	zval *z_ann, *z_train_data; struct fann *ann; struct fann_train_data *train_data; double a,b;
	ZEND_PARSE_PARAMETERS_START(4,4) Z_PARAM_RESOURCE(z_ann) Z_PARAM_RESOURCE(z_train_data) Z_PARAM_DOUBLE(a) Z_PARAM_DOUBLE(b) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann); train_data = php_fann_fetch_train_data(z_train_data);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	if (train_data == NULL) { zend_throw_error(NULL, "Invalid fann training data resource"); RETURN_THROWS(); }
	RETURN_BOOL(fann_set_input_scaling_params(ann, train_data, (float)a, (float)b) == 0);
}
/* }}} */

/* {{{ bool fann_set_output_scaling_params(resource $ann, resource $train_data, float $new_output_min, float $new_output_max) */
PHP_FUNCTION(fann_set_output_scaling_params)
{
	zval *z_ann, *z_train_data; struct fann *ann; struct fann_train_data *train_data; double a,b;
	ZEND_PARSE_PARAMETERS_START(4,4) Z_PARAM_RESOURCE(z_ann) Z_PARAM_RESOURCE(z_train_data) Z_PARAM_DOUBLE(a) Z_PARAM_DOUBLE(b) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann); train_data = php_fann_fetch_train_data(z_train_data);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	if (train_data == NULL) { zend_throw_error(NULL, "Invalid fann training data resource"); RETURN_THROWS(); }
	RETURN_BOOL(fann_set_output_scaling_params(ann, train_data, (float)a, (float)b) == 0);
}
/* }}} */

/* {{{ bool fann_set_scaling_params(resource $ann, resource $train_data, float $new_input_min, float $new_input_max, float $new_output_min, float $new_output_max) */
PHP_FUNCTION(fann_set_scaling_params)
{
	zval *z_ann, *z_train_data; struct fann *ann; struct fann_train_data *train_data; double a,b,c,d;
	ZEND_PARSE_PARAMETERS_START(6,6) Z_PARAM_RESOURCE(z_ann) Z_PARAM_RESOURCE(z_train_data) Z_PARAM_DOUBLE(a) Z_PARAM_DOUBLE(b) Z_PARAM_DOUBLE(c) Z_PARAM_DOUBLE(d) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann); train_data = php_fann_fetch_train_data(z_train_data);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	if (train_data == NULL) { zend_throw_error(NULL, "Invalid fann training data resource"); RETURN_THROWS(); }
	RETURN_BOOL(fann_set_scaling_params(ann, train_data, (float)a, (float)b, (float)c, (float)d) == 0);
}
/* }}} */

/* {{{ bool fann_clear_scaling_params(resource $ann) */
PHP_FUNCTION(fann_clear_scaling_params)
{
	zval *z_ann; struct fann *ann;
	ZEND_PARSE_PARAMETERS_START(1,1) Z_PARAM_RESOURCE(z_ann) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	RETURN_BOOL(fann_clear_scaling_params(ann) == 0);
}
/* }}} */

/* {{{ array fann_scale_input(resource $ann, array $input_vector) */
PHP_FUNCTION(fann_scale_input)
{
	zval *z_ann; struct fann *ann; zend_array *arr; fann_type *input; unsigned int n,i;
	ZEND_PARSE_PARAMETERS_START(2,2) Z_PARAM_RESOURCE(z_ann) Z_PARAM_ARRAY_HT(arr) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	n = fann_get_num_input(ann);
	if (!php_fann_values_from_array(arr, n, 2, "input_vector", &input)) { RETURN_THROWS(); }
	fann_scale_input(ann, input);
	array_init_size(return_value, n);
	for (i=0;i<n;i++) add_next_index_double(return_value, (double)input[i]);
	efree(input);
}
/* }}} */

/* {{{ array fann_scale_output(resource $ann, array $output_vector) */
PHP_FUNCTION(fann_scale_output)
{
	zval *z_ann; struct fann *ann; zend_array *arr; fann_type *output; unsigned int n,i;
	ZEND_PARSE_PARAMETERS_START(2,2) Z_PARAM_RESOURCE(z_ann) Z_PARAM_ARRAY_HT(arr) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	n = fann_get_num_output(ann);
	if (!php_fann_values_from_array(arr, n, 2, "output_vector", &output)) { RETURN_THROWS(); }
	fann_scale_output(ann, output);
	array_init_size(return_value, n);
	for (i=0;i<n;i++) add_next_index_double(return_value, (double)output[i]);
	efree(output);
}
/* }}} */

/* {{{ array fann_descale_input(resource $ann, array $input_vector) */
PHP_FUNCTION(fann_descale_input)
{
	zval *z_ann; struct fann *ann; zend_array *arr; fann_type *input; unsigned int n,i;
	ZEND_PARSE_PARAMETERS_START(2,2) Z_PARAM_RESOURCE(z_ann) Z_PARAM_ARRAY_HT(arr) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	n = fann_get_num_input(ann);
	if (!php_fann_values_from_array(arr, n, 2, "input_vector", &input)) { RETURN_THROWS(); }
	fann_descale_input(ann, input);
	array_init_size(return_value, n);
	for (i=0;i<n;i++) add_next_index_double(return_value, (double)input[i]);
	efree(input);
}
/* }}} */

/* {{{ array fann_descale_output(resource $ann, array $output_vector) */
PHP_FUNCTION(fann_descale_output)
{
	zval *z_ann; struct fann *ann; zend_array *arr; fann_type *output; unsigned int n,i;
	ZEND_PARSE_PARAMETERS_START(2,2) Z_PARAM_RESOURCE(z_ann) Z_PARAM_ARRAY_HT(arr) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	n = fann_get_num_output(ann);
	if (!php_fann_values_from_array(arr, n, 2, "output_vector", &output)) { RETURN_THROWS(); }
	fann_descale_output(ann, output);
	array_init_size(return_value, n);
	for (i=0;i<n;i++) add_next_index_double(return_value, (double)output[i]);
	efree(output);
}
/* }}} */

/* {{{ bool fann_cascadetrain_on_data(resource $ann, resource $train_data, int $max_neurons, int $neurons_between_reports, float $desired_error) */
PHP_FUNCTION(fann_cascadetrain_on_data)
{
	zval *z_ann, *z_train_data;
	struct fann *ann;
	struct fann_train_data *train_data;
	zend_long max_neurons, neurons_between_reports;
	double desired_error;

	ZEND_PARSE_PARAMETERS_START(5,5)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_RESOURCE(z_train_data)
		Z_PARAM_LONG(max_neurons)
		Z_PARAM_LONG(neurons_between_reports)
		Z_PARAM_DOUBLE(desired_error)
	ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	train_data = php_fann_fetch_train_data(z_train_data);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	if (train_data == NULL) { zend_throw_error(NULL, "Invalid fann training data resource"); RETURN_THROWS(); }
	if (max_neurons <= 0 || max_neurons > UINT_MAX) { zend_argument_value_error(3, "must be between 1 and %u", UINT_MAX); RETURN_THROWS(); }
	if (neurons_between_reports < 0 || neurons_between_reports > UINT_MAX) { zend_argument_value_error(4, "must be between 0 and %u", UINT_MAX); RETURN_THROWS(); }
	if (desired_error < 0.0) { zend_argument_value_error(5, "must be >= 0"); RETURN_THROWS(); }

	fann_cascadetrain_on_data(ann, train_data, (unsigned int)max_neurons, (unsigned int)neurons_between_reports, (float)desired_error);
	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_cascadetrain_on_file(resource $ann, string $filename, int $max_neurons, int $neurons_between_reports, float $desired_error) */
PHP_FUNCTION(fann_cascadetrain_on_file)
{
	zval *z_ann;
	struct fann *ann;
	char *filename; size_t filename_len;
	zend_long max_neurons, neurons_between_reports; double desired_error;
	ZEND_PARSE_PARAMETERS_START(5,5)
		Z_PARAM_RESOURCE(z_ann)
		Z_PARAM_PATH(filename, filename_len)
		Z_PARAM_LONG(max_neurons)
		Z_PARAM_LONG(neurons_between_reports)
		Z_PARAM_DOUBLE(desired_error)
	ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	if (max_neurons <= 0 || max_neurons > UINT_MAX) { zend_argument_value_error(3, "must be between 1 and %u", UINT_MAX); RETURN_THROWS(); }
	if (neurons_between_reports < 0 || neurons_between_reports > UINT_MAX) { zend_argument_value_error(4, "must be between 0 and %u", UINT_MAX); RETURN_THROWS(); }
	if (desired_error < 0.0) { zend_argument_value_error(5, "must be >= 0"); RETURN_THROWS(); }
	fann_cascadetrain_on_file(ann, filename, (unsigned int)max_neurons, (unsigned int)neurons_between_reports, (float)desired_error);
	RETURN_TRUE;
}
/* }}} */

#define FANN_WRAP_GET_UINT(fn_name, c_fn) \
PHP_FUNCTION(fn_name) { zval *z_ann; struct fann *ann; \
	ZEND_PARSE_PARAMETERS_START(1,1) Z_PARAM_RESOURCE(z_ann) ZEND_PARSE_PARAMETERS_END(); \
	ann = php_fann_fetch_ann(z_ann); \
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); } \
	RETURN_LONG((zend_long) c_fn(ann)); \
}

#define FANN_WRAP_SET_UINT(fn_name, c_fn) \
PHP_FUNCTION(fn_name) { zval *z_ann; struct fann *ann; zend_long v; \
	ZEND_PARSE_PARAMETERS_START(2,2) Z_PARAM_RESOURCE(z_ann) Z_PARAM_LONG(v) ZEND_PARSE_PARAMETERS_END(); \
	ann = php_fann_fetch_ann(z_ann); \
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); } \
	if (v < 0 || v > UINT_MAX) { zend_argument_value_error(2, "must be between 0 and %u", UINT_MAX); RETURN_THROWS(); } \
	c_fn(ann, (unsigned int) v); \
	RETURN_TRUE; \
}

/* {{{ cascade scalar parameter wrappers */
FANN_WRAP_GET_FLOAT(fann_get_cascade_output_change_fraction, fann_get_cascade_output_change_fraction)
FANN_WRAP_SET_FLOAT(fann_set_cascade_output_change_fraction, fann_set_cascade_output_change_fraction)
FANN_WRAP_GET_UINT(fann_get_cascade_output_stagnation_epochs, fann_get_cascade_output_stagnation_epochs)
FANN_WRAP_SET_UINT(fann_set_cascade_output_stagnation_epochs, fann_set_cascade_output_stagnation_epochs)
FANN_WRAP_GET_FLOAT(fann_get_cascade_candidate_change_fraction, fann_get_cascade_candidate_change_fraction)
FANN_WRAP_SET_FLOAT(fann_set_cascade_candidate_change_fraction, fann_set_cascade_candidate_change_fraction)
FANN_WRAP_GET_UINT(fann_get_cascade_candidate_stagnation_epochs, fann_get_cascade_candidate_stagnation_epochs)
FANN_WRAP_SET_UINT(fann_set_cascade_candidate_stagnation_epochs, fann_set_cascade_candidate_stagnation_epochs)
FANN_WRAP_GET_FLOAT(fann_get_cascade_weight_multiplier, fann_get_cascade_weight_multiplier)
FANN_WRAP_SET_FLOAT(fann_set_cascade_weight_multiplier, fann_set_cascade_weight_multiplier)
FANN_WRAP_GET_FLOAT(fann_get_cascade_candidate_limit, fann_get_cascade_candidate_limit)
FANN_WRAP_SET_FLOAT(fann_set_cascade_candidate_limit, fann_set_cascade_candidate_limit)
FANN_WRAP_GET_UINT(fann_get_cascade_max_out_epochs, fann_get_cascade_max_out_epochs)
FANN_WRAP_SET_UINT(fann_set_cascade_max_out_epochs, fann_set_cascade_max_out_epochs)
FANN_WRAP_GET_UINT(fann_get_cascade_min_out_epochs, fann_get_cascade_min_out_epochs)
FANN_WRAP_SET_UINT(fann_set_cascade_min_out_epochs, fann_set_cascade_min_out_epochs)
FANN_WRAP_GET_UINT(fann_get_cascade_max_cand_epochs, fann_get_cascade_max_cand_epochs)
FANN_WRAP_SET_UINT(fann_set_cascade_max_cand_epochs, fann_set_cascade_max_cand_epochs)
FANN_WRAP_GET_UINT(fann_get_cascade_min_cand_epochs, fann_get_cascade_min_cand_epochs)
FANN_WRAP_SET_UINT(fann_set_cascade_min_cand_epochs, fann_set_cascade_min_cand_epochs)
FANN_WRAP_GET_UINT(fann_get_cascade_num_candidates, fann_get_cascade_num_candidates)
FANN_WRAP_GET_UINT(fann_get_cascade_activation_functions_count, fann_get_cascade_activation_functions_count)
FANN_WRAP_GET_UINT(fann_get_cascade_activation_steepnesses_count, fann_get_cascade_activation_steepnesses_count)
FANN_WRAP_GET_UINT(fann_get_cascade_num_candidate_groups, fann_get_cascade_num_candidate_groups)
FANN_WRAP_SET_UINT(fann_set_cascade_num_candidate_groups, fann_set_cascade_num_candidate_groups)
/* }}} */

/* {{{ array fann_get_cascade_activation_functions(resource $ann) */
PHP_FUNCTION(fann_get_cascade_activation_functions)
{
	zval *z_ann; struct fann *ann; enum fann_activationfunc_enum *arr; unsigned int n, i;
	ZEND_PARSE_PARAMETERS_START(1,1) Z_PARAM_RESOURCE(z_ann) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	n = fann_get_cascade_activation_functions_count(ann);
	arr = fann_get_cascade_activation_functions(ann);
	array_init_size(return_value, n);
	for (i=0; i<n; i++) add_next_index_long(return_value, (zend_long) arr[i]);
}
/* }}} */

/* {{{ bool fann_set_cascade_activation_functions(resource $ann, array $functions) */
PHP_FUNCTION(fann_set_cascade_activation_functions)
{
	zval *z_ann; struct fann *ann; zend_array *zfuncs; enum fann_activationfunc_enum *funcs; unsigned int count;
	ZEND_PARSE_PARAMETERS_START(2,2) Z_PARAM_RESOURCE(z_ann) Z_PARAM_ARRAY_HT(zfuncs) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	if (!php_fann_int_array_from_zend_array(zfuncs, &funcs, &count, 2)) { RETURN_THROWS(); }
	fann_set_cascade_activation_functions(ann, funcs, count);
	efree(funcs);
	RETURN_TRUE;
}
/* }}} */

/* {{{ array fann_get_cascade_activation_steepnesses(resource $ann) */
PHP_FUNCTION(fann_get_cascade_activation_steepnesses)
{
	zval *z_ann; struct fann *ann; fann_type *arr; unsigned int n, i;
	ZEND_PARSE_PARAMETERS_START(1,1) Z_PARAM_RESOURCE(z_ann) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	n = fann_get_cascade_activation_steepnesses_count(ann);
	arr = fann_get_cascade_activation_steepnesses(ann);
	array_init_size(return_value, n);
	for (i=0; i<n; i++) add_next_index_double(return_value, (double) arr[i]);
}
/* }}} */

/* {{{ bool fann_set_cascade_activation_steepnesses(resource $ann, array $steepnesses) */
PHP_FUNCTION(fann_set_cascade_activation_steepnesses)
{
	zval *z_ann; struct fann *ann; zend_array *zs; fann_type *vals; unsigned int count;
	ZEND_PARSE_PARAMETERS_START(2,2) Z_PARAM_RESOURCE(z_ann) Z_PARAM_ARRAY_HT(zs) ZEND_PARSE_PARAMETERS_END();
	ann = php_fann_fetch_ann(z_ann);
	if (ann == NULL) { zend_throw_error(NULL, "Invalid fann neural network resource"); RETURN_THROWS(); }
	if (!php_fann_float_array_from_zend_array(zs, &vals, &count, 2)) { RETURN_THROWS(); }
	fann_set_cascade_activation_steepnesses(ann, vals, count);
	efree(vals);
	RETURN_TRUE;
}
/* }}} */

/* {{{ int fann_get_errno(resource $resource) */
PHP_FUNCTION(fann_get_errno)
{
	zval *z_resource;
	struct fann_error *err;
	ZEND_PARSE_PARAMETERS_START(1,1) Z_PARAM_RESOURCE(z_resource) ZEND_PARSE_PARAMETERS_END();
	err = php_fann_fetch_error_context(z_resource);
	if (err == NULL) { zend_throw_error(NULL, "Invalid fann resource"); RETURN_THROWS(); }
	RETURN_LONG((zend_long) fann_get_errno(err));
}
/* }}} */

/* {{{ bool fann_reset_errno(resource $resource) */
PHP_FUNCTION(fann_reset_errno)
{
	zval *z_resource;
	struct fann_error *err;
	ZEND_PARSE_PARAMETERS_START(1,1) Z_PARAM_RESOURCE(z_resource) ZEND_PARSE_PARAMETERS_END();
	err = php_fann_fetch_error_context(z_resource);
	if (err == NULL) { zend_throw_error(NULL, "Invalid fann resource"); RETURN_THROWS(); }
	fann_reset_errno(err);
	RETURN_TRUE;
}
/* }}} */

/* {{{ bool fann_reset_errstr(resource $resource) */
PHP_FUNCTION(fann_reset_errstr)
{
	zval *z_resource;
	struct fann_error *err;
	ZEND_PARSE_PARAMETERS_START(1,1) Z_PARAM_RESOURCE(z_resource) ZEND_PARSE_PARAMETERS_END();
	err = php_fann_fetch_error_context(z_resource);
	if (err == NULL) { zend_throw_error(NULL, "Invalid fann resource"); RETURN_THROWS(); }
	fann_reset_errstr(err);
	RETURN_TRUE;
}
/* }}} */

/* {{{ string fann_get_errstr(resource $resource) */
PHP_FUNCTION(fann_get_errstr)
{
	zval *z_resource;
	struct fann_error *err;
	char *msg;
	ZEND_PARSE_PARAMETERS_START(1,1) Z_PARAM_RESOURCE(z_resource) ZEND_PARSE_PARAMETERS_END();
	err = php_fann_fetch_error_context(z_resource);
	if (err == NULL) { zend_throw_error(NULL, "Invalid fann resource"); RETURN_THROWS(); }
	msg = fann_get_errstr(err);
	if (msg == NULL) {
		RETURN_EMPTY_STRING();
	}
	RETURN_STRING(msg);
}
/* }}} */

/* {{{ void fann_print_error(resource $resource) */
PHP_FUNCTION(fann_print_error)
{
	zval *z_resource;
	struct fann_error *err;
	ZEND_PARSE_PARAMETERS_START(1,1) Z_PARAM_RESOURCE(z_resource) ZEND_PARSE_PARAMETERS_END();
	err = php_fann_fetch_error_context(z_resource);
	if (err == NULL) { zend_throw_error(NULL, "Invalid fann resource"); RETURN_THROWS(); }
	fann_print_error(err);
}
/* }}} */

/* {{{ bool fann_set_error_log(resource $resource, ?string $log_file = null) */
PHP_FUNCTION(fann_set_error_log)
{
	zval *z_resource;
	struct fann_error *err;
	char *log_file = NULL;
	size_t log_file_len = 0;
	FILE *fp = NULL;

	ZEND_PARSE_PARAMETERS_START(1,2)
		Z_PARAM_RESOURCE(z_resource)
		Z_PARAM_OPTIONAL
		Z_PARAM_PATH_OR_NULL(log_file, log_file_len)
	ZEND_PARSE_PARAMETERS_END();

	err = php_fann_fetch_error_context(z_resource);
	if (err == NULL) { zend_throw_error(NULL, "Invalid fann resource"); RETURN_THROWS(); }

	if (log_file != NULL) {
		fp = fopen(log_file, "a");
		if (fp == NULL) {
			RETURN_FALSE;
		}
	}

	{
		zval *zold = zend_hash_index_find(&fann_error_logs, Z_RES_HANDLE_P(z_resource));
		if (zold != NULL && Z_TYPE_P(zold) == IS_LONG) {
			FILE *old_fp = (FILE *) (uintptr_t) Z_LVAL_P(zold);
			if (old_fp != NULL) {
				fclose(old_fp);
			}
			zend_hash_index_del(&fann_error_logs, Z_RES_HANDLE_P(z_resource));
		}
	}

	if (fp != NULL) {
		zval zv;
		ZVAL_LONG(&zv, (zend_long) (uintptr_t) fp);
		zend_hash_index_update(&fann_error_logs, Z_RES_HANDLE_P(z_resource), &zv);
	}

	fann_set_error_log(err, fp);
	RETURN_TRUE;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(fann)
{
	zend_hash_init(&fann_error_logs, 8, NULL, NULL, 1);
	le_fann_ann = zend_register_list_destructors_ex(php_fann_ann_dtor, NULL, "fann neural network", module_number);
	le_fann_train_data = zend_register_list_destructors_ex(php_fann_train_data_dtor, NULL, "fann training data", module_number);

	REGISTER_LONG_CONSTANT("FANN_TRAIN_INCREMENTAL", FANN_TRAIN_INCREMENTAL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_TRAIN_BATCH", FANN_TRAIN_BATCH, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_TRAIN_RPROP", FANN_TRAIN_RPROP, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_TRAIN_QUICKPROP", FANN_TRAIN_QUICKPROP, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_TRAIN_SARPROP", FANN_TRAIN_SARPROP, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("FANN_LINEAR", FANN_LINEAR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_THRESHOLD", FANN_THRESHOLD, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_THRESHOLD_SYMMETRIC", FANN_THRESHOLD_SYMMETRIC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_SIGMOID", FANN_SIGMOID, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_SIGMOID_STEPWISE", FANN_SIGMOID_STEPWISE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_SIGMOID_SYMMETRIC", FANN_SIGMOID_SYMMETRIC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_SIGMOID_SYMMETRIC_STEPWISE", FANN_SIGMOID_SYMMETRIC_STEPWISE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_GAUSSIAN", FANN_GAUSSIAN, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_GAUSSIAN_SYMMETRIC", FANN_GAUSSIAN_SYMMETRIC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_GAUSSIAN_STEPWISE", FANN_GAUSSIAN_STEPWISE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_ELLIOT", FANN_ELLIOT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_ELLIOT_SYMMETRIC", FANN_ELLIOT_SYMMETRIC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_LINEAR_PIECE", FANN_LINEAR_PIECE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_LINEAR_PIECE_SYMMETRIC", FANN_LINEAR_PIECE_SYMMETRIC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_SIN_SYMMETRIC", FANN_SIN_SYMMETRIC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_COS_SYMMETRIC", FANN_COS_SYMMETRIC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_SIN", FANN_SIN, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_COS", FANN_COS, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("FANN_ERRORFUNC_LINEAR", FANN_ERRORFUNC_LINEAR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_ERRORFUNC_TANH", FANN_ERRORFUNC_TANH, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("FANN_STOPFUNC_MSE", FANN_STOPFUNC_MSE, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_STOPFUNC_BIT", FANN_STOPFUNC_BIT, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("FANN_NETTYPE_LAYER", FANN_NETTYPE_LAYER, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_NETTYPE_SHORTCUT", FANN_NETTYPE_SHORTCUT, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("FANN_E_NO_ERROR", FANN_E_NO_ERROR, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_OPEN_CONFIG_R", FANN_E_CANT_OPEN_CONFIG_R, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_OPEN_CONFIG_W", FANN_E_CANT_OPEN_CONFIG_W, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_WRONG_CONFIG_VERSION", FANN_E_WRONG_CONFIG_VERSION, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_READ_CONFIG", FANN_E_CANT_READ_CONFIG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_READ_NEURON", FANN_E_CANT_READ_NEURON, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_READ_CONNECTIONS", FANN_E_CANT_READ_CONNECTIONS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_WRONG_NUM_CONNECTIONS", FANN_E_WRONG_NUM_CONNECTIONS, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_OPEN_TD_W", FANN_E_CANT_OPEN_TD_W, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_OPEN_TD_R", FANN_E_CANT_OPEN_TD_R, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_READ_TD", FANN_E_CANT_READ_TD, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_ALLOCATE_MEM", FANN_E_CANT_ALLOCATE_MEM, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_TRAIN_ACTIVATION", FANN_E_CANT_TRAIN_ACTIVATION, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_USE_ACTIVATION", FANN_E_CANT_USE_ACTIVATION, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_TRAIN_DATA_MISMATCH", FANN_E_TRAIN_DATA_MISMATCH, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_USE_TRAIN_ALG", FANN_E_CANT_USE_TRAIN_ALG, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_TRAIN_DATA_SUBSET", FANN_E_TRAIN_DATA_SUBSET, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_INDEX_OUT_OF_BOUND", FANN_E_INDEX_OUT_OF_BOUND, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_SCALE_NOT_PRESENT", FANN_E_SCALE_NOT_PRESENT, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_INPUT_NO_MATCH", FANN_E_INPUT_NO_MATCH, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_OUTPUT_NO_MATCH", FANN_E_OUTPUT_NO_MATCH, CONST_CS | CONST_PERSISTENT);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION */
PHP_MSHUTDOWN_FUNCTION(fann)
{
	zend_hash_destroy(&fann_error_logs);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION */
PHP_RINIT_FUNCTION(fann)
{
#if defined(ZTS) && defined(COMPILE_DL_FANN)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(fann)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "fann support", "enabled");
	php_info_print_table_end();
}
/* }}} */

/* {{{ fann_module_entry */
zend_module_entry fann_module_entry = {
	STANDARD_MODULE_HEADER,
	"fann",					/* Extension name */
	ext_functions,					/* zend_function_entry */
	PHP_MINIT(fann),				/* PHP_MINIT - Module initialization */
	PHP_MSHUTDOWN(fann),			/* PHP_MSHUTDOWN - Module shutdown */
	PHP_RINIT(fann),			/* PHP_RINIT - Request initialization */
	NULL,							/* PHP_RSHUTDOWN - Request shutdown */
	PHP_MINFO(fann),			/* PHP_MINFO - Module info */
	PHP_FANN_VERSION,		/* Version */
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_FANN
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(fann)
#endif
