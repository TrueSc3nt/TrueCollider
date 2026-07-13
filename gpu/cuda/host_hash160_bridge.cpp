/*
 * MSVC-compiled bridge so nvcc host code never calls C++-ABI hash helpers
 * directly (nvcc/MSVC ABI mismatch was corrupting the stack on return).
 */
#include <stdint.h>
#include "../../hash/sha256.h"
#include "../../hash/ripemd160.h"

extern "C" void tcuda_bridge_hash160(const uint8_t *pub, int compressed, uint8_t out20[20]) {
    uint8_t sha[32];
    if (compressed)
        sha256_33((uint8_t *)pub, sha);
    else
        sha256_65((uint8_t *)pub, sha);
    ripemd160_32(sha, out20);
}
