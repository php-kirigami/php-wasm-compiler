#pragma once
#include <cstdint>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
  #include <cpuid.h>
#endif

namespace scanme {

struct CpuFeatures {
    bool sse2     = false;
    bool sse42    = false;
    bool avx2     = false;
    bool avx512f  = false;
    bool avx512bw = false;
    bool neon     = false;
};

inline CpuFeatures detect_cpu_features() noexcept {
    CpuFeatures f{};
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
    uint32_t eax, ebx, ecx, edx;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
        f.sse2  = (edx >> 26) & 1;
        f.sse42 = (ecx >> 20) & 1;
    }
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx)) {
        f.avx2     = (ebx >>  5) & 1;
        f.avx512f  = (ebx >> 16) & 1;
        f.avx512bw = (ebx >> 30) & 1;
    }
#elif defined(__aarch64__) || defined(__ARM_NEON)
    f.neon = true;
#endif
    return f;
}

inline const CpuFeatures& cpu_features() noexcept {
    static const CpuFeatures features = detect_cpu_features();
    return features;
}

} // namespace scanme
