#ifndef CPU_FEATURES_H
#define CPU_FEATURES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct CpuFeatures {
    int sse2;
    int ssse3;
    int avx;
    int avx2;
    int avx512f;
    int avx512bw;
    int avx512vl;
    int avx512vbmi;
};

struct CpuFeatures detect_cpu_features(void);
const char* cpu_vector_name(void);
const char* cpu_vector_level_name(int level);
const char* cpu_hash_kernel_name(int level);
int cpu_vector_auto_level(void);

/* True when CPU + OS expose AVX-512F/BW (required for hash160 AVX-512 kernel). */
int cpu_has_avx512_hash160(void);

/*
 * Clamp a requested CPU_VECTOR_* level to what this machine supports.
 * Used so -A avx512 on an SSE-only PC safely falls back instead of crashing.
 */
int cpu_clamp_vector_level(int requested);

#ifdef __cplusplus
}
#endif

#endif
