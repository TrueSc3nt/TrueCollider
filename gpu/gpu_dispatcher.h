#ifndef GPU_DISPATCHER_H
#define GPU_DISPATCHER_H

#include "../backend_config.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Address encode after GPU EC (host-side hash / bloom). */
#define GPU_ENCODE_HASH160 0  /* BTC-family P2PKH hash160 */
#define GPU_ENCODE_ETH     1  /* Keccak256(X||Y) → last 20 bytes */

struct GpuDispatcher;

struct GpuDispatcher* gpu_dispatcher_create(void);
void gpu_dispatcher_destroy(struct GpuDispatcher* disp);

int gpu_dispatcher_init(struct GpuDispatcher* disp, const struct BackendConfig* cfg);
int gpu_dispatcher_available(const struct GpuDispatcher* disp);
/* 1 if CUDA secp256k1 + device bloom path passed self-test / is usable. */
int gpu_dispatcher_secp_ready(const struct GpuDispatcher* disp);
int gpu_dispatcher_ed25519_ready(const struct GpuDispatcher* disp);

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

/* Upload host bloom filter for device-side probing (CUDA secp path). */
int gpu_dispatcher_load_bloom(struct GpuDispatcher* disp,
                              const uint8_t* bf, uint64_t bits, uint64_t bytes, uint8_t hashes);

/*
 * Full GPU search: privkeys (count*32 BE) -> secp256k1 -> encode -> bloom.
 * encode: GPU_ENCODE_HASH160 or GPU_ENCODE_ETH (ETH requires uncompressed).
 * Returns number of bloom hits; match_indices filled up to max_matches.
 */
uint32_t gpu_dispatcher_search_privkeys(struct GpuDispatcher* disp,
                                        const uint8_t* privkeys,
                                        uint32_t count,
                                        int compressed,
                                        int encode,
                                        uint32_t* match_indices,
                                        uint32_t max_matches);

/*
 * GPU EC only: privkeys (count*32 BE) -> count*65 pubkey bytes (0x02/03/04 + coords).
 * Returns 1 on success. Used by Taproot / experimental BSGS helpers.
 */
int gpu_dispatcher_pubkey_batch(struct GpuDispatcher* disp,
                                const uint8_t* privkeys,
                                uint32_t count,
                                int compressed,
                                uint8_t* out_pubs65);

/* Solana: seeds (count*32) → ed25519 pubs (count*32). Full device ge when CUDA ready. */
int gpu_dispatcher_ed25519_pubkey_batch(struct GpuDispatcher* disp,
                                        const uint8_t* seeds32,
                                        uint32_t count,
                                        uint8_t* out_pubs32);

/* BSGS giant-step GRP on CUDA (GSn + _2GSn upload; cycle run returns x-coords). */
int gpu_dispatcher_bsgs_grp_init(struct GpuDispatcher* disp,
                                 const uint8_t* gsn_xy64, int half,
                                 const uint8_t* twogsn_xy64);
int gpu_dispatcher_bsgs_grp_ready(const struct GpuDispatcher* disp);
int gpu_dispatcher_bsgs_grp_run(struct GpuDispatcher* disp,
                                uint8_t* start_xy64, int n_cycles, uint8_t* out_x32);

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
