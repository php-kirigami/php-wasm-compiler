<?php
// Exercised by compile/check-shared-extension-symbols.mjs. Trains the XOR
// network (the whole vendored libfann: creation, incremental training, MSE,
// running), then saves it and loads it back, which goes through libfann's
// file I/O.

function fail(string $message): never {
	echo $message, "\n";
	exit(1);
}

if (!extension_loaded('fann')) {
	fail('fann not loaded');
}

$ann = fann_create_standard(3, 2, 3, 1);
if (fann_get_num_layers($ann) !== 3 || fann_get_num_input($ann) !== 2 || fann_get_num_output($ann) !== 1) {
	fail('network topology');
}
fann_set_training_algorithm($ann, FANN_TRAIN_INCREMENTAL);
fann_set_learning_rate($ann, 0.7);
fann_set_activation_function_hidden($ann, FANN_SIGMOID_SYMMETRIC);
fann_set_activation_function_output($ann, FANN_SIGMOID_SYMMETRIC);

$xor = [
	[[-1.0, -1.0], [-1.0]],
	[[-1.0, 1.0], [1.0]],
	[[1.0, -1.0], [1.0]],
	[[1.0, 1.0], [-1.0]],
];
$epoch_mse = function () use ($ann, $xor): float {
	fann_reset_MSE($ann);
	foreach ($xor as [$in, $out]) {
		fann_train($ann, $in, $out);
	}
	return fann_get_MSE($ann);
};

$first = $epoch_mse();
$last = $first;
for ($epoch = 0; $epoch < 3000 && $last > 0.001; $epoch++) {
	$last = $epoch_mse();
}
if (!($last < $first)) {
	fail("training did not reduce the error: $first -> $last");
}

$path = tempnam(sys_get_temp_dir(), 'fann') . '.net';
if (!fann_save($ann, $path)) {
	fail('fann_save');
}
$loaded = fann_create_from_file($path);
unlink($path);
if ($loaded === false) {
	fail('fann_create_from_file');
}
$a = fann_run($ann, [-1.0, 1.0]);
$b = fann_run($loaded, [-1.0, 1.0]);
if (!is_array($a) || count($a) !== 1 || abs($a[0] - $b[0]) > 1e-6) {
	fail('saved network does not match: ' . json_encode([$a, $b]));
}

fann_destroy($ann);
fann_destroy($loaded);

echo "fann smoke test OK\n";
