#pragma once
#ifdef BUILD_AVX2
#include <immintrin.h>
#include <cstdint>
#include "row3.hpp"

namespace scanme {

struct Row3Avx2 {
    __m256i v;

    static Row3Avx2 from_row3(Row3 r) noexcept {
        return {_mm256_set_epi64x(0, static_cast<int64_t>(r.w[2]),
                                     static_cast<int64_t>(r.w[1]),
                                     static_cast<int64_t>(r.w[0]))};
    }

    Row3 to_row3() const noexcept {
        alignas(32) uint64_t buf[4];
        _mm256_store_si256(reinterpret_cast<__m256i*>(buf), v);
        return {{buf[0], buf[1], buf[2]}};
    }
};

inline Row3Avx2 operator&(Row3Avx2 a, Row3Avx2 b) noexcept {
    return {_mm256_and_si256(a.v, b.v)};
}

inline Row3Avx2 operator|(Row3Avx2 a, Row3Avx2 b) noexcept {
    return {_mm256_or_si256(a.v, b.v)};
}

inline Row3Avx2 operator^(Row3Avx2 a, Row3Avx2 b) noexcept {
    return {_mm256_xor_si256(a.v, b.v)};
}

inline Row3Avx2 operator~(Row3Avx2 a) noexcept {
    return {_mm256_xor_si256(a.v, _mm256_set1_epi64x(-1LL))};
}

inline Row3Avx2 andnot(Row3Avx2 mask, Row3Avx2 a) noexcept {
    return {_mm256_andnot_si256(mask.v, a.v)};
}

inline bool is_zero(Row3Avx2 r) noexcept {
    return _mm256_testz_si256(r.v, r.v) != 0;
}

inline int popcnt_avx2(Row3Avx2 r) noexcept {
    const __m256i lut = _mm256_setr_epi8(
        0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
        0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4);
    const __m256i mask4 = _mm256_set1_epi8(0x0F);

    __m256i lo = _mm256_and_si256(r.v, mask4);
    __m256i hi = _mm256_and_si256(_mm256_srli_epi16(r.v, 4), mask4);
    __m256i cnt = _mm256_add_epi8(_mm256_shuffle_epi8(lut, lo),
                                   _mm256_shuffle_epi8(lut, hi));
    __m256i sum = _mm256_sad_epu8(cnt, _mm256_setzero_si256());
    alignas(32) uint64_t buf[4];
    _mm256_store_si256(reinterpret_cast<__m256i*>(buf), sum);
    return static_cast<int>(buf[0] + buf[1] + buf[2]);
}

inline Row3Avx2 shr1_avx2(Row3Avx2 r) noexcept {
    // Cross-lane shift right by 1 is complex in AVX2.
    // Extract to scalar Row3, shift, re-pack.
    Row3 s = r.to_row3();
    s = shr(s, 1);
    return Row3Avx2::from_row3(s);
}

inline Row3Avx2 shr_avx2(Row3Avx2 r, int n) noexcept {
    Row3 s = r.to_row3();
    s = shr(s, n);
    return Row3Avx2::from_row3(s);
}

inline Row3Avx2 shl1_avx2(Row3Avx2 r) noexcept {
    Row3 s = r.to_row3();
    s = shl(s, 1);
    return Row3Avx2::from_row3(s);
}

inline Row3Avx2 shl_avx2(Row3Avx2 r, int n) noexcept {
    Row3 s = r.to_row3();
    s = shl(s, n);
    return Row3Avx2::from_row3(s);
}

} // namespace scanme
#endif // BUILD_AVX2
