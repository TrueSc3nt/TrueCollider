#ifndef BACKEND_CONFIG_H
#define BACKEND_CONFIG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Search modes — must match keyhunt.cpp definitions. */
#define MODE_XPOINT     0
#define MODE_ADDRESS    1
#define MODE_BSGS       2
#define MODE_RMD160     3
#define MODE_PUB2RMD    4
#define MODE_MINIKEYS   5
#define MODE_VANITY     6
#define MODE_MNEMONIC   7
#define MODE_POETRY     8
#define MODE_BRAINWALLET 9
#define MODE_PUB2ADDR   10
#define MODE_KANGAROO   11

/* CPU vectorization levels. */
#define CPU_VECTOR_NONE   0
#define CPU_VECTOR_SSE    1
#define CPU_VECTOR_AVX    2
#define CPU_VECTOR_AVX2   3
#define CPU_VECTOR_AVX512 4

/* GPU backends. */
#define GPU_BACKEND_NONE    0
#define GPU_BACKEND_CUDA    1
#define GPU_BACKEND_OPENCL  2

struct BackendConfig {
    /* CPU vectorization (-A none/sse/avx/avx2/avx512 or auto). */
    int cpu_vector;
    int cpu_vector_auto;  /* 1 = pick best available at runtime */

    /* GPU (-U cuda/opencl or none). */
    int gpu_enabled;
    int gpu_backend;
    int gpu_device;
    uint32_t gpu_batch_size;
    int gpu_batch_user_set; /* 1 if -G was given */

    /* Memory budget for GPU/search buffers (-M MB|auto). 0 = unset. */
    uint64_t memory_budget_bytes;
    int memory_auto; /* 1 = size from free GPU VRAM */

    /* Global mode so backends can reject unsupported modes. */
    int search_mode;
};

extern struct BackendConfig g_backend_config;

#ifdef __cplusplus
}
#endif

#endif
