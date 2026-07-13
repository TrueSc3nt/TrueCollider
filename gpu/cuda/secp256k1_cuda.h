#ifndef SECP256K1_CUDA_H
#define SECP256K1_CUDA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CUDA secp256k1 search: privkey -> pubkey -> hash160 -> bloom check.
 * Bloom layout matches bloom/bloom.cpp (XXH64 double-hash).
 */
int tcuda_secp_search_init(const uint8_t *bloom_bf, uint64_t bloom_bits,
                           uint64_t bloom_bytes, uint8_t bloom_hashes);
void tcuda_secp_search_free(void);

/* privkeys: count*32 big-endian; returns number of bloom hits; match_indices optional */
int tcuda_secp_search_batch(const uint8_t *privkeys, int count, int compressed,
                            uint32_t *match_indices, int max_matches,
                            uint32_t *out_hit_count);

/* GPU EC only: writes count*65 pubkey bytes (prefix + X[+Y]) to out_pubs65. */
int tcuda_secp_pubkey_batch(const uint8_t *privkeys, int count, int compressed,
                            uint8_t *out_pubs65);

int tcuda_secp_selftest(void);

/* Free / total VRAM in bytes. Returns 1 on success. */
int tcuda_memory_info(uint64_t *free_bytes, uint64_t *total_bytes);

/*
 * Size GPU batch buffers from a memory budget (bytes).
 * budget_bytes==0 means auto: ~60% of currently free VRAM.
 * Returns recommended host batch size in keys (clamped).
 */
uint32_t tcuda_apply_memory_budget(uint64_t budget_bytes);

#ifdef __cplusplus
}
#endif

#endif
