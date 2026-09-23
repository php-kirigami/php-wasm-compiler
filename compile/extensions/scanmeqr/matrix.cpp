#include "matrix.hpp"
#include "tables.hpp"
#include <array>
#include <cstdlib>
#include <cstring>

namespace scanme {

static void place_finder(QRMatrix& m, int tx, int ty) {
    for (int dy = -1; dy <= 7; ++dy) {
        for (int dx = -1; dx <= 7; ++dx) {
            int x = tx + dx, y = ty + dy;
            if (x < 0 || x >= m.size || y < 0 || y >= m.size) continue;
            bool dark = false;
            if (dx >= 0 && dx <= 6 && dy >= 0 && dy <= 6) {
                if (dx == 0 || dx == 6 || dy == 0 || dy == 6) dark = true;
                else if (dx >= 2 && dx <= 4 && dy >= 2 && dy <= 4) dark = true;
            }
            m.set_function(x, y, dark);
        }
    }
}

void place_finder_patterns(QRMatrix& m) {
    place_finder(m, 0, 0);
    place_finder(m, m.size - 7, 0);
    place_finder(m, 0, m.size - 7);
}

void place_alignment_patterns(QRMatrix& m) {
    if (m.version < 2) return;
    int count = ALIGN_COUNT[m.version];
    const auto& pos = ALIGN_POS[m.version];
    for (int i = 0; i < count; ++i) {
        for (int j = 0; j < count; ++j) {
            int cx = pos[j], cy = pos[i];
            // Skip if overlaps finder patterns
            if ((cx <= 8 && cy <= 8) ||
                (cx >= m.size - 8 && cy <= 8) ||
                (cx <= 8 && cy >= m.size - 8)) continue;
            for (int dy = -2; dy <= 2; ++dy) {
                for (int dx = -2; dx <= 2; ++dx) {
                    bool dark = (std::abs(dx) == 2 || std::abs(dy) == 2 || (dx == 0 && dy == 0));
                    m.set_function(cx + dx, cy + dy, dark);
                }
            }
        }
    }
}

void place_timing_patterns(QRMatrix& m) {
    for (int i = 8; i < m.size - 8; ++i) {
        bool dark = (i % 2 == 0);
        m.set_function(i, 6, dark);
        m.set_function(6, i, dark);
    }
}

void place_dark_module(QRMatrix& m) {
    m.set_function(8, 4 * m.version + 9, true);
}

void reserve_format_info(QRMatrix& m) {
    int n = m.size;
    // Top-left: column x=8, rows 0-5,7,8
    for (int y : {0, 1, 2, 3, 4, 5, 7, 8})
        m.mark_function(8, y);
    // Top-left: row y=8, cols 7,5,4,3,2,1,0
    for (int x : {7, 5, 4, 3, 2, 1, 0})
        m.mark_function(x, 8);
    // Top-right: row y=8, cols n-1 down to n-8
    for (int i = 0; i < 8; ++i)
        m.mark_function(n - 1 - i, 8);
    // Bottom-left: col x=8, rows n-7 up to n-1
    for (int i = 8; i < 15; ++i)
        m.mark_function(8, n - 15 + i);
}

void place_format_info(QRMatrix& m, int ecl, int mask) {
    uint16_t fmt = FORMAT_INFO[ecl * 8 + mask];
    int n = m.size;

    // Top-left: column x=8, rows 0-5,7,8 then row y=8, cols 7,5,4,3,2,1,0
    m.set_function(8, 0, (fmt >> 0) & 1);
    m.set_function(8, 1, (fmt >> 1) & 1);
    m.set_function(8, 2, (fmt >> 2) & 1);
    m.set_function(8, 3, (fmt >> 3) & 1);
    m.set_function(8, 4, (fmt >> 4) & 1);
    m.set_function(8, 5, (fmt >> 5) & 1);
    m.set_function(8, 7, (fmt >> 6) & 1);
    m.set_function(8, 8, (fmt >> 7) & 1);
    m.set_function(7, 8, (fmt >> 8) & 1);
    m.set_function(5, 8, (fmt >> 9) & 1);
    m.set_function(4, 8, (fmt >> 10) & 1);
    m.set_function(3, 8, (fmt >> 11) & 1);
    m.set_function(2, 8, (fmt >> 12) & 1);
    m.set_function(1, 8, (fmt >> 13) & 1);
    m.set_function(0, 8, (fmt >> 14) & 1);

    // Top-right: row y=8, cols n-1 down to n-8
    for (int i = 0; i < 8; ++i)
        m.set_function(n - 1 - i, 8, (fmt >> i) & 1);

    // Bottom-left: col x=8, rows n-7 up to n-1
    for (int i = 8; i < 15; ++i)
        m.set_function(8, n - 15 + i, (fmt >> i) & 1);
}

void place_version_info(QRMatrix& m) {
    if (m.version < 7) return;
    uint32_t ver = VERSION_INFO[m.version - 7];
    for (int i = 0; i < 18; ++i) {
        bool dark = (ver >> i) & 1;
        int a = m.size - 11 + i % 3;
        int b = i / 3;
        m.set_function(a, b, dark);
        m.set_function(b, a, dark);
    }
}

// Bit-reverse table for bytes: the codeword stream is MSB-first, but we want
// to pull bits out LSB-first with plain shifts.
static const std::array<uint8_t, 256> REV8 = []() {
    std::array<uint8_t, 256> t{};
    for (int b = 0; b < 256; ++b) {
        uint8_t r = 0;
        for (int i = 0; i < 8; ++i) if ((b >> i) & 1) r |= static_cast<uint8_t>(1 << (7 - i));
        t[static_cast<size_t>(b)] = r;
    }
    return t;
}();

// Zig-zag placement, two modules (x = col, x = col-1) per row per step.
//
// Branch-free inner loop: for each row we read the 2-bit function mask of the
// pair, peek the next 2 stream bits, and use tiny tables to decide which bits
// land where and how many stream bits were consumed:
//   func2 bit1 = (col is function), bit0 = (col-1 is function)
//   d2    bit0 = next stream bit (goes to col), bit1 = the one after (col-1)
// OUT[func2][d2] is the 2-bit pattern to OR in at position col-1
// (bit1 -> col, bit0 -> col-1); ADV[func2] is the number of bits consumed.
void place_data(QRMatrix& m, const uint8_t* data, int data_len) {
    static constexpr uint8_t ADV[4] = {2, 1, 1, 0};
    static constexpr uint8_t OUT[4][4] = {
        // d2:  00  01  10  11        (bit0 = first stream bit, bit1 = second)
        /*00*/ {0b00, 0b10, 0b01, 0b11}, // both free: first -> col (bit1), second -> col-1 (bit0)
        /*01*/ {0b00, 0b10, 0b00, 0b10}, // col-1 is function: first -> col only
        /*10*/ {0b00, 0b01, 0b00, 0b01}, // col is function: first -> col-1 only
        /*11*/ {0b00, 0b00, 0b00, 0b00}, // both function
    };

    // LSB-first copy of the stream, zero padded so peeking past the end reads 0
    // (remainder modules stay light).
    uint8_t stream[8192 + 16];
    for (int i = 0; i < data_len; ++i) stream[i] = REV8[data[i]];
    std::memset(stream + data_len, 0, 16);

    const int n = m.size;
    unsigned bit_idx = 0;

    for (int col = n - 1; col > 0; col -= 2) {
        if (col == 6) col--;  // skip timing column
        const bool up = ((n - 1 - col) / 2) % 2 == 0;
        const int x0 = col - 1;          // low column of the pair
        const int wi = x0 >> 6;          // word holding bit x0
        const int sh = x0 & 63;          // bit offset of x0 within the word
        // The pair straddles a word boundary only when sh == 63.
        const bool straddle = (sh == 63) && (wi + 1 < 3);

        for (int row_step = 0; row_step < n; ++row_step) {
            const int y = up ? (n - 1 - row_step) : row_step;
            Row3& frow = m.func[y];
            uint64_t func2 = frow.w[wi] >> sh;
            if (straddle) func2 |= frow.w[wi + 1] << 1;
            func2 &= 3;

            uint64_t peek;
            std::memcpy(&peek, stream + (bit_idx >> 3), 8);
            const unsigned d2 = static_cast<unsigned>((peek >> (bit_idx & 7)) & 3);

            const uint64_t out = OUT[func2][d2];
            Row3& drow = m.rows[y];
            drow.w[wi] |= out << sh;
            if (straddle) drow.w[wi + 1] |= out >> 1;
            bit_idx += ADV[func2];
        }
    }
}

} // namespace scanme
