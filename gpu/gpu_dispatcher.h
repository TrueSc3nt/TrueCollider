#ifndef GPU_DISPATCHER_H
#define GPU_DISPATCHER_H

#include "backend_config.h"
#include <stdint.h>

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
 * Batch hash160 for compressed public keys (33 bytes each).
 * Host supplies keys; writes 20-byte hashes to out.
 * Returns 1 on success.
 */
int gpu_dispatcher_hash160_33(struct GpuDispatcher* disp,
                              const uint8_t* keys33,
                              uint32_t count,
                              uint8_t* out20);

/*
 * Submit a batch of compressed pubkeys for address-mode hash+filter.
 * keys33: count * 33 bytes. matches: optional index list (may be NULL).
 * Returns number of filter hits written to matches (0 if matches is NULL —
 * caller should filter host-side using out hashes via hash160_33).
 */
uint32_t gpu_dispatcher_search_address(struct GpuDispatcher* disp,
                                       const uint8_t* keys33,
                                       uint32_t count,
                                       uint32_t* matches);

uint32_t gpu_dispatcher_search_rmd160(struct GpuDispatcher* disp,
                                      const uint8_t* keys33,
                                      uint32_t count,
                                      uint32_t* matches);

const char* gpu_dispatcher_name(const struct GpuDispatcher* disp);

#ifdef __cplusplus
}
#endif

#endif
