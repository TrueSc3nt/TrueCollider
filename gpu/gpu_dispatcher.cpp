/*
 * GPU dispatcher for TrueCollider.
 *
 * Currently supports device enumeration and mode validation. Full EC+hash160
 * kernels are under construction in gpu/cuda/ and gpu/opencl/.
 */

#include "gpu_dispatcher.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

/*
 * Real CUDA/OpenCL implementations are compiled only when ENABLE_CUDA /
 * ENABLE_OPENCL are defined. Provide fallback stubs so the dispatcher links
 * cleanly in CPU-only builds.
 */
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
#else
extern "C" int topencl_device_count(void);
extern "C" int topencl_hello(void);
#endif

struct GpuDispatcher {
    struct BackendConfig cfg;
    int initialized;
    int device_count;
};

struct GpuDispatcher* gpu_dispatcher_create(void) {
    struct GpuDispatcher* disp = (struct GpuDispatcher*)calloc(1, sizeof(*disp));
    return disp;
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
            disp->initialized = 0;
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
    /*
     * Address and raw RMD160 search are embarrassingly parallel and map well
     * to a GPU. BSGS, mnemonic, vanity, minikeys, etc. remain CPU-only for
     * now because they need per-key branching, derivation, or wordlist logic.
     */
    return (mode == MODE_ADDRESS || mode == MODE_RMD160 || mode == MODE_VANITY);
}

uint32_t gpu_dispatcher_search_address(struct GpuDispatcher* disp,
                                       const uint8_t* private_keys,
                                       uint32_t count,
                                       uint32_t* matches) {
    (void)disp;
    (void)private_keys;
    (void)count;
    (void)matches;
    std::fprintf(stderr, "[W] GPU address search kernel is not yet implemented; running on CPU.\n");
    return 0;
}

uint32_t gpu_dispatcher_search_rmd160(struct GpuDispatcher* disp,
                                      const uint8_t* private_keys,
                                      uint32_t count,
                                      uint32_t* matches) {
    (void)disp;
    (void)private_keys;
    (void)count;
    (void)matches;
    std::fprintf(stderr, "[W] GPU rmd160 search kernel is not yet implemented; running on CPU.\n");
    return 0;
}

const char* gpu_dispatcher_name(const struct GpuDispatcher* disp) {
    if (!disp || !disp->initialized) return "none";
    if (disp->cfg.gpu_backend == GPU_BACKEND_CUDA) return "CUDA";
    if (disp->cfg.gpu_backend == GPU_BACKEND_OPENCL) return "OpenCL";
    return "unknown";
}
