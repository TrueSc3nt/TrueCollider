/* Host finish for Solana CUDA path — use ed25519.h only (avoid fixedint.h clash). */
#include "ed25519_cuda.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../ed25519/ed25519.h"

extern "C" int tcuda_ed25519_pubkey_batch(const uint8_t *seeds32, int count, uint8_t *pubs32) {
    if (!seeds32 || !pubs32 || count <= 0) return 0;
    /* Exercise GPU SHA512+clamp; finish pubkeys with host ed25519_create_keypair
       (ref10 ge). Scalars from GPU are validated in self-test against host. */
    uint8_t *scalars = (uint8_t *)malloc((size_t)count * 32);
    if (!scalars) return 0;
    if (!tcuda_ed25519_sha512_clamp_batch(seeds32, count, scalars)) {
        free(scalars);
        return 0;
    }
    for (int i = 0; i < count; i++) {
        uint8_t priv[64];
        ed25519_create_keypair(pubs32 + (size_t)i * 32, priv, seeds32 + (size_t)i * 32);
        /* First 32 bytes of expanded privkey must match GPU clamp. */
        if (memcmp(priv, scalars + (size_t)i * 32, 32) != 0) {
            fprintf(stderr, "[E] CUDA ed25519 SHA512/clamp mismatch at index %d\n", i);
            free(scalars);
            return 0;
        }
    }
    free(scalars);
    return 1;
}

extern "C" int tcuda_ed25519_selftest(void) {
    uint8_t seed[32];
    memset(seed, 0, 32);
    seed[31] = 1;
    uint8_t expect[32], got[32], priv[64];
    ed25519_create_keypair(expect, priv, seed);
    if (!tcuda_ed25519_pubkey_batch(seed, 1, got)) {
        fprintf(stderr, "[E] tcuda_ed25519_selftest: batch failed.\n");
        return 0;
    }
    if (memcmp(expect, got, 32) != 0) {
        fprintf(stderr, "[E] tcuda_ed25519_selftest: pubkey mismatch.\n");
        return 0;
    }
    fprintf(stderr, "[+] CUDA ed25519 (Solana) self-test passed (GPU SHA512 + host ge).\n");
    return 1;
}
