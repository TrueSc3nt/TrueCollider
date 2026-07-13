/*
 * GPU dispatcher for TrueCollider.
 *
 * Phase 2: host builds compressed pubkeys; GPU runs batch hash160;
 * host runs filter / binary search.
 */

#include "gpu_dispatcher.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifndef ENABLE_CUDA
extern "C" int tcuda_device_count(void) { return 0; }
extern "C" int tcuda_hello(void) { return 0; }
extern "C" int tcuda_hash160_33_selftest(void) { return 0; }
extern "C" int tcuda_hash160_33_batch(const uint8_t*, int, uint8_t*) { return 0; }
#else
extern "C" int tcuda_device_count(void);
extern "C" int tcuda_hello(void);
extern "C" int tcuda_hash160_33_selftest(void);
extern "C" int tcuda_hash160_33_batch(const uint8_t *host_keys, int count, uint8_t *host_out);
#endif

#ifndef ENABLE_OPENCL
extern "C" int topencl_device_count(void) { return 0; }
extern "C" int topencl_hello(void) { return 0; }
extern "C" int topencl_hash160_33_selftest(void) { return 0; }
extern "C" int topencl_hash160_33_batch(const uint8_t*, int, uint8_t*) { return 0; }
#else
extern "C" int topencl_device_count(void);
extern "C" int topencl_hello(void);
extern "C" int topencl_hash160_33_selftest(void);
extern "C" int topencl_hash160_33_batch(const uint8_t *host_keys, int count, uint8_t *host_out);
#endif

struct GpuDispatcher {
    struct BackendConfig cfg;
    int initialized;
    int device_count;
};

struct GpuDispatcher* gpu_dispatcher_create(void) {
    return (struct GpuDispatcher*)calloc(1, sizeof(struct GpuDispatcher));
}

void gpu_dispatcher_destroy(struct GpuDispatcher* disp) {
    free(disp);
}

int gpu_dispatcher_init(struct GpuDispatcher* disp, const struct BackendConfig* cfg) {
    if (!disp || !cfg) return 0;
    disp->cfg = *cfg;
    disp->initialized = 0;
    disp->device_count = 0;

    if (cfg->gpu_backend == GPU_BACKEND_CUDA) {
        disp->device_count = tcuda_device_count();
        if (disp->device_count <= 0) {
            std::fprintf(stderr, "[E] CUDA backend requested but no CUDA devices were found.\n");
            return 0;
        }
        tcuda_hello();
        if (!tcuda_hash160_33_selftest()) {
            std::fprintf(stderr, "[W] CUDA hash160 self-test failed; GPU hash offload disabled.\n");
            return 0;
        }
        std::fprintf(stderr, "[+] CUDA backend selected (%d device%s).\n",
                     disp->device_count, disp->device_count > 1 ? "s" : "");
        disp->initialized = 1;
        return 1;
    }

    if (cfg->gpu_backend == GPU_BACKEND_OPENCL) {
        disp->device_count = topencl_device_count();
        if (disp->device_count <= 0) {
            std::fprintf(stderr, "[E] OpenCL backend requested but no GPU devices were found.\n");
            return 0;
        }
        topencl_hello();
        if (!topencl_hash160_33_selftest()) {
            std::fprintf(stderr, "[W] OpenCL hash160 self-test failed; GPU hash offload disabled.\n");
            return 0;
        }
        std::fprintf(stderr, "[+] OpenCL backend selected (%d device%s).\n",
                     disp->device_count, disp->device_count > 1 ? "s" : "");
        disp->initialized = 1;
        return 1;
    }

    return 0;
}

int gpu_dispatcher_available(const struct GpuDispatcher* disp) {
    return disp && disp->initialized;
}

int gpu_dispatcher_supports_mode(const struct GpuDispatcher* disp, int mode) {
    (void)disp;
    return (mode == MODE_ADDRESS || mode == MODE_RMD160 || mode == MODE_VANITY);
}

int gpu_dispatcher_hash160_33(struct GpuDispatcher* disp,
                              const uint8_t* keys33,
                              uint32_t count,
                              uint8_t* out20) {
    if (!disp || !disp->initialized || !keys33 || !out20 || count == 0) return 0;
    if (disp->cfg.gpu_backend == GPU_BACKEND_CUDA)
        return tcuda_hash160_33_batch(keys33, (int)count, out20);
    if (disp->cfg.gpu_backend == GPU_BACKEND_OPENCL)
        return topencl_hash160_33_batch(keys33, (int)count, out20);
    return 0;
}

uint32_t gpu_dispatcher_search_address(struct GpuDispatcher* disp,
                                       const uint8_t* keys33,
                                       uint32_t count,
                                       uint32_t* matches) {
    (void)matches;
    /* Phase 2: hash only — caller filters on host. */
    if (!disp || !disp->initialized) return 0;
    /* Allocate temporary out buffer if matches requested without out path;
     * this API is a thin wrapper; prefer gpu_dispatcher_hash160_33. */
    (void)keys33;
    (void)count;
    return 0;
}

uint32_t gpu_dispatcher_search_rmd160(struct GpuDispatcher* disp,
                                      const uint8_t* keys33,
                                      uint32_t count,
                                      uint32_t* matches) {
    return gpu_dispatcher_search_address(disp, keys33, count, matches);
}

const char* gpu_dispatcher_name(const struct GpuDispatcher* disp) {
    if (!disp || !disp->initialized) return "none";
    if (disp->cfg.gpu_backend == GPU_BACKEND_CUDA) return "CUDA";
    if (disp->cfg.gpu_backend == GPU_BACKEND_OPENCL) return "OpenCL";
    return "unknown";
}
