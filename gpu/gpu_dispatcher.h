#ifndef GPU_DISPATCHER_H
#define GPU_DISPATCHER_H

#include "backend_config.h"

#ifdef __cplusplus
extern "C" {
#endif

struct GpuDispatcher;

struct GpuDispatcher* gpu_dispatcher_create(void);
void gpu_dispatcher_destroy(struct GpuDispatcher* disp);

int gpu_dispatcher_init(struct GpuDispatcher* disp, const struct BackendConfig* cfg);
int gpu_dispatcher_available(const struct GpuDispatcher* disp);

/* Returns 1 if the current mode can be run on the GPU. */
int gpu_dispatcher_supports_mode(const struct GpuDispatcher* disp, int mode);

/*
 * Submit a batch of private keys for address-mode search.
 * Returns the number of candidate matches found (indices written to matches).
 * A return value of 0 means no candidates in this batch.
 */
uint32_t gpu_dispatcher_search_address(struct GpuDispatcher* disp,
                                       const uint8_t* private_keys,
                                       uint32_t count,
                                       uint32_t* matches);

uint32_t gpu_dispatcher_search_rmd160(struct GpuDispatcher* disp,
                                      const uint8_t* private_keys,
                                      uint32_t count,
                                      uint32_t* matches);

const char* gpu_dispatcher_name(const struct GpuDispatcher* disp);

#ifdef __cplusplus
}
#endif

#endif
