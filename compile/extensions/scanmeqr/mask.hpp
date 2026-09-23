#pragma once
#include "matrix.hpp"

namespace scanme {

void apply_mask(QRMatrix& m, int mask_id);

// Evaluates all 8 masks (lane-parallel) and returns the one with the lowest
// penalty. Optionally writes the 8 penalties.
int select_best_mask(QRMatrix& m, int ecl, int* penalties_out = nullptr);

// Name of the penalty kernel selected for this CPU ("generic", "avx2", "avx512").
const char* active_mask_kernel();

// Whether a named kernel ("generic", "avx2", "avx512") is both compiled into
// this build and runnable on this CPU. SCANME_MASK_KERNEL forces a kernel
// without any CPU check (forcing an unsupported ISA SIGILLs), so tests and
// benchmarks must ask this before forcing one.
bool mask_kernel_supported(const char* name);

// Scalar reference implementation (nayuki-style); used by tests/bench.
int select_best_mask_reference(const QRMatrix& m, int ecl, int* penalties_out = nullptr);
int calculate_penalty_scalar(const Row3* masked_rows, int size, int* rule_out = nullptr);

inline bool mask_condition(int mask_id, int x, int y) noexcept {
    switch (mask_id) {
        case 0: return (x + y) % 2 == 0;
        case 1: return y % 2 == 0;
        case 2: return x % 3 == 0;
        case 3: return (x + y) % 3 == 0;
        case 4: return (y / 2 + x / 3) % 2 == 0;
        case 5: return (x * y) % 2 + (x * y) % 3 == 0;
        case 6: return ((x * y) % 2 + (x * y) % 3) % 2 == 0;
        case 7: return ((x + y) % 2 + (x * y) % 3) % 2 == 0;
        default: return false;
    }
}

Row3 build_mask_row(int mask_id, int y, int size, const Row3& func_row);

} // namespace scanme
