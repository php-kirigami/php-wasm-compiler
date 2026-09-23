#pragma once
#ifdef BUILD_SSE2
#include <emmintrin.h>
#include <cstdint>
#include <bit>
#include "row3.hpp"

namespace scanme {

struct Row3Sse2 {
    __m128i lo;
    uint64_t hi;

    static Row3Sse2 from_row3(Row3 r) noexcept {
        return {_mm_set_epi64x(static_cast<int64_t>(r.w[1]),
                               static_cast<int64_t>(r.w[0])),
                r.w[2]};
    }

    Row3 to_row3() const noexcept {
        alignas(16) uint64_t buf[2];
        _mm_store_si128(reinterpret_cast<__m128i*>(buf), lo);
        return {{buf[0], buf[1], hi}};
    }
};

inline Row3Sse2 operator&(Row3Sse2 a, Row3Sse2 b) noexcept {
    return {_mm_and_si128(a.lo, b.lo), a.hi & b.hi};
}

inline Row3Sse2 operator|(Row3Sse2 a, Row3Sse2 b) noexcept {
    return {_mm_or_si128(a.lo, b.lo), a.hi | b.hi};
}

inline Row3Sse2 operator^(Row3Sse2 a, Row3Sse2 b) noexcept {
    return {_mm_xor_si128(a.lo, b.lo), a.hi ^ b.hi};
}

inline Row3Sse2 operator~(Row3Sse2 a) noexcept {
    __m128i ones = _mm_set1_epi64x(-1LL);
    return {_mm_xor_si128(a.lo, ones), ~a.hi};
}

inline Row3Sse2 andnot(Row3Sse2 mask, Row3Sse2 a) noexcept {
    return {_mm_andnot_si128(mask.lo, a.lo), ~mask.hi & a.hi};
}

inline bool is_zero(Row3Sse2 r) noexcept {
    alignas(16) uint64_t buf[2];
    _mm_store_si128(reinterpret_cast<__m128i*>(buf), r.lo);
    return (buf[0] | buf[1] | r.hi) == 0;
}

inline int popcnt_sse2(Row3Sse2 r) noexcept {
    alignas(16) uint64_t buf[2];
    _mm_store_si128(reinterpret_cast<__m128i*>(buf), r.lo);
    return std::popcount(buf[0]) + std::popcount(buf[1]) + std::popcount(r.hi);
}

inline Row3Sse2 shr1_sse2(Row3Sse2 r) noexcept {
    Row3 s = r.to_row3();
    s = shr(s, 1);
    return Row3Sse2::from_row3(s);
}

inline Row3Sse2 shr_sse2(Row3Sse2 r, int n) noexcept {
    Row3 s = r.to_row3();
    s = shr(s, n);
    return Row3Sse2::from_row3(s);
}

inline Row3Sse2 shl1_sse2(Row3Sse2 r) noexcept {
    Row3 s = r.to_row3();
    s = shl(s, 1);
    return Row3Sse2::from_row3(s);
}

} // namespace scanme
#endif // BUILD_SSE2
