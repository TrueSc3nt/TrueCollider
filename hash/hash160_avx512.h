#ifndef HASH160_AVX512_H
#define HASH160_AVX512_H

#include <stdint.h>
#include <stddef.h>

/*
 * AVX-512 batch hash160 (SHA-256 + RIPEMD-160) for Bitcoin public keys.
 *
 * These functions are compiled only when:
 *   - x86_64 architecture is detected
 *   - AVX-512 is not disabled at build time
 *   - The compiler supports AVX-512BW (required for byte shuffle)
 *
 * Runtime dispatch checks CPUID + XCR0 before calling them.
 */

#if !defined(NO_AVX512) && (defined(__x86_64__) || defined(_M_X64)) && !defined(TERMUX)
#define HASH160_AVX512_AVAILABLE 1
#else
#define HASH160_AVX512_AVAILABLE 0
#endif

#if HASH160_AVX512_AVAILABLE

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Compute hash160 for 16 compressed public keys (33 bytes each).
 * inputs:  array of 16 pointers to 33-byte buffers. Each buffer must be at
 *          least 64 bytes so padding can be written in place (same convention
 *          as sha256_33).
 * outputs: array of 16 pointers to 20-byte output buffers.
 */
void hash160_avx512_16x33(const uint8_t *inputs[16], uint8_t *outputs[16]);

/*
 * Compute hash160 for 16 uncompressed public keys (65 bytes each).
 * inputs:  array of 16 pointers to 65-byte buffers. Each buffer must be at
 *          least 128 bytes so padding can be written in place.
 * outputs: array of 16 pointers to 20-byte output buffers.
 */
void hash160_avx512_16x65(const uint8_t *inputs[16], uint8_t *outputs[16]);

/*
 * Compute hash160 for 16 P2SH redeem scripts (22 bytes each).
 */
void hash160_avx512_16x22(const uint8_t *inputs[16], uint8_t *outputs[16]);

/* Self-test: compare AVX-512 output against scalar reference. */
int hash160_avx512_selftest(void);

#ifdef __cplusplus
}
#endif

#endif /* HASH160_AVX512_AVAILABLE */

#endif /* HASH160_AVX512_H */
