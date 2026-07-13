#ifndef ED25519_CUDA_H
#define ED25519_CUDA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Device SHA-512 + ed25519 clamp → count*32 scalars. Returns 1 on success. */
int tcuda_ed25519_sha512_clamp_batch(const uint8_t *seeds32, int count, uint8_t *scalars32);

/* Full Solana pubkeys: CUDA SHA512+clamp, host ge_scalarmult_base. */
int tcuda_ed25519_pubkey_batch(const uint8_t *seeds32, int count, uint8_t *pubs32);

int tcuda_ed25519_selftest(void);

#ifdef __cplusplus
}
#endif

#endif
