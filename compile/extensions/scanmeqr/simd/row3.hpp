#pragma once
#include <bit>
#include <cstdint>

namespace scanme {

struct Row3 {
    uint64_t w[3];

    static constexpr Row3 zero() noexcept { return {{0, 0, 0}}; }

    static constexpr Row3 from_words(uint64_t w0, uint64_t w1, uint64_t w2) noexcept {
        return {{w0, w1, w2}};
    }
};

inline Row3 operator&(Row3 a, Row3 b) noexcept {
    return {{a.w[0] & b.w[0], a.w[1] & b.w[1], a.w[2] & b.w[2]}};
}

inline Row3 operator|(Row3 a, Row3 b) noexcept {
    return {{a.w[0] | b.w[0], a.w[1] | b.w[1], a.w[2] | b.w[2]}};
}

inline Row3 operator^(Row3 a, Row3 b) noexcept {
    return {{a.w[0] ^ b.w[0], a.w[1] ^ b.w[1], a.w[2] ^ b.w[2]}};
}

inline Row3 operator~(Row3 a) noexcept {
    return {{~a.w[0], ~a.w[1], ~a.w[2]}};
}

inline Row3 shr1(Row3 r) noexcept {
    return {{
        (r.w[0] >> 1) | (r.w[1] << 63),
        (r.w[1] >> 1) | (r.w[2] << 63),
        r.w[2] >> 1
    }};
}

inline Row3 shr(Row3 r, int n) noexcept {
    return {{
        (r.w[0] >> n) | (r.w[1] << (64 - n)),
        (r.w[1] >> n) | (r.w[2] << (64 - n)),
        r.w[2] >> n
    }};
}

inline Row3 shl1(Row3 r) noexcept {
    return {{
        r.w[0] << 1,
        (r.w[1] << 1) | (r.w[0] >> 63),
        (r.w[2] << 1) | (r.w[1] >> 63)
    }};
}

inline Row3 shl(Row3 r, int n) noexcept {
    return {{
        r.w[0] << n,
        (r.w[1] << n) | (r.w[0] >> (64 - n)),
        (r.w[2] << n) | (r.w[1] >> (64 - n))
    }};
}

inline bool is_zero(Row3 r) noexcept {
    return (r.w[0] | r.w[1] | r.w[2]) == 0;
}

inline Row3 andnot(Row3 mask, Row3 a) noexcept {
    return {{~mask.w[0] & a.w[0], ~mask.w[1] & a.w[1], ~mask.w[2] & a.w[2]}};
}

inline int popcnt(Row3 r) noexcept {
    return std::popcount(r.w[0]) + std::popcount(r.w[1]) + std::popcount(r.w[2]);
}

inline Row3 mask_low_n(int n) noexcept {
    Row3 r = {{0, 0, 0}};
    if (n >= 192) return {{~uint64_t(0), ~uint64_t(0), ~uint64_t(0)}};
    if (n >= 128) {
        r.w[0] = ~uint64_t(0);
        r.w[1] = ~uint64_t(0);
        r.w[2] = (uint64_t(1) << (n - 128)) - 1;
    } else if (n >= 64) {
        r.w[0] = ~uint64_t(0);
        r.w[1] = (uint64_t(1) << (n - 64)) - 1;
    } else if (n > 0) {
        r.w[0] = (uint64_t(1) << n) - 1;
    }
    return r;
}

} // namespace scanme
