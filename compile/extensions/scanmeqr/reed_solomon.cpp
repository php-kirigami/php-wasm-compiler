#include "reed_solomon.hpp"
#include "tables.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <mutex>

namespace scanme {

// Reed-Solomon over GF(256) via polynomial long division with a precomputed
// factor table: table[factor][i] = gf_mul(gen_poly[i+1], factor).
//
// The remainder accumulator is held in four uint64 words (32 bytes, byte i at
// bits [8i, 8i+8) little-endian), so each data byte costs one 256-bit
// byte-shift plus one 256-bit XOR against the table row — no per-byte loop.
// QR's largest ec_count is 30, which fits with room to spare.
//
// Tables are cached per ec_count (QR uses only ~13 distinct values), each
// built once on first use, so mixed-version workloads never rebuild.
namespace {

constexpr int MAX_EC = 30;
constexpr int ROW_WORDS = 4; // 32 bytes >= MAX_EC

static_assert(std::endian::native == std::endian::little,
              "rs_generate_ec assumes little-endian byte order in the accumulator words");

// Sanity check the table bound against the EC table at compile time.
constexpr int max_ec_per_block() {
    int m = 0;
    for (const auto& row : EC_TABLE)
        for (const auto& e : row)
            if (e.ec_per_block > m) m = e.ec_per_block;
    return m;
}
static_assert(max_ec_per_block() <= MAX_EC, "MAX_EC too small for EC_TABLE");

struct alignas(32) RsRow { uint64_t w[ROW_WORDS]; };

struct RsFactorTable {
    std::once_flag once;
    RsRow rows[256]; // rows[0] is all-zero so factor == 0 needs no branch
};

RsFactorTable g_tables[MAX_EC + 1];

void build_factor_table(RsFactorTable& ft, int ec_count) {
    // Generator polynomial: prod_{i<ec_count} (x - alpha^i), highest degree first.
    std::array<uint8_t, MAX_EC + 1> poly{};
    int deg = 0;
    poly[0] = 1;
    for (int i = 0; i < ec_count; ++i) {
        const uint8_t alpha_i = GF_EXP[i % 255];
        std::array<uint8_t, MAX_EC + 1> next{};
        for (int j = 0; j <= deg; ++j) {
            next[static_cast<size_t>(j)] ^= poly[static_cast<size_t>(j)];
            next[static_cast<size_t>(j + 1)] ^= gf_mul(poly[static_cast<size_t>(j)], alpha_i);
        }
        poly = next;
        ++deg;
    }

    std::memset(ft.rows, 0, sizeof(ft.rows));
    for (int factor = 1; factor < 256; ++factor) {
        uint8_t bytes[ROW_WORDS * 8] = {};
        for (int i = 0; i < ec_count; ++i)
            bytes[i] = gf_mul(poly[static_cast<size_t>(i + 1)], static_cast<uint8_t>(factor));
        std::memcpy(ft.rows[factor].w, bytes, sizeof(bytes));
    }
}

const RsFactorTable& factor_table(int ec_count) {
    RsFactorTable& ft = g_tables[ec_count];
    std::call_once(ft.once, [&] { build_factor_table(ft, ec_count); });
    return ft;
}

} // namespace

void rs_generate_ec(
    std::span<const uint8_t> data,
    std::span<uint8_t> ecc
) {
    const int ec_count = static_cast<int>(ecc.size());
    const RsFactorTable& ft = factor_table(ec_count);

    uint64_t a0 = 0, a1 = 0, a2 = 0, a3 = 0;
    for (const uint8_t byte : data) {
        const uint8_t factor = static_cast<uint8_t>(byte ^ static_cast<uint8_t>(a0));
        const RsRow& row = ft.rows[factor];
        // Shift the accumulator left by one byte (drop byte 0), then XOR the row.
        a0 = ((a0 >> 8) | (a1 << 56)) ^ row.w[0];
        a1 = ((a1 >> 8) | (a2 << 56)) ^ row.w[1];
        a2 = ((a2 >> 8) | (a3 << 56)) ^ row.w[2];
        a3 = (a3 >> 8) ^ row.w[3];
    }

    uint8_t out[ROW_WORDS * 8];
    std::memcpy(out, &a0, 8);
    std::memcpy(out + 8, &a1, 8);
    std::memcpy(out + 16, &a2, 8);
    std::memcpy(out + 24, &a3, 8);
    std::memcpy(ecc.data(), out, static_cast<size_t>(ec_count));
}

} // namespace scanme
