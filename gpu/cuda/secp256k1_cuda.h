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

/* 1 if device hash160+bloom search path is active (after successful self-test + bloom upload). */
int tcuda_secp_device_filter(void);

/*
 * BSGS giant-step GRP: upload GSn[0..half-1] and _2GSn (each 64B = x||y BE).
 * half = CPU_GRP_SIZE/2. Returns 1 on success.
 */
int tcuda_bsgs_grp_init(const uint8_t *gsn_xy64, int half, const uint8_t *twogsn_xy64);
void tcuda_bsgs_grp_free(void);

/*
 * Run n_cycles of BSGS GRP from start_xy (64B x||y BE, updated in-place to next center).
 * Writes n_cycles * (2*half) * 32 bytes of x-coords (BE) to out_x32.
 * Returns 1 on success.
 */
int tcuda_bsgs_grp_run(uint8_t *start_xy64, int n_cycles, uint8_t *out_x32);

/* Parallel one-cycle GRP for n_starts independent centers (batched giants). */
int tcuda_bsgs_grp_run_batch(uint8_t *starts_xy64, int n_starts, uint8_t *out_x32);

int tcuda_bsgs_grp_ready(void);

/* Free / total VRAM in bytes. Returns 1 on success. */
int tcuda_memory_info(uint64_t *free_bytes, uint64_t *total_bytes);

/*
 * Size GPU batch buffers from a memory budget (bytes).
 * budget_bytes==0 means auto: ~60% of currently free VRAM.
 * Returns recommended host batch size in keys (clamped).
 */
uint32_t tcuda_apply_memory_budget(uint64_t budget_bytes);

/*
 * Kangaroo CUDA helpers.
 * Small ranges: host batches privkeys; compare device pubs to target.
 * Larger ranges: multi-walker DP kernel (EC jumps on device); host DP table / collision.
 */
int tcuda_kangaroo_scan_match(const uint8_t *privkeys, int count, int compressed,
                               const uint8_t *target_xy64, int *match_index);

int tcuda_kangaroo_dp_run(
	uint8_t *pos_xy64,          /* in/out: n_walkers * 64 affine x||y BE */
	uint8_t *dist32,            /* in/out: n_walkers * 32 BE distance */
	const int *herd,            /* n_walkers: 0=wild, 1=tame */
	int n_walkers,
	const uint8_t *jump_xy64,   /* 32 * 64 jump points */
	const uint8_t *jump_len32,  /* 32 * 32 jump lengths BE */
	uint64_t dp_mask,
	int steps_per_launch,
	uint8_t *out_dp_xy64,       /* max_dps * 64 */
	uint8_t *out_dp_dist32,     /* max_dps * 32 */
	int *out_dp_herd,           /* max_dps */
	int max_dps,
	int *n_dps_out);

#ifdef __cplusplus
}
#endif

#endif
