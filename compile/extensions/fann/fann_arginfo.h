/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 24b3c620ba97de02d0dadacab82ecae1ca83c910 */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_version, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_create_standard, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, num_layers, IS_LONG, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, layers, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_create_standard_array, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, layers, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_create_sparse, 0, 2, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, connection_rate, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, num_layers, IS_LONG, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, layers, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_create_sparse_array, 0, 2, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, connection_rate, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, layers, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_create_shortcut arginfo_fann_create_standard

#define arginfo_fann_create_shortcut_array arginfo_fann_create_standard_array

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_create_from_file, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, configuration_file, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_copy, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_destroy, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_save, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, configuration_file, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_save_to_fixed, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, configuration_file, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_run, 0, 2, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, input, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_print_parameters, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_create_train, 0, 3, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, num_data, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, num_input, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, num_output, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_read_train_from_file, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, filename, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_save_train, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, train_data, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, filename, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_save_train_to_fixed, 0, 3, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, train_data, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, filename, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, decimal_point, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_destroy_train, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, train_data, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_train, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, input, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, desired_output, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_train_on_data, 0, 5, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, train_data, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, max_epochs, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, epochs_between_reports, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, desired_error, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_train_on_file, 0, 5, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, filename, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, max_epochs, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, epochs_between_reports, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, desired_error, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_test, 0, 3, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, input, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, desired_output, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_get_MSE, 0, 1, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_reset_MSE arginfo_fann_destroy

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_get_bit_fail, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_training_algorithm, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, training_algorithm, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_training_algorithm arginfo_fann_get_bit_fail

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_learning_rate, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, learning_rate, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_learning_rate arginfo_fann_get_MSE

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_activation_function_hidden, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, activation_function, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_set_activation_function_output arginfo_fann_set_activation_function_hidden

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_activation_function, 0, 4, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, activation_function, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, layer, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, neuron, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_activation_function_layer, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, activation_function, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, layer, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_get_activation_function, 0, 3, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, layer, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, neuron, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_get_activation_steepness, 0, 3, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, layer, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, neuron, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_activation_steepness, 0, 4, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, steepness, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, layer, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, neuron, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_activation_steepness_layer, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, steepness, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, layer, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_activation_steepness_hidden, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, steepness, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_set_activation_steepness_output arginfo_fann_set_activation_steepness_hidden

#define arginfo_fann_get_num_input arginfo_fann_get_bit_fail

#define arginfo_fann_get_num_output arginfo_fann_get_bit_fail

#define arginfo_fann_get_total_neurons arginfo_fann_get_bit_fail

#define arginfo_fann_get_total_connections arginfo_fann_get_bit_fail

#define arginfo_fann_get_network_type arginfo_fann_get_bit_fail

#define arginfo_fann_get_connection_rate arginfo_fann_get_MSE

#define arginfo_fann_get_num_layers arginfo_fann_get_bit_fail

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_get_layer_array, 0, 1, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_bias_array arginfo_fann_get_layer_array

#define arginfo_fann_get_connection_array arginfo_fann_get_layer_array

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_weight_array, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, connections, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_weight, 0, 4, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, from_neuron, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, to_neuron, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, weight, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_randomize_weights, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, min_weight, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, max_weight, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_init_weights, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, train_data, IS_MIXED, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_print_connections arginfo_fann_print_parameters

#define arginfo_fann_get_learning_momentum arginfo_fann_get_MSE

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_learning_momentum, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, learning_momentum, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_train_error_function arginfo_fann_get_bit_fail

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_train_error_function, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, train_error_function, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_train_stop_function arginfo_fann_get_bit_fail

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_train_stop_function, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, train_stop_function, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_bit_fail_limit arginfo_fann_get_MSE

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_bit_fail_limit, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, bit_fail_limit, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_quickprop_decay arginfo_fann_get_MSE

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_quickprop_decay, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, quickprop_decay, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_quickprop_mu arginfo_fann_get_MSE

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_quickprop_mu, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, quickprop_mu, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_rprop_increase_factor arginfo_fann_get_MSE

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_rprop_increase_factor, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, rprop_increase_factor, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_rprop_decrease_factor arginfo_fann_get_MSE

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_rprop_decrease_factor, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, rprop_decrease_factor, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_rprop_delta_min arginfo_fann_get_MSE

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_rprop_delta_min, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, rprop_delta_min, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_rprop_delta_max arginfo_fann_get_MSE

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_rprop_delta_max, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, rprop_delta_max, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_rprop_delta_zero arginfo_fann_get_MSE

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_rprop_delta_zero, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, rprop_delta_zero, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_sarprop_weight_decay_shift arginfo_fann_get_MSE

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_sarprop_weight_decay_shift, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, sarprop_weight_decay_shift, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_sarprop_step_error_threshold_factor arginfo_fann_get_MSE

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_sarprop_step_error_threshold_factor, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, sarprop_step_error_threshold_factor, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_sarprop_step_error_shift arginfo_fann_get_MSE

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_sarprop_step_error_shift, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, sarprop_step_error_shift, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_sarprop_temperature arginfo_fann_get_MSE

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_sarprop_temperature, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, sarprop_temperature, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_shuffle_train_data arginfo_fann_destroy_train

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_scale_input_train_data, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, train_data, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, new_min, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, new_max, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_scale_output_train_data arginfo_fann_scale_input_train_data

#define arginfo_fann_scale_train_data arginfo_fann_scale_input_train_data

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_merge_train_data, 0, 2, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, data1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, data2, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_duplicate_train_data, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, train_data, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_subset_train_data, 0, 3, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, train_data, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, pos, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, length, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_length_train_data, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, train_data, IS_MIXED, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_num_input_train_data arginfo_fann_length_train_data

#define arginfo_fann_num_output_train_data arginfo_fann_length_train_data

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_train_epoch, 0, 2, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, train_data, IS_MIXED, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_test_data arginfo_fann_train_epoch

#define arginfo_fann_scale_train arginfo_fann_init_weights

#define arginfo_fann_descale_train arginfo_fann_init_weights

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_input_scaling_params, 0, 4, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, train_data, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, new_input_min, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, new_input_max, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_output_scaling_params, 0, 4, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, train_data, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, new_output_min, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, new_output_max, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_scaling_params, 0, 6, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, train_data, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, new_input_min, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, new_input_max, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, new_output_min, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, new_output_max, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_clear_scaling_params arginfo_fann_destroy

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_scale_input, 0, 2, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, input_vector, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_scale_output, 0, 2, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, output_vector, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_descale_input arginfo_fann_scale_input

#define arginfo_fann_descale_output arginfo_fann_scale_output

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_cascadetrain_on_data, 0, 5, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, train_data, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, max_neurons, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, neurons_between_reports, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, desired_error, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_cascadetrain_on_file, 0, 5, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, filename, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, max_neurons, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, neurons_between_reports, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, desired_error, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_cascade_output_change_fraction arginfo_fann_get_MSE

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_cascade_output_change_fraction, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, cascade_output_change_fraction, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_cascade_output_stagnation_epochs arginfo_fann_get_bit_fail

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_cascade_output_stagnation_epochs, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, cascade_output_stagnation_epochs, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_cascade_candidate_change_fraction arginfo_fann_get_MSE

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_cascade_candidate_change_fraction, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, cascade_candidate_change_fraction, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_cascade_candidate_stagnation_epochs arginfo_fann_get_bit_fail

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_cascade_candidate_stagnation_epochs, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, cascade_candidate_stagnation_epochs, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_cascade_weight_multiplier arginfo_fann_get_MSE

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_cascade_weight_multiplier, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, cascade_weight_multiplier, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_cascade_candidate_limit arginfo_fann_get_MSE

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_cascade_candidate_limit, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, cascade_candidate_limit, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_cascade_max_out_epochs arginfo_fann_get_bit_fail

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_cascade_max_out_epochs, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, cascade_max_out_epochs, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_cascade_min_out_epochs arginfo_fann_get_bit_fail

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_cascade_min_out_epochs, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, cascade_min_out_epochs, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_cascade_max_cand_epochs arginfo_fann_get_bit_fail

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_cascade_max_cand_epochs, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, cascade_max_cand_epochs, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_cascade_min_cand_epochs arginfo_fann_get_bit_fail

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_cascade_min_cand_epochs, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, cascade_min_cand_epochs, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_cascade_num_candidates arginfo_fann_get_bit_fail

#define arginfo_fann_get_cascade_activation_functions_count arginfo_fann_get_bit_fail

#define arginfo_fann_get_cascade_activation_functions arginfo_fann_get_layer_array

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_cascade_activation_functions, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, cascade_activation_functions, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_cascade_activation_steepnesses_count arginfo_fann_get_bit_fail

#define arginfo_fann_get_cascade_activation_steepnesses arginfo_fann_get_layer_array

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_cascade_activation_steepnesses, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, cascade_activation_steepnesses, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_get_cascade_num_candidate_groups arginfo_fann_get_bit_fail

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_cascade_num_candidate_groups, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, ann, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, cascade_num_candidate_groups, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_get_errno, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, resource, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_reset_errno, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, resource, IS_MIXED, 0)
ZEND_END_ARG_INFO()

#define arginfo_fann_reset_errstr arginfo_fann_reset_errno

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_get_errstr, 0, 1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, resource, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_print_error, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, resource, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_fann_set_error_log, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, resource, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, log_file, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()


ZEND_FUNCTION(fann_version);
ZEND_FUNCTION(fann_create_standard);
ZEND_FUNCTION(fann_create_standard_array);
ZEND_FUNCTION(fann_create_sparse);
ZEND_FUNCTION(fann_create_sparse_array);
ZEND_FUNCTION(fann_create_shortcut);
ZEND_FUNCTION(fann_create_shortcut_array);
ZEND_FUNCTION(fann_create_from_file);
ZEND_FUNCTION(fann_copy);
ZEND_FUNCTION(fann_destroy);
ZEND_FUNCTION(fann_save);
ZEND_FUNCTION(fann_save_to_fixed);
ZEND_FUNCTION(fann_run);
ZEND_FUNCTION(fann_print_parameters);
ZEND_FUNCTION(fann_create_train);
ZEND_FUNCTION(fann_read_train_from_file);
ZEND_FUNCTION(fann_save_train);
ZEND_FUNCTION(fann_save_train_to_fixed);
ZEND_FUNCTION(fann_destroy_train);
ZEND_FUNCTION(fann_train);
ZEND_FUNCTION(fann_train_on_data);
ZEND_FUNCTION(fann_train_on_file);
ZEND_FUNCTION(fann_test);
ZEND_FUNCTION(fann_get_MSE);
ZEND_FUNCTION(fann_reset_MSE);
ZEND_FUNCTION(fann_get_bit_fail);
ZEND_FUNCTION(fann_set_training_algorithm);
ZEND_FUNCTION(fann_get_training_algorithm);
ZEND_FUNCTION(fann_set_learning_rate);
ZEND_FUNCTION(fann_get_learning_rate);
ZEND_FUNCTION(fann_set_activation_function_hidden);
ZEND_FUNCTION(fann_set_activation_function_output);
ZEND_FUNCTION(fann_set_activation_function);
ZEND_FUNCTION(fann_set_activation_function_layer);
ZEND_FUNCTION(fann_get_activation_function);
ZEND_FUNCTION(fann_get_activation_steepness);
ZEND_FUNCTION(fann_set_activation_steepness);
ZEND_FUNCTION(fann_set_activation_steepness_layer);
ZEND_FUNCTION(fann_set_activation_steepness_hidden);
ZEND_FUNCTION(fann_set_activation_steepness_output);
ZEND_FUNCTION(fann_get_num_input);
ZEND_FUNCTION(fann_get_num_output);
ZEND_FUNCTION(fann_get_total_neurons);
ZEND_FUNCTION(fann_get_total_connections);
ZEND_FUNCTION(fann_get_network_type);
ZEND_FUNCTION(fann_get_connection_rate);
ZEND_FUNCTION(fann_get_num_layers);
ZEND_FUNCTION(fann_get_layer_array);
ZEND_FUNCTION(fann_get_bias_array);
ZEND_FUNCTION(fann_get_connection_array);
ZEND_FUNCTION(fann_set_weight_array);
ZEND_FUNCTION(fann_set_weight);
ZEND_FUNCTION(fann_randomize_weights);
ZEND_FUNCTION(fann_init_weights);
ZEND_FUNCTION(fann_print_connections);
ZEND_FUNCTION(fann_get_learning_momentum);
ZEND_FUNCTION(fann_set_learning_momentum);
ZEND_FUNCTION(fann_get_train_error_function);
ZEND_FUNCTION(fann_set_train_error_function);
ZEND_FUNCTION(fann_get_train_stop_function);
ZEND_FUNCTION(fann_set_train_stop_function);
ZEND_FUNCTION(fann_get_bit_fail_limit);
ZEND_FUNCTION(fann_set_bit_fail_limit);
ZEND_FUNCTION(fann_get_quickprop_decay);
ZEND_FUNCTION(fann_set_quickprop_decay);
ZEND_FUNCTION(fann_get_quickprop_mu);
ZEND_FUNCTION(fann_set_quickprop_mu);
ZEND_FUNCTION(fann_get_rprop_increase_factor);
ZEND_FUNCTION(fann_set_rprop_increase_factor);
ZEND_FUNCTION(fann_get_rprop_decrease_factor);
ZEND_FUNCTION(fann_set_rprop_decrease_factor);
ZEND_FUNCTION(fann_get_rprop_delta_min);
ZEND_FUNCTION(fann_set_rprop_delta_min);
ZEND_FUNCTION(fann_get_rprop_delta_max);
ZEND_FUNCTION(fann_set_rprop_delta_max);
ZEND_FUNCTION(fann_get_rprop_delta_zero);
ZEND_FUNCTION(fann_set_rprop_delta_zero);
ZEND_FUNCTION(fann_get_sarprop_weight_decay_shift);
ZEND_FUNCTION(fann_set_sarprop_weight_decay_shift);
ZEND_FUNCTION(fann_get_sarprop_step_error_threshold_factor);
ZEND_FUNCTION(fann_set_sarprop_step_error_threshold_factor);
ZEND_FUNCTION(fann_get_sarprop_step_error_shift);
ZEND_FUNCTION(fann_set_sarprop_step_error_shift);
ZEND_FUNCTION(fann_get_sarprop_temperature);
ZEND_FUNCTION(fann_set_sarprop_temperature);
ZEND_FUNCTION(fann_shuffle_train_data);
ZEND_FUNCTION(fann_scale_input_train_data);
ZEND_FUNCTION(fann_scale_output_train_data);
ZEND_FUNCTION(fann_scale_train_data);
ZEND_FUNCTION(fann_merge_train_data);
ZEND_FUNCTION(fann_duplicate_train_data);
ZEND_FUNCTION(fann_subset_train_data);
ZEND_FUNCTION(fann_length_train_data);
ZEND_FUNCTION(fann_num_input_train_data);
ZEND_FUNCTION(fann_num_output_train_data);
ZEND_FUNCTION(fann_train_epoch);
ZEND_FUNCTION(fann_test_data);
ZEND_FUNCTION(fann_scale_train);
ZEND_FUNCTION(fann_descale_train);
ZEND_FUNCTION(fann_set_input_scaling_params);
ZEND_FUNCTION(fann_set_output_scaling_params);
ZEND_FUNCTION(fann_set_scaling_params);
ZEND_FUNCTION(fann_clear_scaling_params);
ZEND_FUNCTION(fann_scale_input);
ZEND_FUNCTION(fann_scale_output);
ZEND_FUNCTION(fann_descale_input);
ZEND_FUNCTION(fann_descale_output);
ZEND_FUNCTION(fann_cascadetrain_on_data);
ZEND_FUNCTION(fann_cascadetrain_on_file);
ZEND_FUNCTION(fann_get_cascade_output_change_fraction);
ZEND_FUNCTION(fann_set_cascade_output_change_fraction);
ZEND_FUNCTION(fann_get_cascade_output_stagnation_epochs);
ZEND_FUNCTION(fann_set_cascade_output_stagnation_epochs);
ZEND_FUNCTION(fann_get_cascade_candidate_change_fraction);
ZEND_FUNCTION(fann_set_cascade_candidate_change_fraction);
ZEND_FUNCTION(fann_get_cascade_candidate_stagnation_epochs);
ZEND_FUNCTION(fann_set_cascade_candidate_stagnation_epochs);
ZEND_FUNCTION(fann_get_cascade_weight_multiplier);
ZEND_FUNCTION(fann_set_cascade_weight_multiplier);
ZEND_FUNCTION(fann_get_cascade_candidate_limit);
ZEND_FUNCTION(fann_set_cascade_candidate_limit);
ZEND_FUNCTION(fann_get_cascade_max_out_epochs);
ZEND_FUNCTION(fann_set_cascade_max_out_epochs);
ZEND_FUNCTION(fann_get_cascade_min_out_epochs);
ZEND_FUNCTION(fann_set_cascade_min_out_epochs);
ZEND_FUNCTION(fann_get_cascade_max_cand_epochs);
ZEND_FUNCTION(fann_set_cascade_max_cand_epochs);
ZEND_FUNCTION(fann_get_cascade_min_cand_epochs);
ZEND_FUNCTION(fann_set_cascade_min_cand_epochs);
ZEND_FUNCTION(fann_get_cascade_num_candidates);
ZEND_FUNCTION(fann_get_cascade_activation_functions_count);
ZEND_FUNCTION(fann_get_cascade_activation_functions);
ZEND_FUNCTION(fann_set_cascade_activation_functions);
ZEND_FUNCTION(fann_get_cascade_activation_steepnesses_count);
ZEND_FUNCTION(fann_get_cascade_activation_steepnesses);
ZEND_FUNCTION(fann_set_cascade_activation_steepnesses);
ZEND_FUNCTION(fann_get_cascade_num_candidate_groups);
ZEND_FUNCTION(fann_set_cascade_num_candidate_groups);
ZEND_FUNCTION(fann_get_errno);
ZEND_FUNCTION(fann_reset_errno);
ZEND_FUNCTION(fann_reset_errstr);
ZEND_FUNCTION(fann_get_errstr);
ZEND_FUNCTION(fann_print_error);
ZEND_FUNCTION(fann_set_error_log);


static const zend_function_entry ext_functions[] = {
	ZEND_FE(fann_version, arginfo_fann_version)
	ZEND_FE(fann_create_standard, arginfo_fann_create_standard)
	ZEND_FE(fann_create_standard_array, arginfo_fann_create_standard_array)
	ZEND_FE(fann_create_sparse, arginfo_fann_create_sparse)
	ZEND_FE(fann_create_sparse_array, arginfo_fann_create_sparse_array)
	ZEND_FE(fann_create_shortcut, arginfo_fann_create_shortcut)
	ZEND_FE(fann_create_shortcut_array, arginfo_fann_create_shortcut_array)
	ZEND_FE(fann_create_from_file, arginfo_fann_create_from_file)
	ZEND_FE(fann_copy, arginfo_fann_copy)
	ZEND_FE(fann_destroy, arginfo_fann_destroy)
	ZEND_FE(fann_save, arginfo_fann_save)
	ZEND_FE(fann_save_to_fixed, arginfo_fann_save_to_fixed)
	ZEND_FE(fann_run, arginfo_fann_run)
	ZEND_FE(fann_print_parameters, arginfo_fann_print_parameters)
	ZEND_FE(fann_create_train, arginfo_fann_create_train)
	ZEND_FE(fann_read_train_from_file, arginfo_fann_read_train_from_file)
	ZEND_FE(fann_save_train, arginfo_fann_save_train)
	ZEND_FE(fann_save_train_to_fixed, arginfo_fann_save_train_to_fixed)
	ZEND_FE(fann_destroy_train, arginfo_fann_destroy_train)
	ZEND_FE(fann_train, arginfo_fann_train)
	ZEND_FE(fann_train_on_data, arginfo_fann_train_on_data)
	ZEND_FE(fann_train_on_file, arginfo_fann_train_on_file)
	ZEND_FE(fann_test, arginfo_fann_test)
	ZEND_FE(fann_get_MSE, arginfo_fann_get_MSE)
	ZEND_FE(fann_reset_MSE, arginfo_fann_reset_MSE)
	ZEND_FE(fann_get_bit_fail, arginfo_fann_get_bit_fail)
	ZEND_FE(fann_set_training_algorithm, arginfo_fann_set_training_algorithm)
	ZEND_FE(fann_get_training_algorithm, arginfo_fann_get_training_algorithm)
	ZEND_FE(fann_set_learning_rate, arginfo_fann_set_learning_rate)
	ZEND_FE(fann_get_learning_rate, arginfo_fann_get_learning_rate)
	ZEND_FE(fann_set_activation_function_hidden, arginfo_fann_set_activation_function_hidden)
	ZEND_FE(fann_set_activation_function_output, arginfo_fann_set_activation_function_output)
	ZEND_FE(fann_set_activation_function, arginfo_fann_set_activation_function)
	ZEND_FE(fann_set_activation_function_layer, arginfo_fann_set_activation_function_layer)
	ZEND_FE(fann_get_activation_function, arginfo_fann_get_activation_function)
	ZEND_FE(fann_get_activation_steepness, arginfo_fann_get_activation_steepness)
	ZEND_FE(fann_set_activation_steepness, arginfo_fann_set_activation_steepness)
	ZEND_FE(fann_set_activation_steepness_layer, arginfo_fann_set_activation_steepness_layer)
	ZEND_FE(fann_set_activation_steepness_hidden, arginfo_fann_set_activation_steepness_hidden)
	ZEND_FE(fann_set_activation_steepness_output, arginfo_fann_set_activation_steepness_output)
	ZEND_FE(fann_get_num_input, arginfo_fann_get_num_input)
	ZEND_FE(fann_get_num_output, arginfo_fann_get_num_output)
	ZEND_FE(fann_get_total_neurons, arginfo_fann_get_total_neurons)
	ZEND_FE(fann_get_total_connections, arginfo_fann_get_total_connections)
	ZEND_FE(fann_get_network_type, arginfo_fann_get_network_type)
	ZEND_FE(fann_get_connection_rate, arginfo_fann_get_connection_rate)
	ZEND_FE(fann_get_num_layers, arginfo_fann_get_num_layers)
	ZEND_FE(fann_get_layer_array, arginfo_fann_get_layer_array)
	ZEND_FE(fann_get_bias_array, arginfo_fann_get_bias_array)
	ZEND_FE(fann_get_connection_array, arginfo_fann_get_connection_array)
	ZEND_FE(fann_set_weight_array, arginfo_fann_set_weight_array)
	ZEND_FE(fann_set_weight, arginfo_fann_set_weight)
	ZEND_FE(fann_randomize_weights, arginfo_fann_randomize_weights)
	ZEND_FE(fann_init_weights, arginfo_fann_init_weights)
	ZEND_FE(fann_print_connections, arginfo_fann_print_connections)
	ZEND_FE(fann_get_learning_momentum, arginfo_fann_get_learning_momentum)
	ZEND_FE(fann_set_learning_momentum, arginfo_fann_set_learning_momentum)
	ZEND_FE(fann_get_train_error_function, arginfo_fann_get_train_error_function)
	ZEND_FE(fann_set_train_error_function, arginfo_fann_set_train_error_function)
	ZEND_FE(fann_get_train_stop_function, arginfo_fann_get_train_stop_function)
	ZEND_FE(fann_set_train_stop_function, arginfo_fann_set_train_stop_function)
	ZEND_FE(fann_get_bit_fail_limit, arginfo_fann_get_bit_fail_limit)
	ZEND_FE(fann_set_bit_fail_limit, arginfo_fann_set_bit_fail_limit)
	ZEND_FE(fann_get_quickprop_decay, arginfo_fann_get_quickprop_decay)
	ZEND_FE(fann_set_quickprop_decay, arginfo_fann_set_quickprop_decay)
	ZEND_FE(fann_get_quickprop_mu, arginfo_fann_get_quickprop_mu)
	ZEND_FE(fann_set_quickprop_mu, arginfo_fann_set_quickprop_mu)
	ZEND_FE(fann_get_rprop_increase_factor, arginfo_fann_get_rprop_increase_factor)
	ZEND_FE(fann_set_rprop_increase_factor, arginfo_fann_set_rprop_increase_factor)
	ZEND_FE(fann_get_rprop_decrease_factor, arginfo_fann_get_rprop_decrease_factor)
	ZEND_FE(fann_set_rprop_decrease_factor, arginfo_fann_set_rprop_decrease_factor)
	ZEND_FE(fann_get_rprop_delta_min, arginfo_fann_get_rprop_delta_min)
	ZEND_FE(fann_set_rprop_delta_min, arginfo_fann_set_rprop_delta_min)
	ZEND_FE(fann_get_rprop_delta_max, arginfo_fann_get_rprop_delta_max)
	ZEND_FE(fann_set_rprop_delta_max, arginfo_fann_set_rprop_delta_max)
	ZEND_FE(fann_get_rprop_delta_zero, arginfo_fann_get_rprop_delta_zero)
	ZEND_FE(fann_set_rprop_delta_zero, arginfo_fann_set_rprop_delta_zero)
	ZEND_FE(fann_get_sarprop_weight_decay_shift, arginfo_fann_get_sarprop_weight_decay_shift)
	ZEND_FE(fann_set_sarprop_weight_decay_shift, arginfo_fann_set_sarprop_weight_decay_shift)
	ZEND_FE(fann_get_sarprop_step_error_threshold_factor, arginfo_fann_get_sarprop_step_error_threshold_factor)
	ZEND_FE(fann_set_sarprop_step_error_threshold_factor, arginfo_fann_set_sarprop_step_error_threshold_factor)
	ZEND_FE(fann_get_sarprop_step_error_shift, arginfo_fann_get_sarprop_step_error_shift)
	ZEND_FE(fann_set_sarprop_step_error_shift, arginfo_fann_set_sarprop_step_error_shift)
	ZEND_FE(fann_get_sarprop_temperature, arginfo_fann_get_sarprop_temperature)
	ZEND_FE(fann_set_sarprop_temperature, arginfo_fann_set_sarprop_temperature)
	ZEND_FE(fann_shuffle_train_data, arginfo_fann_shuffle_train_data)
	ZEND_FE(fann_scale_input_train_data, arginfo_fann_scale_input_train_data)
	ZEND_FE(fann_scale_output_train_data, arginfo_fann_scale_output_train_data)
	ZEND_FE(fann_scale_train_data, arginfo_fann_scale_train_data)
	ZEND_FE(fann_merge_train_data, arginfo_fann_merge_train_data)
	ZEND_FE(fann_duplicate_train_data, arginfo_fann_duplicate_train_data)
	ZEND_FE(fann_subset_train_data, arginfo_fann_subset_train_data)
	ZEND_FE(fann_length_train_data, arginfo_fann_length_train_data)
	ZEND_FE(fann_num_input_train_data, arginfo_fann_num_input_train_data)
	ZEND_FE(fann_num_output_train_data, arginfo_fann_num_output_train_data)
	ZEND_FE(fann_train_epoch, arginfo_fann_train_epoch)
	ZEND_FE(fann_test_data, arginfo_fann_test_data)
	ZEND_FE(fann_scale_train, arginfo_fann_scale_train)
	ZEND_FE(fann_descale_train, arginfo_fann_descale_train)
	ZEND_FE(fann_set_input_scaling_params, arginfo_fann_set_input_scaling_params)
	ZEND_FE(fann_set_output_scaling_params, arginfo_fann_set_output_scaling_params)
	ZEND_FE(fann_set_scaling_params, arginfo_fann_set_scaling_params)
	ZEND_FE(fann_clear_scaling_params, arginfo_fann_clear_scaling_params)
	ZEND_FE(fann_scale_input, arginfo_fann_scale_input)
	ZEND_FE(fann_scale_output, arginfo_fann_scale_output)
	ZEND_FE(fann_descale_input, arginfo_fann_descale_input)
	ZEND_FE(fann_descale_output, arginfo_fann_descale_output)
	ZEND_FE(fann_cascadetrain_on_data, arginfo_fann_cascadetrain_on_data)
	ZEND_FE(fann_cascadetrain_on_file, arginfo_fann_cascadetrain_on_file)
	ZEND_FE(fann_get_cascade_output_change_fraction, arginfo_fann_get_cascade_output_change_fraction)
	ZEND_FE(fann_set_cascade_output_change_fraction, arginfo_fann_set_cascade_output_change_fraction)
	ZEND_FE(fann_get_cascade_output_stagnation_epochs, arginfo_fann_get_cascade_output_stagnation_epochs)
	ZEND_FE(fann_set_cascade_output_stagnation_epochs, arginfo_fann_set_cascade_output_stagnation_epochs)
	ZEND_FE(fann_get_cascade_candidate_change_fraction, arginfo_fann_get_cascade_candidate_change_fraction)
	ZEND_FE(fann_set_cascade_candidate_change_fraction, arginfo_fann_set_cascade_candidate_change_fraction)
	ZEND_FE(fann_get_cascade_candidate_stagnation_epochs, arginfo_fann_get_cascade_candidate_stagnation_epochs)
	ZEND_FE(fann_set_cascade_candidate_stagnation_epochs, arginfo_fann_set_cascade_candidate_stagnation_epochs)
	ZEND_FE(fann_get_cascade_weight_multiplier, arginfo_fann_get_cascade_weight_multiplier)
	ZEND_FE(fann_set_cascade_weight_multiplier, arginfo_fann_set_cascade_weight_multiplier)
	ZEND_FE(fann_get_cascade_candidate_limit, arginfo_fann_get_cascade_candidate_limit)
	ZEND_FE(fann_set_cascade_candidate_limit, arginfo_fann_set_cascade_candidate_limit)
	ZEND_FE(fann_get_cascade_max_out_epochs, arginfo_fann_get_cascade_max_out_epochs)
	ZEND_FE(fann_set_cascade_max_out_epochs, arginfo_fann_set_cascade_max_out_epochs)
	ZEND_FE(fann_get_cascade_min_out_epochs, arginfo_fann_get_cascade_min_out_epochs)
	ZEND_FE(fann_set_cascade_min_out_epochs, arginfo_fann_set_cascade_min_out_epochs)
	ZEND_FE(fann_get_cascade_max_cand_epochs, arginfo_fann_get_cascade_max_cand_epochs)
	ZEND_FE(fann_set_cascade_max_cand_epochs, arginfo_fann_set_cascade_max_cand_epochs)
	ZEND_FE(fann_get_cascade_min_cand_epochs, arginfo_fann_get_cascade_min_cand_epochs)
	ZEND_FE(fann_set_cascade_min_cand_epochs, arginfo_fann_set_cascade_min_cand_epochs)
	ZEND_FE(fann_get_cascade_num_candidates, arginfo_fann_get_cascade_num_candidates)
	ZEND_FE(fann_get_cascade_activation_functions_count, arginfo_fann_get_cascade_activation_functions_count)
	ZEND_FE(fann_get_cascade_activation_functions, arginfo_fann_get_cascade_activation_functions)
	ZEND_FE(fann_set_cascade_activation_functions, arginfo_fann_set_cascade_activation_functions)
	ZEND_FE(fann_get_cascade_activation_steepnesses_count, arginfo_fann_get_cascade_activation_steepnesses_count)
	ZEND_FE(fann_get_cascade_activation_steepnesses, arginfo_fann_get_cascade_activation_steepnesses)
	ZEND_FE(fann_set_cascade_activation_steepnesses, arginfo_fann_set_cascade_activation_steepnesses)
	ZEND_FE(fann_get_cascade_num_candidate_groups, arginfo_fann_get_cascade_num_candidate_groups)
	ZEND_FE(fann_set_cascade_num_candidate_groups, arginfo_fann_set_cascade_num_candidate_groups)
	ZEND_FE(fann_get_errno, arginfo_fann_get_errno)
	ZEND_FE(fann_reset_errno, arginfo_fann_reset_errno)
	ZEND_FE(fann_reset_errstr, arginfo_fann_reset_errstr)
	ZEND_FE(fann_get_errstr, arginfo_fann_get_errstr)
	ZEND_FE(fann_print_error, arginfo_fann_print_error)
	ZEND_FE(fann_set_error_log, arginfo_fann_set_error_log)
	ZEND_FE_END
};

static void register_fann_symbols(int module_number)
{
	REGISTER_LONG_CONSTANT("FANN_TRAIN_INCREMENTAL", 0, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_TRAIN_BATCH", 1, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_TRAIN_RPROP", 2, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_TRAIN_QUICKPROP", 3, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_TRAIN_SARPROP", 4, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_LINEAR", 0, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_THRESHOLD", 1, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_THRESHOLD_SYMMETRIC", 2, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_SIGMOID", 3, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_SIGMOID_STEPWISE", 4, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_SIGMOID_SYMMETRIC", 5, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_SIGMOID_SYMMETRIC_STEPWISE", 6, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_GAUSSIAN", 7, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_GAUSSIAN_SYMMETRIC", 8, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_GAUSSIAN_STEPWISE", 9, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_ELLIOT", 10, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_ELLIOT_SYMMETRIC", 11, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_LINEAR_PIECE", 12, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_LINEAR_PIECE_SYMMETRIC", 13, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_SIN_SYMMETRIC", 14, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_COS_SYMMETRIC", 15, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_SIN", 16, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_COS", 17, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_ERRORFUNC_LINEAR", 0, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_ERRORFUNC_TANH", 1, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_STOPFUNC_MSE", 0, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_STOPFUNC_BIT", 1, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_NETTYPE_LAYER", 0, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_NETTYPE_SHORTCUT", 1, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_NO_ERROR", 0, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_OPEN_CONFIG_R", 1, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_OPEN_CONFIG_W", 2, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_WRONG_CONFIG_VERSION", 3, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_READ_CONFIG", 4, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_READ_NEURON", 5, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_READ_CONNECTIONS", 6, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_WRONG_NUM_CONNECTIONS", 7, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_OPEN_TD_W", 8, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_OPEN_TD_R", 9, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_READ_TD", 10, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_ALLOCATE_MEM", 11, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_TRAIN_ACTIVATION", 12, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_USE_ACTIVATION", 13, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_TRAIN_DATA_MISMATCH", 14, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_CANT_USE_TRAIN_ALG", 15, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_TRAIN_DATA_SUBSET", 16, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_INDEX_OUT_OF_BOUND", 17, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_SCALE_NOT_PRESENT", 18, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_INPUT_NO_MATCH", 19, CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("FANN_E_OUTPUT_NO_MATCH", 20, CONST_PERSISTENT);
}
