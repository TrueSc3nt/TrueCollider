/*
 * CPU feature detection (SSE, AVX, AVX2, AVX-512) for TrueCollider.
 *
 * Uses CPUID on x86/x86_64. On ARM or other architectures all features
 * report as disabled and the code falls back to portable scalar/SSE paths.
 */

#include "cpu_features.h"
#include "backend_config.h"

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#define TC_X86 1
#else
#define TC_X86 0
#endif

#if TC_X86
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#endif

#if TC_X86
static void tc_cpuid(uint32_t info, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
#if defined(_MSC_VER)
    int regs[4];
    __cpuid(regs, (int)info);
    *eax = (uint32_t)regs[0];
    *ebx = (uint32_t)regs[1];
    *ecx = (uint32_t)regs[2];
    *edx = (uint32_t)regs[3];
#else
    __cpuid(info, *eax, *ebx, *ecx, *edx);
#endif
}

static void tc_cpuidex(uint32_t info, uint32_t sub, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
#if defined(_MSC_VER)
    int regs[4];
    __cpuidex(regs, (int)info, (int)sub);
    *eax = (uint32_t)regs[0];
    *ebx = (uint32_t)regs[1];
    *ecx = (uint32_t)regs[2];
    *edx = (uint32_t)regs[3];
#else
    __cpuid_count(info, sub, *eax, *ebx, *ecx, *edx);
#endif
}

static int xgetbv_enabled(uint32_t mask) {
#if defined(_MSC_VER)
    return ((uint32_t)_xgetbv(0) & mask) == mask;
#elif defined(__GNUC__)
    uint32_t lo, hi;
    __asm__ __volatile__ ("xgetbv" : "=a"(lo), "=d"(hi) : "c"(0));
    return ((lo & mask) == mask);
#else
    return 0;
#endif
}
#endif

struct CpuFeatures detect_cpu_features(void) {
    struct CpuFeatures f = {0};

#if TC_X86
    uint32_t eax, ebx, ecx, edx;
    tc_cpuid(0, &eax, &ebx, &ecx, &edx);
    uint32_t max_leaf = eax;
    if (max_leaf < 1) {
        return f;
    }

    tc_cpuid(1, &eax, &ebx, &ecx, &edx);
    f.sse2 = (edx >> 26) & 1;
    f.ssse3 = (ecx >> 9) & 1;
    int osxsave = (ecx >> 27) & 1;

    if (osxsave && xgetbv_enabled(0x6)) { /* XCR0: XMM + YMM */
        f.avx = (ecx >> 28) & 1;
    }

    if (max_leaf >= 7) {
        uint32_t eax7, ebx7, ecx7, edx7;
        tc_cpuidex(7, 0, &eax7, &ebx7, &ecx7, &edx7);
        /* AVX2 only needs YMM state in XCR0 (same as AVX). */
        if (f.avx && xgetbv_enabled(0x6)) {
            f.avx2 = (ebx7 >> 5) & 1;
        }
        /* AVX-512 needs Opmask + ZMM_Hi256 + Hi16_ZMM (XCR0 bits 5–7) plus YMM/XMM. */
        if (f.avx2 && xgetbv_enabled(0xE6)) {
            f.avx512f = (ebx7 >> 16) & 1;
            f.avx512bw = (ebx7 >> 30) & 1;
            f.avx512vl = (ebx7 >> 31) & 1;
            f.avx512vbmi = (ecx7 >> 1) & 1;
        }
    }
#endif

    return f;
}

/*
 * Return a human-readable name of the best vector ISA supported by the host.
 */
const char* cpu_vector_name(void) {
    struct CpuFeatures f = detect_cpu_features();
    if (f.avx512f && f.avx512bw) return "AVX-512";
    if (f.avx2) return "AVX2";
    if (f.avx) return "AVX";
    if (f.ssse3) return "SSSE3";
    if (f.sse2) return "SSE2";
    return "scalar";
}

const char* cpu_vector_level_name(int level) {
    switch (level) {
        case CPU_VECTOR_AVX512: return "AVX-512";
        case CPU_VECTOR_AVX2:   return "AVX2";
        case CPU_VECTOR_AVX:    return "AVX";
        case CPU_VECTOR_SSE:    return "SSE";
        case CPU_VECTOR_NONE:   return "scalar";
        default:                return "unknown";
    }
}

/*
 * Which hash160 implementation actually runs for the selected level.
 * AVX/AVX2 have no dedicated hash kernels yet — they use SSE 4-wide.
 */
const char* cpu_hash_kernel_name(int level) {
    if (level == CPU_VECTOR_AVX512)
        return "AVX-512 16-wide hash160";
    if (level >= CPU_VECTOR_SSE)
        return "SSE 4-wide hash160";
    return "scalar hash160";
}

/*
 * Auto-select the best CPU vector level (matches CPU_VECTOR_* in backend_config.h).
 */
int cpu_has_avx512_hash160(void) {
    struct CpuFeatures f = detect_cpu_features();
    return f.avx512f && f.avx512bw;
}

int cpu_clamp_vector_level(int requested) {
    struct CpuFeatures f = detect_cpu_features();

    if (requested >= CPU_VECTOR_AVX512) {
        if (f.avx512f && f.avx512bw) return CPU_VECTOR_AVX512;
        requested = CPU_VECTOR_AVX2;
    }
    if (requested >= CPU_VECTOR_AVX2) {
        if (f.avx2) return CPU_VECTOR_AVX2;
        requested = CPU_VECTOR_AVX;
    }
    if (requested >= CPU_VECTOR_AVX) {
        if (f.avx) return CPU_VECTOR_AVX;
        requested = CPU_VECTOR_SSE;
    }
    if (requested >= CPU_VECTOR_SSE) {
        if (f.sse2) return CPU_VECTOR_SSE;
        return CPU_VECTOR_NONE;
    }
    return CPU_VECTOR_NONE;
}

int cpu_vector_auto_level(void) {
    return cpu_clamp_vector_level(CPU_VECTOR_AVX512);
}
