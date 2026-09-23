#include "mask.hpp"
#include "mask_common.hpp"
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace scanme {

Row3 build_mask_row(int mask_id, int y, int size, const Row3& func_row) {
    return MASK_TILES[mask_id][y % MASK_Y_PERIOD] & ~func_row & mask_low_n(size);
}

void apply_mask(QRMatrix& m, int mask_id) {
    for (int y = 0; y < m.size; ++y) {
        Row3 mask_row = build_mask_row(mask_id, y, m.size, m.func[y]);
        m.rows[y] = m.rows[y] ^ mask_row;
    }
}

// ---------------------------------------------------------------------------
// Runtime kernel selection. The kernels are identical C++ compiled for
// different ISAs; on x86-64 we pick the widest one the CPU (and OS) support.
// ---------------------------------------------------------------------------

namespace generic { int select_best_mask_kernel(const QRMatrix&, int, int*, MaskScratch&); }
#if defined(SCANME_HAVE_AVX2_KERNEL)
namespace avx2   { int select_best_mask_kernel(const QRMatrix&, int, int*, MaskScratch&); }
#endif
#if defined(SCANME_HAVE_AVX512_KERNEL)
namespace avx512 { int select_best_mask_kernel(const QRMatrix&, int, int*, MaskScratch&); }
#endif

using KernelFn = int (*)(const QRMatrix&, int, int*, MaskScratch&);

static KernelFn pick_kernel() {
    // Test/bench override: SCANME_MASK_KERNEL=generic|avx2|avx512. No CPU check
    // is done for an explicit override — forcing an unsupported ISA will SIGILL,
    // so callers must gate on mask_kernel_supported() first.
    const char* forced = std::getenv("SCANME_MASK_KERNEL");
    if (forced && std::strcmp(forced, "generic") == 0) return &generic::select_best_mask_kernel;
#if defined(SCANME_HAVE_AVX512_KERNEL)
    if (forced && std::strcmp(forced, "avx512") == 0) return &avx512::select_best_mask_kernel;
#endif
#if defined(SCANME_HAVE_AVX2_KERNEL)
    if (forced && std::strcmp(forced, "avx2") == 0) return &avx2::select_best_mask_kernel;
#endif
#if (defined(SCANME_HAVE_AVX2_KERNEL) || defined(SCANME_HAVE_AVX512_KERNEL)) && (defined(__GNUC__) || defined(__clang__))
    __builtin_cpu_init();
#if defined(SCANME_HAVE_AVX512_KERNEL)
    if (__builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512bw") &&
        __builtin_cpu_supports("avx512vpopcntdq") && __builtin_cpu_supports("bmi2"))
        return &avx512::select_best_mask_kernel;
#endif
#if defined(SCANME_HAVE_AVX2_KERNEL)
    if (__builtin_cpu_supports("avx2") && __builtin_cpu_supports("popcnt") &&
        __builtin_cpu_supports("bmi2"))
        return &avx2::select_best_mask_kernel;
#endif
#endif
    return &generic::select_best_mask_kernel;
}

bool mask_kernel_supported(const char* name) {
    if (!name) return false;
    if (std::strcmp(name, "generic") == 0) return true;
#if (defined(SCANME_HAVE_AVX2_KERNEL) || defined(SCANME_HAVE_AVX512_KERNEL)) && (defined(__GNUC__) || defined(__clang__))
    __builtin_cpu_init();
#if defined(SCANME_HAVE_AVX512_KERNEL)
    if (std::strcmp(name, "avx512") == 0)
        return __builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512bw") &&
               __builtin_cpu_supports("avx512vpopcntdq") && __builtin_cpu_supports("bmi2");
#endif
#if defined(SCANME_HAVE_AVX2_KERNEL)
    if (std::strcmp(name, "avx2") == 0)
        return __builtin_cpu_supports("avx2") && __builtin_cpu_supports("popcnt") &&
               __builtin_cpu_supports("bmi2");
#endif
#endif
    return false;
}

const char* active_mask_kernel() {
    const KernelFn fn = pick_kernel();
#if defined(SCANME_HAVE_AVX512_KERNEL)
    if (fn == &avx512::select_best_mask_kernel) return "avx512";
#endif
#if defined(SCANME_HAVE_AVX2_KERNEL)
    if (fn == &avx2::select_best_mask_kernel) return "avx2";
#endif
    (void)fn;
    return "generic";
}

int select_best_mask(QRMatrix& m, int ecl, int* penalties_out) {
    static const KernelFn fn = pick_kernel();
    static thread_local MaskScratch scratch;
    return fn(m, ecl, penalties_out, scratch);
}

// ---------------------------------------------------------------------------
// Scalar reference (nayuki-style). Kept as the oracle for tests and bench.
// ---------------------------------------------------------------------------

int calculate_penalty_scalar(const Row3* masked_rows, int size, int* rule_out) {
    int dark_count = 0;
    int r1_pen = 0, r2_pen = 0, r3_pen = 0, r4_pen = 0;

    auto getModule = [&](int x, int y) -> bool {
        return (masked_rows[y].w[x >> 6] >> (x & 63)) & 1;
    };

    for (int y = 0; y < size; ++y) {
        bool runColor = false;
        int runX = 0;
        for (int x = 0; x < size; ++x) {
            if (getModule(x, y) == runColor) {
                runX++;
                if (runX == 5) r1_pen += 3;
                else if (runX > 5) r1_pen++;
            } else {
                runColor = getModule(x, y);
                runX = 1;
            }
        }
        r3_pen += finder_patterns_on_line([&](int x) { return getModule(x, y); }, size, 1) * 40;
    }

    for (int x = 0; x < size; ++x) {
        bool runColor = false;
        int runY = 0;
        for (int y = 0; y < size; ++y) {
            if (getModule(x, y) == runColor) {
                runY++;
                if (runY == 5) r1_pen += 3;
                else if (runY > 5) r1_pen++;
            } else {
                runColor = getModule(x, y);
                runY = 1;
            }
        }
        r3_pen += finder_patterns_on_line([&](int y) { return getModule(x, y); }, size, 1) * 40;
    }

    {
        Row3 valid_r2 = mask_low_n(size - 1);
        for (int y = 0; y < size - 1; ++y) {
            Row3 cur  = masked_rows[y];
            Row3 next = masked_rows[y + 1];
            Row3 cur_s  = shr1(cur);
            Row3 next_s = shr1(next);
            Row3 all_dark  = cur & cur_s & next & next_s & valid_r2;
            Row3 all_light = ~cur & ~cur_s & ~next & ~next_s & valid_r2;
            r2_pen += (popcnt(all_dark) + popcnt(all_light)) * 3;
        }
    }

    for (int y = 0; y < size; ++y)
        dark_count += popcnt(masked_rows[y] & mask_low_n(size));
    int total = size * size;
    int k = static_cast<int>((std::abs(dark_count * 20L - total * 10L) + total - 1) / total) - 1;
    r4_pen = k * 10;

    if (rule_out) {
        rule_out[0] = r1_pen;
        rule_out[1] = r2_pen;
        rule_out[2] = r3_pen;
        rule_out[3] = r4_pen;
        rule_out[4] = dark_count;
    }
    return r1_pen + r2_pen + r3_pen + r4_pen;
}

// Reference implementation of select_best_mask using the scalar penalty and
// the matrix.cpp format-info placement (independent of format_positions()),
// so tests cross-check both the penalty math and the format-bit layout.
int select_best_mask_reference(const QRMatrix& m, int ecl, int* penalties_out) {
    static thread_local Row3 masked[MAX_QR_SIZE];
    const int size = m.size;
    int best_mask = 0, best_penalty = INT_MAX;
    for (int mk = 0; mk < 8; ++mk) {
        QRMatrix copy = m;
        place_format_info(copy, ecl, mk); // format modules are function modules: untouched by the mask
        for (int y = 0; y < size; ++y)
            masked[y] = copy.rows[y] ^ build_mask_row(mk, y, size, copy.func[y]);
        int p = calculate_penalty_scalar(masked, size, nullptr);
        if (penalties_out) penalties_out[mk] = p;
        if (p < best_penalty) { best_penalty = p; best_mask = mk; }
    }
    return best_mask;
}

} // namespace scanme
