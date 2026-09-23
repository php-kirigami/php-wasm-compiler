#pragma once
#include "matrix.hpp"
#include <cstdint>
#include <vector>

namespace scanme {

enum class ECL { L = 0, M = 1, Q = 2, H = 3 };

struct EncodeResult {
    QRMatrix matrix;
    int version;
};

EncodeResult encode(const char* data, size_t len, ECL ecl);

// Encode into a caller-owned matrix (avoids copying the ~13 KB QRMatrix).
// Returns the version. Throws std::invalid_argument on bad input.
int encode_into(QRMatrix& m, const char* data, size_t len, ECL ecl);

// Expand the matrix into one byte per module (0/1), row-major. `out` must
// have room for size*size + 8 bytes (the expansion writes 8 modules at a time
// and may spill up to 7 bytes past the last row).
void matrix_to_bytes(const QRMatrix& m, uint8_t* out);
EncodeResult encode_forced_mask(const char* data, size_t len, ECL ecl, int mask);
EncodeResult encode_for_debug(const char* data, size_t len, ECL ecl, int penalties_out[8]);

// Per-stage timings (microseconds) — used by bench/ to attribute cost.
struct EncodeStages {
    double codewords_us = 0;
    double rs_us = 0;
    double place_us = 0;
    double select_mask_us = 0;
    double apply_us = 0;
};
// Everything up to (but excluding) mask selection — for tests/bench.
QRMatrix build_unmasked_matrix(const char* data, size_t len, ECL ecl);

EncodeResult encode_timed(const char* data, size_t len, ECL ecl, EncodeStages& stages);

} // namespace scanme
