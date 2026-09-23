#pragma once
#ifdef BUILD_NEON
#include <arm_neon.h>
#include <cstdint>
#include <bit>
#include "row3.hpp"

namespace scanme {

struct Row3Neon {
    uint64x2_t lo;
    uint64_t hi;

    static Row3Neon from_row3(Row3 r) noexcept {
        uint64_t buf[2] = {r.w[0], r.w[1]};
        return {vld1q_u64(buf), r.w[2]};
    }

    Row3 to_row3() const noexcept {
        uint64_t buf[2];
        vst1q_u64(buf, lo);
        return {{buf[0], buf[1], hi}};
    }
};

inline Row3Neon operator&(Row3Neon a, Row3Neon b) noexcept {
    return {vandq_u64(a.lo, b.lo), a.hi & b.hi};
}

inline Row3Neon operator|(Row3Neon a, Row3Neon b) noexcept {
    return {vorrq_u64(a.lo, b.lo), a.hi | b.hi};
}

inline Row3Neon operator^(Row3Neon a, Row3Neon b) noexcept {
    return {veorq_u64(a.lo, b.lo), a.hi ^ b.hi};
}

inline Row3Neon operator~(Row3Neon a) noexcept {
    uint32x4_t v32 = vreinterpretq_u32_u64(a.lo);
    return {vreinterpretq_u64_u32(vmvnq_u32(v32)), ~a.hi};
}

inline bool is_zero(Row3Neon r) noexcept {
    uint64_t buf[2];
    vst1q_u64(buf, r.lo);
    return (buf[0] | buf[1] | r.hi) == 0;
}

inline int popcnt_neon(Row3Neon r) noexcept {
    uint8x16_t cnt = vcntq_u8(vreinterpretq_u8_u64(r.lo));
    uint64_t sum = vaddlvq_u8(cnt);
    sum += std::popcount(r.hi);
    return static_cast<int>(sum);
}

inline Row3Neon shr_neon(Row3Neon r, int n) noexcept {
    Row3 s = r.to_row3();
    s = shr(s, n);
    return Row3Neon::from_row3(s);
}

inline Row3Neon shl_neon(Row3Neon r, int n) noexcept {
    Row3 s = r.to_row3();
    s = shl(s, n);
    return Row3Neon::from_row3(s);
}

} // namespace scanme
#endif // BUILD_NEON
