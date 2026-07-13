#ifndef HASH160_CUDA_H
#define HASH160_CUDA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Phase-1 CUDA hash160: batch SHA-256 + RIPEMD-160 on 33-byte compressed pubkeys.
 * Host supplies keys; returns 20-byte hash160 per key.
 */
int tcuda_hash160_33_selftest(void);
int tcuda_hash160_33_batch(const uint8_t *host_keys, int count, uint8_t *host_out);

#ifdef __cplusplus
}
#endif

#endif
