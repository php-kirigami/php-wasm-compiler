#include "encoder.hpp"
#include "reed_solomon.hpp"
#include "matrix.hpp"
#include "mask.hpp"
#include "tables.hpp"
#include "scanme_qr.h"
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <array>
#include <chrono>

namespace scanme {

// Bounds from the QR spec (v40-H has 81 blocks; max ECC per block is 30).
static constexpr int MAX_RS_BLOCKS = 81;
static constexpr int MAX_RS_EC = 30;
static_assert([] {
    int blocks = 0, ec = 0;
    for (const auto& row : EC_TABLE)
        for (const auto& e : row) {
            blocks = std::max(blocks, e.g1_blocks + e.g2_blocks);
            ec = std::max(ec, e.ec_per_block);
        }
    return blocks <= MAX_RS_BLOCKS && ec <= MAX_RS_EC;
}(), "RS block bounds too small for EC_TABLE");

static int find_version(size_t len, int ecl) {
    for (int v = 1; v <= 40; ++v) {
        if (static_cast<int>(len) <= BYTE_CAPACITY[v-1][ecl])
            return v;
    }
    return -1;
}

// Total data codewords for a version/ecl
static int total_data_codewords(int version, int ecl) {
    const auto& ei = EC_TABLE[version-1][ecl];
    int g1 = ei.g1_blocks * ei.g1_data;
    int g2 = ei.g2_blocks * ei.g2_data;
    return g1 + g2;
}

// Direct byte packing — eliminates intermediate bit array.
static int build_data_codewords(
    const char* data, size_t len, int version, int /*ecl*/,
    uint8_t* codewords, int capacity)
{
    int cc_bits = (version <= 9) ? 8 : 16;

    if (cc_bits == 8) {
        int idx = 0;
        codewords[idx++] = static_cast<uint8_t>(0x40 | (len >> 4));

        uint8_t prev4 = static_cast<uint8_t>((len & 0x0F) << 4);
        for (size_t i = 0; i < len; ++i) {
            uint8_t b = static_cast<uint8_t>(data[i]);
            codewords[idx++] = static_cast<uint8_t>(prev4 | (b >> 4));
            prev4 = static_cast<uint8_t>((b & 0x0F) << 4);
        }
        codewords[idx++] = prev4;

        int pad_idx = 0;
        while (idx < capacity) {
            codewords[idx++] = (pad_idx++ % 2 == 0) ? 0xEC : 0x11;
        }
        return capacity;
    } else {
        int idx = 0;
        codewords[idx++] = static_cast<uint8_t>(0x40 | (len >> 12));
        codewords[idx++] = static_cast<uint8_t>((len >> 4) & 0xFF);

        uint8_t prev4 = static_cast<uint8_t>((len & 0x0F) << 4);
        for (size_t i = 0; i < len; ++i) {
            uint8_t b = static_cast<uint8_t>(data[i]);
            codewords[idx++] = static_cast<uint8_t>(prev4 | (b >> 4));
            prev4 = static_cast<uint8_t>((b & 0x0F) << 4);
        }
        codewords[idx++] = prev4;

        int pad_idx = 0;
        while (idx < capacity) {
            codewords[idx++] = (pad_idx++ % 2 == 0) ? 0xEC : 0x11;
        }
        return capacity;
    }
}

// Split the data codewords into RS blocks, compute each block's ECC and
// interleave both into the final codeword sequence. Blocks are addressed by
// offset into data_cw — no copies. QR has at most 81 blocks of <= 30 ECC bytes.
static int interleave(
    const uint8_t* data_cw, int data_len, int version, int ecl,
    uint8_t* output)
{
    (void)data_len;
    const auto& ei = EC_TABLE[version-1][ecl];
    const int num_short = ei.g1_blocks;
    const int num_long  = ei.g2_blocks;
    const int num_blocks = num_short + num_long;
    const int ec_len = ei.ec_per_block;
    const int short_data = ei.g1_data;
    const int long_data  = (num_long > 0) ? ei.g2_data : short_data;

    int offset[MAX_RS_BLOCKS];
    uint8_t block_ecc[MAX_RS_BLOCKS][MAX_RS_EC];

    int k = 0;
    for (int i = 0; i < num_blocks; ++i) {
        const int dlen = (i < num_short) ? short_data : long_data;
        offset[i] = k;
        rs_generate_ec({data_cw + k, static_cast<size_t>(dlen)}, {block_ecc[i], static_cast<size_t>(ec_len)});
        k += dlen;
    }

    int idx = 0;
    for (int col = 0; col < long_data; ++col) {
        for (int b = 0; b < num_blocks; ++b) {
            const int dlen = (b < num_short) ? short_data : long_data;
            if (col < dlen)
                output[idx++] = data_cw[offset[b] + col];
        }
    }
    for (int col = 0; col < ec_len; ++col) {
        for (int b = 0; b < num_blocks; ++b) {
            output[idx++] = block_ecc[b][col];
        }
    }
    return idx;
}

QRMatrix build_unmasked_matrix(const char* data, size_t len, ECL ecl) {
    if (len == 0) throw std::invalid_argument("empty data");
    int ecl_idx = static_cast<int>(ecl);
    int version = find_version(len, ecl_idx);
    if (version < 0) throw std::invalid_argument("data too large");

    int capacity = total_data_codewords(version, ecl_idx);
    uint8_t data_cw[4096];
    uint8_t all_cw[8192];
    build_data_codewords(data, len, version, ecl_idx, data_cw, capacity);
    int total_len = interleave(data_cw, capacity, version, ecl_idx, all_cw);

    QRMatrix m(version);
    place_finder_patterns(m);
    place_alignment_patterns(m);
    place_timing_patterns(m);
    place_dark_module(m);
    place_version_info(m);
    reserve_format_info(m);
    place_data(m, all_cw, total_len);
    return m;
}

int encode_into(QRMatrix& m, const char* data, size_t len, ECL ecl) {
    if (len == 0) throw std::invalid_argument("empty data");

    int ecl_idx = static_cast<int>(ecl);
    int version = find_version(len, ecl_idx);
    if (version < 0) throw std::invalid_argument("data too large");

    int capacity = total_data_codewords(version, ecl_idx);
    uint8_t data_cw[4096];
    uint8_t all_cw[8192];

    build_data_codewords(data, len, version, ecl_idx, data_cw, capacity);
    int total_len = interleave(data_cw, capacity, version, ecl_idx, all_cw);

    m.reset(version);
    place_finder_patterns(m);
    place_alignment_patterns(m);
    place_timing_patterns(m);
    place_dark_module(m);
    place_version_info(m);
    reserve_format_info(m);
    place_data(m, all_cw, total_len);

    int best_mask = select_best_mask(m, ecl_idx);
    place_format_info(m, ecl_idx, best_mask);
    apply_mask(m, best_mask);
    return version;
}

EncodeResult encode(const char* data, size_t len, ECL ecl) {
    EncodeResult r{QRMatrix(1), 0};
    r.version = encode_into(r.matrix, data, len, ecl);
    return r;
}

// EXPAND[b] holds the 8 bits of b as 8 bytes (bit i -> byte i), so a row can
// be converted 8 modules per store instead of one branchy byte at a time.
static const std::array<uint64_t, 256> EXPAND = []() {
    std::array<uint64_t, 256> t{};
    for (int b = 0; b < 256; ++b) {
        uint64_t v = 0;
        for (int i = 0; i < 8; ++i)
            if ((b >> i) & 1) v |= uint64_t(1) << (8 * i);
        t[static_cast<size_t>(b)] = v;
    }
    return t;
}();

void matrix_to_bytes(const QRMatrix& m, uint8_t* out) {
    const int sz = m.size;
    for (int y = 0; y < sz; ++y) {
        const Row3& row = m.rows[y];
        uint8_t* dst = out + static_cast<size_t>(y) * static_cast<size_t>(sz);
        // x is a multiple of 8 so an 8-bit slice never straddles a word.
        for (int x = 0; x < sz; x += 8) {
            const uint64_t chunk = EXPAND[(row.w[x >> 6] >> (x & 63)) & 0xFF];
            std::memcpy(dst + x, &chunk, 8);
        }
    }
}

EncodeResult encode_timed(const char* data, size_t len, ECL ecl, EncodeStages& st) {
    using clk = std::chrono::steady_clock;
    auto us = [](clk::time_point a, clk::time_point b) {
        return std::chrono::duration<double, std::micro>(b - a).count();
    };
    if (len == 0) throw std::invalid_argument("empty data");

    int ecl_idx = static_cast<int>(ecl);
    int version = find_version(len, ecl_idx);
    if (version < 0) throw std::invalid_argument("data too large");

    int capacity = total_data_codewords(version, ecl_idx);
    uint8_t data_cw[4096];
    uint8_t all_cw[8192];

    auto t0 = clk::now();
    build_data_codewords(data, len, version, ecl_idx, data_cw, capacity);
    auto t1 = clk::now();
    int total_len = interleave(data_cw, capacity, version, ecl_idx, all_cw);
    auto t2 = clk::now();

    QRMatrix m(version);
    place_finder_patterns(m);
    place_alignment_patterns(m);
    place_timing_patterns(m);
    place_dark_module(m);
    place_version_info(m);
    reserve_format_info(m);
    place_data(m, all_cw, total_len);
    auto t3 = clk::now();

    int best_mask = select_best_mask(m, ecl_idx);
    auto t4 = clk::now();
    place_format_info(m, ecl_idx, best_mask);
    apply_mask(m, best_mask);
    auto t5 = clk::now();

    st.codewords_us = us(t0, t1);
    st.rs_us = us(t1, t2);
    st.place_us = us(t2, t3);
    st.select_mask_us = us(t3, t4);
    st.apply_us = us(t4, t5);
    return {std::move(m), version};
}

EncodeResult encode_for_debug(const char* data, size_t len, ECL ecl, int penalties_out[8]) {
    int ecl_idx = static_cast<int>(ecl);
    int version = find_version(len, ecl_idx);
    if (version < 0) throw std::runtime_error("data too large");

    int capacity = total_data_codewords(version, ecl_idx);
    uint8_t data_cw[4096];
    uint8_t all_cw[8192];

    build_data_codewords(data, len, version, ecl_idx, data_cw, capacity);
    int total_len = interleave(data_cw, capacity, version, ecl_idx, all_cw);

    QRMatrix m(version);
    place_finder_patterns(m);
    place_alignment_patterns(m);
    place_timing_patterns(m);
    place_dark_module(m);
    place_version_info(m);
    reserve_format_info(m);
    place_data(m, all_cw, total_len);

    int best_mask = select_best_mask(m, ecl_idx, penalties_out);
    place_format_info(m, ecl_idx, best_mask);
    apply_mask(m, best_mask);

    return {std::move(m), version};
}

} // namespace scanme

extern "C" {

int scanme_qr_encode(
    const char*         data,
    size_t              len,
    int                 ecl,
    scanme_qr_result_t* out
) {
    out->modules = nullptr;
    out->size    = 0;
    out->version = 0;
    try {
        scanme::QRMatrix m(1);
        int version = scanme::encode_into(m, data, len, static_cast<scanme::ECL>(ecl));
        int sz = m.size;
        uint8_t* buf = new uint8_t[static_cast<size_t>(sz * sz) + 8];
        scanme::matrix_to_bytes(m, buf);
        out->modules = buf;
        out->size    = sz;
        out->version = version;
        return 0;
    } catch (...) {
        return -1;
    }
}

void scanme_qr_result_free(scanme_qr_result_t* out) {
    if (out && out->modules) {
        delete[] out->modules;
        out->modules = nullptr;
    }
}

scanme_qr_matrix_t* scanme_qr_encode_matrix(const char* data, size_t len, int ecl) {
    try {
        scanme::QRMatrix m(1);
        int version = scanme::encode_into(m, data, len, static_cast<scanme::ECL>(ecl));
        int sz = m.size;
        scanme_qr_matrix_t* m_out = new scanme_qr_matrix_t;
        m_out->version = version;
        m_out->size = sz;
        m_out->data = new uint8_t[static_cast<size_t>(sz * sz) + 8];
        scanme::matrix_to_bytes(m, m_out->data);
        return m_out;
    } catch (...) {
        return nullptr;
    }
}

void scanme_qr_matrix_free(scanme_qr_matrix_t* matrix) {
    if (matrix) {
        if (matrix->data) delete[] matrix->data;
        delete matrix;
    }
}

const char* scanme_qr_version(void) {
    return "1.0.0";
}

int scanme_qr_debug_penalties(
    const char* data,
    size_t      len,
    int         ecl,
    int         penalties_out[8]
) {
    try {
        auto ecl_enum = static_cast<scanme::ECL>(ecl);
        auto result   = scanme::encode_for_debug(data, len, ecl_enum, penalties_out);
        return result.version;
    } catch (...) {
        return -1;
    }
}

} // extern "C"
