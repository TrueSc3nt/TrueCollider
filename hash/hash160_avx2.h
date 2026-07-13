#ifndef HASH160_AVX2_H
#define HASH160_AVX2_H

#include <stdint.h>
#include <stddef.h>

/*
 * AVX2 batch hash160 (SHA-256 + RIPEMD-160) for Bitcoin public keys.
 * 8-way SIMD. Compile with -mavx2. Runtime must verify AVX2 before calling.
 */

#if !defined(NO_AVX2) && (defined(__x86_64__) || defined(_M_X64)) && !defined(TERMUX)
#define HASH160_AVX2_AVAILABLE 1
#else
#define HASH160_AVX2_AVAILABLE 0
#endif

#if HASH160_AVX2_AVAILABLE

#ifdef __cplusplus
extern "C" {
#endif

void hash160_avx2_8x33(const uint8_t *inputs[8], uint8_t *outputs[8]);
void hash160_avx2_8x65(const uint8_t *inputs[8], uint8_t *outputs[8]);
void hash160_avx2_8x22(const uint8_t *inputs[8], uint8_t *outputs[8]);
int hash160_avx2_selftest(void);

#ifdef __cplusplus
}
#endif

#endif /* HASH160_AVX2_AVAILABLE */

#endif /* HASH160_AVX2_H */
