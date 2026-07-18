#ifndef PBKDF2_SHA512_CUDA_H
#define PBKDF2_SHA512_CUDA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Batch BIP39 PBKDF2-HMAC-SHA512 on CUDA (2048 iters → 64-byte seeds).
 * mnemonics: count null-terminated UTF-8 strings packed in host buffer
 *            (each mnemonic starts at offsets[i], length lens[i]).
 * Returns 1 on success. Host fallback if CUDA unavailable.
 */
int tcuda_pbkdf2_sha512_batch(const char *blob, const uint32_t *offsets,
                              const uint32_t *lens, uint32_t count,
                              const char *passphrase, uint8_t *out_seeds64);

/* Checksum-first BIP39 entropy validate on host (fast gate before PBKDF2). */
int research_bip39_checksum_ok_entropy(const uint8_t *ent, int ent_bytes);

#ifdef __cplusplus
}
#endif

#endif
