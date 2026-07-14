#ifndef ED25519_CUDA_H
#define ED25519_CUDA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Device SHA-512 + ed25519 clamp → count*32 scalars. Returns 1 on success. */
int tcuda_ed25519_sha512_clamp_batch(const uint8_t *seeds32, int count, uint8_t *scalars32);

/* Full on-device Solana pubkeys: SHA512+clamp + ge_scalarmult_base. */
int tcuda_ed25519_pubkey_device_batch(const uint8_t *seeds32, int count, uint8_t *pubs32);

/* Preferred entry: device ge path (falls back to host ge only if device fails). */
int tcuda_ed25519_pubkey_batch(const uint8_t *seeds32, int count, uint8_t *pubs32);

int tcuda_ed25519_selftest(void);

#ifdef __cplusplus
}
#endif

#endif
