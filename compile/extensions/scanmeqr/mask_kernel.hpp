#pragma once
// Lane-parallel mask-penalty kernel. This header is compiled once per target
// ISA (see mask_kernel_*.cpp): each translation unit defines SCANME_KERNEL_NS
// and is built with the matching -m flags so the plain loops below vectorize
// for that ISA. mask.cpp picks a kernel at runtime.
#ifndef SCANME_KERNEL_NS
#error "SCANME_KERNEL_NS must be defined before including mask_kernel.hpp"
#endif

#include "mask_common.hpp"
#include <bit>
#include <climits>
#include <cstdlib>
#include <cstring>

namespace scanme {
namespace SCANME_KERNEL_NS {

// ---------------------------------------------------------------------------
// Lane-parallel penalty: all 8 masks evaluated at once.
//
// Layout: R8 holds one 192-bit row for each of the 8 masks, word-major —
// w[word].v[mask]. Every operation below is lane-wise over the 8 masks, so the
// compiler can map it straight onto NEON / AVX2 / AVX-512 vectors without any
// cross-lane shuffles. Horizontal shifts combine adjacent words of the *same*
// lane, which is still lane-wise.
//
// Rules 1 and 3 are computed with bit tricks instead of per-module run-length
// scans:
//   rule 1: for a colour bitset C, C5 = C & C>>1 & ... & C>>4 marks every
//           start of a run of >= 5; a run of length L contributes (L-4) bits
//           to C5 plus one "group start" bit, and (L-4) + 2 == L-2 is exactly
//           nayuki's 3 + (L-5) penalty.
//   rule 3: the unit-width (n = 1) pattern L D L DDD L D L with >= 4 light on
//           at least one side is a fixed 11-bit template matched with shifts;
//           out-of-bounds modules are light (bits beyond `size` are zero, and
//           shifts pull in zeros, so ~R yields light there — matching
//           nayuki's virtual border runs). Wider (n >= 2) patterns need a
//           dark run of >= 6 preceded by >= 2 light modules; those anchors are
//           found bitwise and checked locally with scaled_patterns_at().
// Vertical variants use the rows above/below directly (zero-padded), which
// gives column-parallel evaluation with no transpose.
// ---------------------------------------------------------------------------

constexpr int LANES = KERNEL_LANES;
constexpr int PAD = KERNEL_PAD;

struct V8 { uint64_t v[LANES]; };

// W = number of 64-bit words needed per row: 1 for size <= 64 (v1-v11),
// 2 for size <= 128 (v12-v27), 3 otherwise. Templating on W removes 2/3 of
// the work for the most common (small) symbols.
template <int W>
struct RW { V8 w[W]; };

#define V8_LANE_OP(name, expr)                                        \
    inline V8 name(V8 a, V8 b) noexcept {                      \
        V8 r;                                                         \
        for (int i = 0; i < LANES; ++i) r.v[i] = (expr);              \
        return r;                                                     \
    }
V8_LANE_OP(v_and, a.v[i] & b.v[i])
V8_LANE_OP(v_or,  a.v[i] | b.v[i])
V8_LANE_OP(v_andnot, ~a.v[i] & b.v[i])   // ~a & b
#undef V8_LANE_OP

inline V8 v_not(V8 a) noexcept {
    V8 r;
    for (int i = 0; i < LANES; ++i) r.v[i] = ~a.v[i];
    return r;
}
inline V8 v_bcast(uint64_t x) noexcept {
    V8 r;
    for (int i = 0; i < LANES; ++i) r.v[i] = x;
    return r;
}
inline uint64_t v_reduce_or(V8 a) noexcept {
    uint64_t o = 0;
    for (int i = 0; i < LANES; ++i) o |= a.v[i];
    return o;
}

template <int W> inline RW<W> operator&(const RW<W>& a, const RW<W>& b) noexcept {
    RW<W> r; for (int k = 0; k < W; ++k) r.w[k] = v_and(a.w[k], b.w[k]); return r;
}
template <int W> inline RW<W> operator|(const RW<W>& a, const RW<W>& b) noexcept {
    RW<W> r; for (int k = 0; k < W; ++k) r.w[k] = v_or(a.w[k], b.w[k]); return r;
}
template <int W> inline RW<W> operator~(const RW<W>& a) noexcept {
    RW<W> r; for (int k = 0; k < W; ++k) r.w[k] = v_not(a.w[k]); return r;
}
// ~a & b
template <int W> inline RW<W> andnot(const RW<W>& a, const RW<W>& b) noexcept {
    RW<W> r; for (int k = 0; k < W; ++k) r.w[k] = v_andnot(a.w[k], b.w[k]); return r;
}
template <int W> inline bool any(const RW<W>& a) noexcept {
    V8 o = a.w[0];
    for (int k = 1; k < W; ++k) o = v_or(o, a.w[k]);
    return v_reduce_or(o) != 0;
}

template <int K, int W>
inline RW<W> shr(const RW<W>& r) noexcept {
    static_assert(K > 0 && K < 64);
    RW<W> o;
    for (int k = 0; k < W; ++k) {
        for (int i = 0; i < LANES; ++i) {
            uint64_t hi = (k + 1 < W) ? r.w[k + 1].v[i] : 0;
            o.w[k].v[i] = (r.w[k].v[i] >> K) | (hi << (64 - K));
        }
    }
    return o;
}

template <int K, int W>
inline RW<W> shl(const RW<W>& r) noexcept {
    static_assert(K > 0 && K < 64);
    RW<W> o;
    for (int k = 0; k < W; ++k) {
        for (int i = 0; i < LANES; ++i) {
            uint64_t lo = (k > 0) ? r.w[k - 1].v[i] : 0;
            o.w[k].v[i] = (r.w[k].v[i] << K) | (lo >> (64 - K));
        }
    }
    return o;
}

// acc[m] += popcount(row m)
template <int W>
inline void popcnt_acc(const RW<W>& r, int* acc) noexcept {
    for (int i = 0; i < LANES; ++i) {
        int c = 0;
        for (int k = 0; k < W; ++k) c += std::popcount(r.w[k].v[i]);
        acc[i] += c;
    }
}

template <int W>
inline Row3 lane(const RW<W>& r, int m) noexcept {
    Row3 out = Row3::zero();
    for (int k = 0; k < W; ++k) out.w[k] = r.w[k].v[m];
    return out;
}

template <int W>
inline RW<W> bcast_row(const Row3& r) noexcept {
    RW<W> o;
    for (int k = 0; k < W; ++k) o.w[k] = v_bcast(r.w[k]);
    return o;
}

template <int W>
inline int select_best_mask_impl(const QRMatrix& m, int ecl, int* penalties_out, MaskScratch& scratch) {
    using R8 = RW<W>;
    static_assert(sizeof(R8) * (MAX_QR_SIZE + 2 * PAD) <= sizeof(MaskScratch));
    // Zero padding on both sides so vertical look-ups never branch.
    R8* const rows = reinterpret_cast<R8*>(scratch.words);
    const int size = m.size;
    R8* const P = rows + PAD; // P[y] valid for -PAD <= y < size + PAD

    const Row3 valid_row = mask_low_n(size);
    const R8 valid = bcast_row<W>(valid_row);
    const R8 valid2 = bcast_row<W>(mask_low_n(size - 1));
    const R8 inside_from4[2] = {R8{}, valid};

    // ---- Build the 8 masked matrices (format info included) --------------
    std::memset(rows, 0, sizeof(R8) * PAD);
    std::memset(rows + PAD + size, 0, sizeof(R8) * PAD);
    for (int y = 0; y < size; ++y) {
        const Row3 base = m.rows[y];
        const Row3 not_func = ~m.func[y] & valid_row;
        const int ty = y % MASK_Y_PERIOD;
        for (int mk = 0; mk < LANES; ++mk) {
            Row3 r = base ^ (MASK_TILES[mk][ty] & not_func);
            for (int k = 0; k < W; ++k) P[y].w[k].v[mk] = r.w[k];
        }
    }
    {
        const auto pos = format_positions(size);
        for (int mk = 0; mk < LANES; ++mk) {
            const uint16_t fmt = FORMAT_INFO[ecl * 8 + mk];
            for (const FmtBit& fb : pos) {
                uint64_t& word = P[fb.y].w[fb.x >> 6].v[mk];
                const uint64_t bit = uint64_t(1) << (fb.x & 63);
                if ((fmt >> fb.bit) & 1) word |= bit; else word &= ~bit;
            }
        }
    }

    // ---- Lane-parallel penalty --------------------------------------------
    int r1[LANES] = {}, r2[LANES] = {}, r3[LANES] = {}, dark[LANES] = {};
    R8 prevR = {}, prevS1 = {};    // previous row and its >>1, for rule 2
    R8 prevRunsV = {};             // previous row's vertical run-of-5 marks (dark | light)

    for (int y = 0; y < size; ++y) {
        const R8 R = P[y];
        const R8 S1 = shr<1>(R);
        const R8 S2 = shr<2>(R);
        const R8 D2 = R & S1;                // dark at x, x+1
        const R8 D3 = D2 & S2;               // dark at x..x+2
        const R8 D4 = D2 & shr<2>(D2);       // dark at x..x+3
        const R8 S5 = shr<5>(R);
        const R8 D5 = D4 & shr<4>(R);        // dark at x..x+4
        const R8 D6 = D5 & S5;               // dark at x..x+5
        const R8 D5v = R & P[y-1] & P[y-2] & P[y-3] & P[y-4]; // dark at rows y-4..y

        // Rule 1 (runs of >= 5). Dark and light marks are disjoint, so their
        // popcounts are summed with a single popcount of the OR.
        const R8 L = andnot(R, valid);
        {
            // Horizontal.
            const R8 L2 = L & shr<1>(L);
            const R8 L4 = L2 & shr<2>(L2);
            const R8 L5 = L4 & shr<4>(L);
            const R8 runs = D5 | L5;
            const R8 starts = andnot(shr<1>(runs), runs); // rightmost bit of each run-of-5 group
            popcnt_acc(runs, r1);
            popcnt_acc(starts, r1);
            popcnt_acc(starts, r1);
        }
        {
            // Vertical. Rows above the symbol are not light: zero the light mark while y < 4.
            const R8 L5v = andnot(R | P[y-1] | P[y-2] | P[y-3] | P[y-4], valid) & inside_from4[y >= 4 ? 1 : 0];
            const R8 runs = D5v | L5v;
            const R8 starts = andnot(prevRunsV, runs); // topmost bit of each vertical group
            popcnt_acc(runs, r1);
            popcnt_acc(starts, r1);
            popcnt_acc(starts, r1);
            prevRunsV = runs;
        }

        // Rule 3, horizontal, n = 1. Pattern anchored at x = first dark module:
        //   x-1 light | x dark | x+1 light | x+2..x+4 dark | x+5 light | x+6 dark | x+7 light
        // plus >= 4 light on the left (x-4..x-1) or on the right (x+7..x+10).
        const R8 SL1 = shl<1>(R);
        {
            const R8 core = R & ~S1 & shr<2>(D3) & ~S5 & shr<6>(R) & ~shr<7>(R) & ~SL1;
            const R8 O3l = R | SL1 | shl<2>(R);               // dark at x, x-1, x-2
            const R8 A = andnot(shl<2>(O3l), core);          // light at x-2..x-4
            const R8 O3r = R | S1 | S2;                       // dark at x, x+1, x+2
            const R8 B = andnot(shr<8>(O3r), core);          // light at x+8..x+10
            popcnt_acc(A, r3);
            popcnt_acc(B, r3);
        }

        // Rule 3, vertical, n = 1 (same template down the column).
        {
            const R8 core = R & ~P[y+1] & P[y+2] & P[y+3] & P[y+4] & ~P[y+5] & P[y+6] & ~P[y+7] & ~P[y-1];
            const R8 A = andnot(P[y-2] | P[y-3] | P[y-4], core);
            const R8 B = andnot(P[y+8] | P[y+9] | P[y+10], core);
            popcnt_acc(A, r3);
            popcnt_acc(B, r3);
        }

        // Rule 3, n >= 2. Every such pattern has a dark run of exactly 3n >= 6
        // preceded by a light run of exactly n >= 2, so anchor on "run of >= 6
        // starting at x with x-1, x-2 light" and verify locally.
        {
            const R8 anchors = andnot(shl<1>(R | SL1), D6);
            if (any(anchors)) {
                for (int mk = 0; mk < LANES; ++mk) {
                    const Row3 a = lane(anchors, mk);
                    if (is_zero(a)) continue;
                    const Row3 row = lane(R, mk);
                    auto get = [&](int x) { return ((row.w[x >> 6] >> (x & 63)) & 1) != 0; };
                    for (int word = 0; word < W; ++word) {
                        uint64_t bits = a.w[word];
                        while (bits) {
                            r3[mk] += scaled_patterns_at(get, size, word * 64 + std::countr_zero(bits));
                            bits &= bits - 1;
                        }
                    }
                }
            }
            // Vertical: run of >= 6 ending at row y, i.e. starting at y-5.
            const R8 D6v = D5v & P[y-5];
            if (any(D6v)) {
                const R8 anchorsV = andnot(P[y-6] | P[y-7], D6v);
                for (int mk = 0; mk < LANES; ++mk) {
                    const Row3 a = lane(anchorsV, mk);
                    if (is_zero(a)) continue;
                    for (int word = 0; word < W; ++word) {
                        uint64_t bits = a.w[word];
                        while (bits) {
                            const int x = word * 64 + std::countr_zero(bits);
                            bits &= bits - 1;
                            const uint64_t bit = uint64_t(1) << (x & 63);
                            auto get = [&](int yy) { return (P[yy].w[word].v[mk] & bit) != 0; };
                            r3[mk] += scaled_patterns_at(get, size, y - 5);
                        }
                    }
                }
            }
        }

        // Rule 2: 2x2 blocks between rows y-1 and y.
        if (y > 0) {
            const R8 all_dark  = prevR & prevS1 & R & S1 & valid2;
            const R8 all_light = andnot(prevR | prevS1 | R | S1, valid2);
            popcnt_acc(all_dark | all_light, r2); // disjoint
        }
        prevR = R;
        prevS1 = S1;

        // Rule 4: dark modules.
        popcnt_acc(R, dark);
    }

    // ---- Totals ------------------------------------------------------------
    const int total = size * size;
    int best_mask = 0;
    int best_penalty = INT_MAX;
    for (int mk = 0; mk < LANES; ++mk) {
        const int k = static_cast<int>((std::abs(dark[mk] * 20L - total * 10L) + total - 1) / total) - 1;
        const int p = r1[mk] + r2[mk] * 3 + r3[mk] * 40 + k * 10;
        if (penalties_out) penalties_out[mk] = p;
        if (p < best_penalty) {
            best_penalty = p;
            best_mask = mk;
        }
    }
    return best_mask;
}

int select_best_mask_kernel(const QRMatrix& m, int ecl, int* penalties_out, MaskScratch& scratch) {
    if (m.size <= 64)  return select_best_mask_impl<1>(m, ecl, penalties_out, scratch);
    if (m.size <= 128) return select_best_mask_impl<2>(m, ecl, penalties_out, scratch);
    return select_best_mask_impl<3>(m, ecl, penalties_out, scratch);
}

} // namespace SCANME_KERNEL_NS
} // namespace scanme
