#pragma once
#include "simd/row3.hpp"
#include <cstdint>
#include <cstring>

namespace scanme {

static constexpr int MAX_QR_SIZE = 177;

struct QRMatrix {
    int size;
    int version;
    Row3 rows[MAX_QR_SIZE];
    Row3 func[MAX_QR_SIZE];

    explicit QRMatrix(int version_) { reset(version_); }

    // Re-initialise for `version_` (clears only the rows that matter).
    void reset(int version_) noexcept {
        version = version_;
        size = 17 + version_ * 4;
        std::memset(rows, 0, sizeof(Row3) * static_cast<size_t>(size));
        std::memset(func, 0, sizeof(Row3) * static_cast<size_t>(size));
    }

    void set_module(int x, int y, bool dark) noexcept {
        uint64_t bit = uint64_t(1) << (x & 63);
        int word = x >> 6;
        if (dark) rows[y].w[word] |= bit;
        else      rows[y].w[word] &= ~bit;
    }

    bool get_module(int x, int y) const noexcept {
        return (rows[y].w[x >> 6] >> (x & 63)) & 1;
    }

    void set_function(int x, int y, bool dark) noexcept {
        set_module(x, y, dark);
        func[y].w[x >> 6] |= uint64_t(1) << (x & 63);
    }

    bool is_function(int x, int y) const noexcept {
        return (func[y].w[x >> 6] >> (x & 63)) & 1;
    }

    void mark_function(int x, int y) noexcept {
        func[y].w[x >> 6] |= uint64_t(1) << (x & 63);
    }
};

void place_finder_patterns(QRMatrix& m);
void place_alignment_patterns(QRMatrix& m);
void place_timing_patterns(QRMatrix& m);
void place_dark_module(QRMatrix& m);
void reserve_format_info(QRMatrix& m);
void place_format_info(QRMatrix& m, int ecl, int mask);
void place_version_info(QRMatrix& m);
void place_data(QRMatrix& m, const uint8_t* data, int data_len);

} // namespace scanme
