<?php

/**
 * @generate-class-entries
 */

/**
 * Returns the extension version string.
 */
function fann_version(): string {}

/**
 * Creates a fully connected standard neural network from variadic layer sizes.
 *
 * @param int $num_layers Total number of layers, including input and output layers.
 * @param int ...$layers Number of neurons for each layer.
 * @return resource|false Neural network resource on success, false on failure.
 */
function fann_create_standard(int $num_layers, int ...$layers): mixed {}

/**
 * Creates a fully connected standard neural network from a layer-size array.
 *
 * @param array $layers Number of neurons for each layer.
 * @return resource|false Neural network resource on success, false on failure.
 */
function fann_create_standard_array(array $layers): mixed {}

/**
 * Creates a sparse neural network from variadic layer sizes.
 *
 * @param float $connection_rate Sparse connection rate in range (0, 1].
 * @param int $num_layers Total number of layers, including input and output layers.
 * @param int ...$layers Number of neurons for each layer.
 * @return resource|false Neural network resource on success, false on failure.
 */
function fann_create_sparse(float $connection_rate, int $num_layers, int ...$layers): mixed {}

/**
 * Creates a sparse neural network from a layer-size array.
 *
 * @param float $connection_rate Sparse connection rate in range (0, 1].
 * @param array $layers Number of neurons for each layer.
 * @return resource|false Neural network resource on success, false on failure.
 */
function fann_create_sparse_array(float $connection_rate, array $layers): mixed {}

/**
 * Creates a shortcut neural network from variadic layer sizes.
 *
 * @param int $num_layers Total number of layers, including input and output layers.
 * @param int ...$layers Number of neurons for each layer.
 * @return resource|false Neural network resource on success, false on failure.
 */
function fann_create_shortcut(int $num_layers, int ...$layers): mixed {}

/**
 * Creates a shortcut neural network from a layer-size array.
 *
 * @param array $layers Number of neurons for each layer.
 * @return resource|false Neural network resource on success, false on failure.
 */
function fann_create_shortcut_array(array $layers): mixed {}

/**
 * Loads a neural network from a saved configuration file.
 *
 * @param string $configuration_file Path to a `.net` file.
 * @return resource|false Neural network resource on success, false on failure.
 */
function fann_create_from_file(string $configuration_file): mixed {}

/**
 * Creates a copy of an existing neural network.
 *
 * @param resource $ann Neural network resource.
 * @return resource|false Copied network resource on success, false on failure.
 */
function fann_copy(mixed $ann): mixed {}

/**
 * Destroys a neural network resource.
 *
 * @param resource $ann Neural network resource.
 */
function fann_destroy(mixed $ann): bool {}

/**
 * Saves a neural network to a configuration file.
 *
 * @param resource $ann Neural network resource.
 * @param string $configuration_file Destination file path.
 */
function fann_save(mixed $ann, string $configuration_file): bool {}
/** @param resource $ann */
function fann_save_to_fixed(mixed $ann, string $configuration_file): int {}

/**
 * Runs inference for one input vector.
 *
 * @param resource $ann Neural network resource.
 * @param array $input Input values; size must match network input count.
 * @return array|false Output vector on success, false on failure.
 */
function fann_run(mixed $ann, array $input): mixed {}

/**
 * Prints network parameters to standard output.
 *
 * @param resource $ann Neural network resource.
 */
function fann_print_parameters(mixed $ann): void {}

/**
 * Allocates an empty training-data structure.
 *
 * @param int $num_data Number of training rows.
 * @param int $num_input Number of input values per row.
 * @param int $num_output Number of output values per row.
 * @return resource|false Training data resource on success, false on failure.
 */
function fann_create_train(int $num_data, int $num_input, int $num_output): mixed {}

/**
 * Loads training data from file.
 *
 * @param string $filename Path to a FANN training-data file.
 * @return resource|false Training data resource on success, false on failure.
 */
function fann_read_train_from_file(string $filename): mixed {}

/**
 * Saves training data to file.
 *
 * @param resource $train_data Training data resource.
 * @param string $filename Destination file path.
 */
function fann_save_train(mixed $train_data, string $filename): bool {}
/** @param resource $train_data */
function fann_save_train_to_fixed(mixed $train_data, string $filename, int $decimal_point): int {}

/**
 * Destroys a training-data resource.
 *
 * @param resource $train_data Training data resource.
 */
function fann_destroy_train(mixed $train_data): bool {}

/**
 * Trains the network with one input/output pattern.
 *
 * @param resource $ann Neural network resource.
 * @param array $input Input vector.
 * @param array $desired_output Expected output vector.
 */
function fann_train(mixed $ann, array $input, array $desired_output): bool {}

/**
 * Trains the network using an in-memory training dataset.
 *
 * @param resource $ann Neural network resource.
 * @param resource $train_data Training data resource.
 * @param int $max_epochs Maximum training epochs.
 * @param int $epochs_between_reports Reporting interval.
 * @param float $desired_error Target error threshold.
 */
function fann_train_on_data(
	mixed $ann,
	mixed $train_data,
	int $max_epochs,
	int $epochs_between_reports,
	float $desired_error
): bool {}

/**
 * Trains the network using a training-data file.
 *
 * @param resource $ann Neural network resource.
 * @param string $filename Training-data file path.
 * @param int $max_epochs Maximum training epochs.
 * @param int $epochs_between_reports Reporting interval.
 * @param float $desired_error Target error threshold.
 */
function fann_train_on_file(
	mixed $ann,
	string $filename,
	int $max_epochs,
	int $epochs_between_reports,
	float $desired_error
): bool {}

/**
 * Tests the network with one pattern and updates internal error state.
 *
 * @param resource $ann Neural network resource.
 * @param array $input Input vector.
 * @param array $desired_output Expected output vector.
 * @return array|false Output vector on success, false on failure.
 */
function fann_test(mixed $ann, array $input, array $desired_output): mixed {}

/**
 * Returns the current mean squared error.
 *
 * @param resource $ann Neural network resource.
 */
function fann_get_MSE(mixed $ann): float {}

/**
 * Resets accumulated error metrics.
 *
 * @param resource $ann Neural network resource.
 */
function fann_reset_MSE(mixed $ann): bool {}

/**
 * Returns the current bit-fail count.
 *
 * @param resource $ann Neural network resource.
 */
function fann_get_bit_fail(mixed $ann): int {}

/**
 * Sets the training algorithm.
 *
 * @param resource $ann Neural network resource.
 * @param int $training_algorithm One of `FANN_TRAIN_*`.
 */
function fann_set_training_algorithm(mixed $ann, int $training_algorithm): bool {}

/**
 * Returns the current training algorithm.
 *
 * @param resource $ann Neural network resource.
 */
function fann_get_training_algorithm(mixed $ann): int {}

/**
 * Sets the learning rate.
 *
 * @param resource $ann Neural network resource.
 * @param float $learning_rate Learning rate value.
 */
function fann_set_learning_rate(mixed $ann, float $learning_rate): bool {}

/**
 * Returns the current learning rate.
 *
 * @param resource $ann Neural network resource.
 */
function fann_get_learning_rate(mixed $ann): float {}

/**
 * Sets activation function for all hidden neurons.
 *
 * @param resource $ann Neural network resource.
 * @param int $activation_function One of `FANN_*` activation constants.
 */
function fann_set_activation_function_hidden(mixed $ann, int $activation_function): bool {}

/**
 * Sets activation function for all output neurons.
 *
 * @param resource $ann Neural network resource.
 * @param int $activation_function One of `FANN_*` activation constants.
 */
function fann_set_activation_function_output(mixed $ann, int $activation_function): bool {}

/** @param resource $ann */
function fann_set_activation_function(mixed $ann, int $activation_function, int $layer, int $neuron): bool {}
/** @param resource $ann */
function fann_set_activation_function_layer(mixed $ann, int $activation_function, int $layer): bool {}

/**
 * Returns activation function for a specific neuron.
 *
 * @param resource $ann Neural network resource.
 * @param int $layer Layer index.
 * @param int $neuron Neuron index in layer.
 */
function fann_get_activation_function(mixed $ann, int $layer, int $neuron): int {}
/** @param resource $ann */
function fann_get_activation_steepness(mixed $ann, int $layer, int $neuron): float {}
/** @param resource $ann */
function fann_set_activation_steepness(mixed $ann, float $steepness, int $layer, int $neuron): bool {}
/** @param resource $ann */
function fann_set_activation_steepness_layer(mixed $ann, float $steepness, int $layer): bool {}
/** @param resource $ann */
function fann_set_activation_steepness_hidden(mixed $ann, float $steepness): bool {}
/** @param resource $ann */
function fann_set_activation_steepness_output(mixed $ann, float $steepness): bool {}

/** @param resource $ann */
function fann_get_num_input(mixed $ann): int {}

/** @param resource $ann */
function fann_get_num_output(mixed $ann): int {}

/** @param resource $ann */
function fann_get_total_neurons(mixed $ann): int {}

/** @param resource $ann */
function fann_get_total_connections(mixed $ann): int {}

/** @param resource $ann */
function fann_get_network_type(mixed $ann): int {}

/** @param resource $ann */
function fann_get_connection_rate(mixed $ann): float {}

/** @param resource $ann */
function fann_get_num_layers(mixed $ann): int {}

/** @param resource $ann @return array */
function fann_get_layer_array(mixed $ann): array {}

/** @param resource $ann @return array */
function fann_get_bias_array(mixed $ann): array {}

/**
 * @param resource $ann
 * @return array
 */
function fann_get_connection_array(mixed $ann): array {}

/**
 * @param resource $ann
 * @param array $connections
 */
function fann_set_weight_array(mixed $ann, array $connections): bool {}

/** @param resource $ann */
function fann_set_weight(mixed $ann, int $from_neuron, int $to_neuron, float $weight): bool {}

/** @param resource $ann */
function fann_randomize_weights(mixed $ann, float $min_weight, float $max_weight): bool {}

/** @param resource $ann @param resource $train_data */
function fann_init_weights(mixed $ann, mixed $train_data): bool {}

/** @param resource $ann */
function fann_print_connections(mixed $ann): void {}

/** @param resource $ann */
function fann_get_learning_momentum(mixed $ann): float {}
/** @param resource $ann */
function fann_set_learning_momentum(mixed $ann, float $learning_momentum): bool {}

/** @param resource $ann */
function fann_get_train_error_function(mixed $ann): int {}
/** @param resource $ann */
function fann_set_train_error_function(mixed $ann, int $train_error_function): bool {}

/** @param resource $ann */
function fann_get_train_stop_function(mixed $ann): int {}
/** @param resource $ann */
function fann_set_train_stop_function(mixed $ann, int $train_stop_function): bool {}

/** @param resource $ann */
function fann_get_bit_fail_limit(mixed $ann): float {}
/** @param resource $ann */
function fann_set_bit_fail_limit(mixed $ann, float $bit_fail_limit): bool {}

/** @param resource $ann */
function fann_get_quickprop_decay(mixed $ann): float {}
/** @param resource $ann */
function fann_set_quickprop_decay(mixed $ann, float $quickprop_decay): bool {}

/** @param resource $ann */
function fann_get_quickprop_mu(mixed $ann): float {}
/** @param resource $ann */
function fann_set_quickprop_mu(mixed $ann, float $quickprop_mu): bool {}

/** @param resource $ann */
function fann_get_rprop_increase_factor(mixed $ann): float {}
/** @param resource $ann */
function fann_set_rprop_increase_factor(mixed $ann, float $rprop_increase_factor): bool {}

/** @param resource $ann */
function fann_get_rprop_decrease_factor(mixed $ann): float {}
/** @param resource $ann */
function fann_set_rprop_decrease_factor(mixed $ann, float $rprop_decrease_factor): bool {}

/** @param resource $ann */
function fann_get_rprop_delta_min(mixed $ann): float {}
/** @param resource $ann */
function fann_set_rprop_delta_min(mixed $ann, float $rprop_delta_min): bool {}

/** @param resource $ann */
function fann_get_rprop_delta_max(mixed $ann): float {}
/** @param resource $ann */
function fann_set_rprop_delta_max(mixed $ann, float $rprop_delta_max): bool {}

/** @param resource $ann */
function fann_get_rprop_delta_zero(mixed $ann): float {}
/** @param resource $ann */
function fann_set_rprop_delta_zero(mixed $ann, float $rprop_delta_zero): bool {}

/** @param resource $ann */
function fann_get_sarprop_weight_decay_shift(mixed $ann): float {}
/** @param resource $ann */
function fann_set_sarprop_weight_decay_shift(mixed $ann, float $sarprop_weight_decay_shift): bool {}

/** @param resource $ann */
function fann_get_sarprop_step_error_threshold_factor(mixed $ann): float {}
/** @param resource $ann */
function fann_set_sarprop_step_error_threshold_factor(mixed $ann, float $sarprop_step_error_threshold_factor): bool {}

/** @param resource $ann */
function fann_get_sarprop_step_error_shift(mixed $ann): float {}
/** @param resource $ann */
function fann_set_sarprop_step_error_shift(mixed $ann, float $sarprop_step_error_shift): bool {}

/** @param resource $ann */
function fann_get_sarprop_temperature(mixed $ann): float {}
/** @param resource $ann */
function fann_set_sarprop_temperature(mixed $ann, float $sarprop_temperature): bool {}

/** @param resource $train_data */
function fann_shuffle_train_data(mixed $train_data): bool {}
/** @param resource $train_data */
function fann_scale_input_train_data(mixed $train_data, float $new_min, float $new_max): bool {}
/** @param resource $train_data */
function fann_scale_output_train_data(mixed $train_data, float $new_min, float $new_max): bool {}
/** @param resource $train_data */
function fann_scale_train_data(mixed $train_data, float $new_min, float $new_max): bool {}

/** @param resource $data1 @param resource $data2 @return resource|false */
function fann_merge_train_data(mixed $data1, mixed $data2): mixed {}
/** @param resource $train_data @return resource|false */
function fann_duplicate_train_data(mixed $train_data): mixed {}
/** @param resource $train_data @return resource|false */
function fann_subset_train_data(mixed $train_data, int $pos, int $length): mixed {}

/** @param resource $train_data */
function fann_length_train_data(mixed $train_data): int {}
/** @param resource $train_data */
function fann_num_input_train_data(mixed $train_data): int {}
/** @param resource $train_data */
function fann_num_output_train_data(mixed $train_data): int {}

/** @param resource $ann @param resource $train_data */
function fann_train_epoch(mixed $ann, mixed $train_data): float {}
/** @param resource $ann @param resource $train_data */
function fann_test_data(mixed $ann, mixed $train_data): float {}

/** @param resource $ann @param resource $train_data */
function fann_scale_train(mixed $ann, mixed $train_data): bool {}
/** @param resource $ann @param resource $train_data */
function fann_descale_train(mixed $ann, mixed $train_data): bool {}

/** @param resource $ann @param resource $train_data */
function fann_set_input_scaling_params(mixed $ann, mixed $train_data, float $new_input_min, float $new_input_max): bool {}
/** @param resource $ann @param resource $train_data */
function fann_set_output_scaling_params(mixed $ann, mixed $train_data, float $new_output_min, float $new_output_max): bool {}
/** @param resource $ann @param resource $train_data */
function fann_set_scaling_params(mixed $ann, mixed $train_data, float $new_input_min, float $new_input_max, float $new_output_min, float $new_output_max): bool {}
/** @param resource $ann */
function fann_clear_scaling_params(mixed $ann): bool {}

/** @param resource $ann @return array */
function fann_scale_input(mixed $ann, array $input_vector): array {}
/** @param resource $ann @return array */
function fann_scale_output(mixed $ann, array $output_vector): array {}
/** @param resource $ann @return array */
function fann_descale_input(mixed $ann, array $input_vector): array {}
/** @param resource $ann @return array */
function fann_descale_output(mixed $ann, array $output_vector): array {}

/** @param resource $ann @param resource $train_data */
function fann_cascadetrain_on_data(mixed $ann, mixed $train_data, int $max_neurons, int $neurons_between_reports, float $desired_error): bool {}
/** @param resource $ann */
function fann_cascadetrain_on_file(mixed $ann, string $filename, int $max_neurons, int $neurons_between_reports, float $desired_error): bool {}

/** @param resource $ann */
function fann_get_cascade_output_change_fraction(mixed $ann): float {}
/** @param resource $ann */
function fann_set_cascade_output_change_fraction(mixed $ann, float $cascade_output_change_fraction): bool {}
/** @param resource $ann */
function fann_get_cascade_output_stagnation_epochs(mixed $ann): int {}
/** @param resource $ann */
function fann_set_cascade_output_stagnation_epochs(mixed $ann, int $cascade_output_stagnation_epochs): bool {}

/** @param resource $ann */
function fann_get_cascade_candidate_change_fraction(mixed $ann): float {}
/** @param resource $ann */
function fann_set_cascade_candidate_change_fraction(mixed $ann, float $cascade_candidate_change_fraction): bool {}
/** @param resource $ann */
function fann_get_cascade_candidate_stagnation_epochs(mixed $ann): int {}
/** @param resource $ann */
function fann_set_cascade_candidate_stagnation_epochs(mixed $ann, int $cascade_candidate_stagnation_epochs): bool {}

/** @param resource $ann */
function fann_get_cascade_weight_multiplier(mixed $ann): float {}
/** @param resource $ann */
function fann_set_cascade_weight_multiplier(mixed $ann, float $cascade_weight_multiplier): bool {}
/** @param resource $ann */
function fann_get_cascade_candidate_limit(mixed $ann): float {}
/** @param resource $ann */
function fann_set_cascade_candidate_limit(mixed $ann, float $cascade_candidate_limit): bool {}

/** @param resource $ann */
function fann_get_cascade_max_out_epochs(mixed $ann): int {}
/** @param resource $ann */
function fann_set_cascade_max_out_epochs(mixed $ann, int $cascade_max_out_epochs): bool {}
/** @param resource $ann */
function fann_get_cascade_min_out_epochs(mixed $ann): int {}
/** @param resource $ann */
function fann_set_cascade_min_out_epochs(mixed $ann, int $cascade_min_out_epochs): bool {}
/** @param resource $ann */
function fann_get_cascade_max_cand_epochs(mixed $ann): int {}
/** @param resource $ann */
function fann_set_cascade_max_cand_epochs(mixed $ann, int $cascade_max_cand_epochs): bool {}
/** @param resource $ann */
function fann_get_cascade_min_cand_epochs(mixed $ann): int {}
/** @param resource $ann */
function fann_set_cascade_min_cand_epochs(mixed $ann, int $cascade_min_cand_epochs): bool {}

/** @param resource $ann */
function fann_get_cascade_num_candidates(mixed $ann): int {}
/** @param resource $ann */
function fann_get_cascade_activation_functions_count(mixed $ann): int {}
/** @param resource $ann @return array */
function fann_get_cascade_activation_functions(mixed $ann): array {}
/** @param resource $ann */
function fann_set_cascade_activation_functions(mixed $ann, array $cascade_activation_functions): bool {}

/** @param resource $ann */
function fann_get_cascade_activation_steepnesses_count(mixed $ann): int {}
/** @param resource $ann @return array */
function fann_get_cascade_activation_steepnesses(mixed $ann): array {}
/** @param resource $ann */
function fann_set_cascade_activation_steepnesses(mixed $ann, array $cascade_activation_steepnesses): bool {}

/** @param resource $ann */
function fann_get_cascade_num_candidate_groups(mixed $ann): int {}
/** @param resource $ann */
function fann_set_cascade_num_candidate_groups(mixed $ann, int $cascade_num_candidate_groups): bool {}

/** @param resource $resource */
function fann_get_errno(mixed $resource): int {}
/** @param resource $resource */
function fann_reset_errno(mixed $resource): bool {}
/** @param resource $resource */
function fann_reset_errstr(mixed $resource): bool {}
/** @param resource $resource */
function fann_get_errstr(mixed $resource): string {}
/** @param resource $resource */
function fann_print_error(mixed $resource): void {}
/** @param resource $resource */
function fann_set_error_log(mixed $resource, ?string $log_file = null): bool {}

/**
 * Incremental backpropagation; weights update after each sample.
 * @var int
 */
const FANN_TRAIN_INCREMENTAL = 0;
/**
 * Batch backpropagation; weights update after full epoch.
 * @var int
 */
const FANN_TRAIN_BATCH = 1;
/**
 * Resilient backpropagation (RPROP).
 * @var int
 */
const FANN_TRAIN_RPROP = 2;
/**
 * Quickprop training algorithm.
 * @var int
 */
const FANN_TRAIN_QUICKPROP = 3;
/**
 * Simulated annealing RPROP (SARPROP).
 * @var int
 */
const FANN_TRAIN_SARPROP = 4;

/**
 * Linear activation.
 * @var int
 */
const FANN_LINEAR = 0;
/**
 * Threshold activation.
 * @var int
 */
const FANN_THRESHOLD = 1;
/**
 * Symmetric threshold activation.
 * @var int
 */
const FANN_THRESHOLD_SYMMETRIC = 2;
/**
 * Sigmoid activation.
 * @var int
 */
const FANN_SIGMOID = 3;
/**
 * Stepwise sigmoid activation.
 * @var int
 */
const FANN_SIGMOID_STEPWISE = 4;
/**
 * Symmetric sigmoid activation.
 * @var int
 */
const FANN_SIGMOID_SYMMETRIC = 5;
/**
 * Stepwise symmetric sigmoid activation.
 * @var int
 */
const FANN_SIGMOID_SYMMETRIC_STEPWISE = 6;
/**
 * Gaussian activation.
 * @var int
 */
const FANN_GAUSSIAN = 7;
/**
 * Symmetric gaussian activation.
 * @var int
 */
const FANN_GAUSSIAN_SYMMETRIC = 8;
/**
 * Stepwise gaussian activation.
 * @var int
 */
const FANN_GAUSSIAN_STEPWISE = 9;
/**
 * Elliot activation.
 * @var int
 */
const FANN_ELLIOT = 10;
/**
 * Symmetric Elliot activation.
 * @var int
 */
const FANN_ELLIOT_SYMMETRIC = 11;
/**
 * Piecewise linear activation.
 * @var int
 */
const FANN_LINEAR_PIECE = 12;
/**
 * Symmetric piecewise linear activation.
 * @var int
 */
const FANN_LINEAR_PIECE_SYMMETRIC = 13;
/**
 * Symmetric sine activation.
 * @var int
 */
const FANN_SIN_SYMMETRIC = 14;
/**
 * Symmetric cosine activation.
 * @var int
 */
const FANN_COS_SYMMETRIC = 15;
/**
 * Sine activation.
 * @var int
 */
const FANN_SIN = 16;
/**
 * Cosine activation.
 * @var int
 */
const FANN_COS = 17;

/**
 * Linear error function.
 * @var int
 */
const FANN_ERRORFUNC_LINEAR = 0;
/**
 * Hyperbolic tangent error function.
 * @var int
 */
const FANN_ERRORFUNC_TANH = 1;

/**
 * Stop by MSE threshold.
 * @var int
 */
const FANN_STOPFUNC_MSE = 0;
/**
 * Stop by bit-fail threshold.
 * @var int
 */
const FANN_STOPFUNC_BIT = 1;

/**
 * Layered network (connections to next layer only).
 * @var int
 */
const FANN_NETTYPE_LAYER = 0;
/**
 * Shortcut network (connections to later layers).
 * @var int
 */
const FANN_NETTYPE_SHORTCUT = 1;

/**
 * No error.
 * @var int
 */
const FANN_E_NO_ERROR = 0;
/**
 * Cannot open configuration file for reading.
 * @var int
 */
const FANN_E_CANT_OPEN_CONFIG_R = 1;
/**
 * Cannot open configuration file for writing.
 * @var int
 */
const FANN_E_CANT_OPEN_CONFIG_W = 2;
/**
 * Configuration version mismatch.
 * @var int
 */
const FANN_E_WRONG_CONFIG_VERSION = 3;
/**
 * Failed to read configuration file.
 * @var int
 */
const FANN_E_CANT_READ_CONFIG = 4;
/**
 * Failed to read neuron definition.
 * @var int
 */
const FANN_E_CANT_READ_NEURON = 5;
/**
 * Failed to read connections.
 * @var int
 */
const FANN_E_CANT_READ_CONNECTIONS = 6;
/**
 * Unexpected number of connections.
 * @var int
 */
const FANN_E_WRONG_NUM_CONNECTIONS = 7;
/**
 * Cannot open training data file for writing.
 * @var int
 */
const FANN_E_CANT_OPEN_TD_W = 8;
/**
 * Cannot open training data file for reading.
 * @var int
 */
const FANN_E_CANT_OPEN_TD_R = 9;
/**
 * Failed to read training data.
 * @var int
 */
const FANN_E_CANT_READ_TD = 10;
/**
 * Memory allocation failed.
 * @var int
 */
const FANN_E_CANT_ALLOCATE_MEM = 11;
/**
 * Invalid activation for training.
 * @var int
 */
const FANN_E_CANT_TRAIN_ACTIVATION = 12;
/**
 * Invalid activation for current operation.
 * @var int
 */
const FANN_E_CANT_USE_ACTIVATION = 13;
/**
 * Incompatible training datasets.
 * @var int
 */
const FANN_E_TRAIN_DATA_MISMATCH = 14;
/**
 * Invalid training algorithm for current operation.
 * @var int
 */
const FANN_E_CANT_USE_TRAIN_ALG = 15;
/**
 * Requested training-data subset is out of range.
 * @var int
 */
const FANN_E_TRAIN_DATA_SUBSET = 16;
/**
 * Index is out of bounds.
 * @var int
 */
const FANN_E_INDEX_OUT_OF_BOUND = 17;
/**
 * Scaling parameters are not present.
 * @var int
 */
const FANN_E_SCALE_NOT_PRESENT = 18;
/**
 * Input count does not match network input neurons.
 * @var int
 */
const FANN_E_INPUT_NO_MATCH = 19;
/**
 * Output count does not match network output neurons.
 * @var int
 */
const FANN_E_OUTPUT_NO_MATCH = 20;
