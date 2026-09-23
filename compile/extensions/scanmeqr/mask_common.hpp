#pragma once
// Pieces shared by mask.cpp (dispatch, reference) and the per-ISA penalty
// kernels in mask_kernel.hpp.
#include "mask.hpp"
#include "tables.hpp"
#include <algorithm>
#include <array>
#include <cstdint>

namespace scanme {

inline constexpr int MASK_Y_PERIOD = 12;

inline const auto MASK_TILES = []() {
    std::array<std::array<Row3, MASK_Y_PERIOD>, 8> t{};
    for (int m = 0; m < 8; ++m) {
        for (int y = 0; y < MASK_Y_PERIOD; ++y) {
            Row3 r = Row3::zero();
            for (int x = 0; x < 192; ++x) {
                if (mask_condition(m, x, y))
                    r.w[x >> 6] |= uint64_t(1) << (x & 63);
            }
            t[m][y] = r;
        }
    }
    return t;
}();

// ---------------------------------------------------------------------------
// Format-info placement (15 bits, mirrored into two locations). Same for every
// mask except the bit values, so we keep the positions once and the values per
// mask.
// ---------------------------------------------------------------------------

struct FmtBit { uint8_t x, y, bit; };
inline constexpr int FMT_BITS = 30; // 15 bits, each placed twice

inline constexpr std::array<FmtBit, FMT_BITS> format_positions(int n) {
    std::array<FmtBit, FMT_BITS> p{};
    int k = 0;
    const uint8_t col8_rows[] = {0, 1, 2, 3, 4, 5, 7, 8};
    for (int i = 0; i < 8; ++i) p[k++] = {8, col8_rows[i], static_cast<uint8_t>(i)};
    const uint8_t row8_cols[] = {7, 5, 4, 3, 2, 1, 0};
    for (int i = 0; i < 7; ++i) p[k++] = {row8_cols[i], 8, static_cast<uint8_t>(8 + i)};
    for (int i = 0; i < 8; ++i)
        p[k++] = {static_cast<uint8_t>(n - 1 - i), 8, static_cast<uint8_t>(i)};
    for (int i = 8; i < 15; ++i)
        p[k++] = {8, static_cast<uint8_t>(n - 15 + i), static_cast<uint8_t>(i)};
    return p;
}

// ---------------------------------------------------------------------------
// nayuki-style run-history helpers for rule 3 (finder-like patterns).
// ---------------------------------------------------------------------------

inline void finderPenaltyAddHistory(int runLen, std::array<int,7>& hist, int size) {
    if (hist[0] == 0)
        runLen += size;
    std::copy_backward(hist.begin(), hist.end() - 1, hist.end());
    hist[0] = runLen;
}

inline int finderPenaltyCountPatterns(const std::array<int,7>& hist, int min_n) {
    int n = hist[1];
    if (n < min_n) return 0;
    bool core = hist[2] == n && hist[3] == n * 3 && hist[4] == n && hist[5] == n;
    return (core && hist[0] >= n * 4 && hist[6] >= n ? 1 : 0)
         + (core && hist[6] >= n * 4 && hist[0] >= n ? 1 : 0);
}

inline int finderPenaltyTerminateAndCount(bool curColor, int curLen, std::array<int,7>& hist, int size, int min_n) {
    if (curColor) {
        finderPenaltyAddHistory(curLen, hist, size);
        curLen = 0;
    }
    curLen += size;
    finderPenaltyAddHistory(curLen, hist, size);
    return finderPenaltyCountPatterns(hist, min_n);
}

// Count 1:1:3:1:1 finder-like patterns along one line of `size` modules,
// where get(i) yields the module colour. Only patterns whose unit width n is
// >= min_n are counted. Returns the number of patterns (not yet * 40).
template <typename Get>
inline int finder_patterns_on_line(Get get, int size, int min_n) {
    int count = 0;
    bool runColor = false;
    int run = 0;
    std::array<int,7> hist = {};
    for (int i = 0; i < size; ++i) {
        bool c = get(i);
        if (c == runColor) {
            run++;
        } else {
            finderPenaltyAddHistory(run, hist, size);
            if (!runColor)
                count += finderPenaltyCountPatterns(hist, min_n);
            runColor = c;
            run = 1;
        }
    }
    count += finderPenaltyTerminateAndCount(runColor, run, hist, size, min_n);
    return count;
}

// Counts the scaled (n >= 2) finder-like patterns whose central 3n dark run
// starts at `x` on a line of `size` modules read through get(i). Mirrors
// nayuki's run-history test exactly: inner runs must have exact lengths, the
// two outer light runs must be >= 4n on one side and >= n on the other, and
// the border behaves like an arbitrarily long light run.
template <typename Get>
inline int scaled_patterns_at(Get get, int size, int x) {
    // Light run immediately left of the dark run: must be exactly n >= 2 and
    // must not touch the border (a border-touching run is n + size for nayuki).
    int i = x - 1, n = 0;
    while (i >= 0 && !get(i)) { ++n; --i; }
    if (i < 0 || n < 2) return 0;
    // Dark run of exactly 3n starting at x.
    int d = 0; i = x;
    while (i < size && get(i) && d <= 3 * n) { ++d; ++i; }
    if (d != 3 * n) return 0;
    // Light run of exactly n on the right (border-touching => not exact).
    int nr = 0;
    while (i < size && !get(i) && nr <= n) { ++nr; ++i; }
    if (i >= size || nr != n) return 0;
    // Dark run of exactly n on the right.
    int dr = 0;
    while (i < size && get(i) && dr <= n) { ++dr; ++i; }
    if (dr != n) return 0;
    // Outer right light run, capped at 4n; border counts as >= 4n.
    int outer_r = 0;
    while (i < size && !get(i) && outer_r < 4 * n) { ++outer_r; ++i; }
    if (i >= size) outer_r = 4 * n;
    // Dark run of exactly n on the left (may touch the border).
    int j = x - n - 1, dl = 0;
    while (j >= 0 && get(j) && dl <= n) { ++dl; --j; }
    if (dl != n) return 0;
    // Outer left light run, capped at 4n; border counts as >= 4n.
    int outer_l = 0;
    while (j >= 0 && !get(j) && outer_l < 4 * n) { ++outer_l; --j; }
    if (j < 0) outer_l = 4 * n;
    return (outer_l >= 4 * n && outer_r >= n ? 1 : 0)
         + (outer_r >= 4 * n && outer_l >= n ? 1 : 0);
}

// Scratch memory for the lane-parallel kernels: enough for the widest row
// layout (3 words x 8 lanes) with zero padding on both sides. One instance per
// thread lives in mask.cpp and is handed to whichever kernel is selected.
inline constexpr int KERNEL_LANES = 8;
inline constexpr int KERNEL_PAD = 12; // >= 11 rows of look-ahead/behind for rule 3
struct alignas(64) MaskScratch {
    uint64_t words[(MAX_QR_SIZE + 2 * KERNEL_PAD) * 3 * KERNEL_LANES];
};

} // namespace scanme
