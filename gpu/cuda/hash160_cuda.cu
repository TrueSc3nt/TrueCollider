/*
 * CUDA batch hash160 for 33-byte compressed public keys (phase 1).
 * EC point generation remains on CPU; this validates the GPU hash pipeline.
 */

#include "hash160_cuda.h"
#include <cuda_runtime.h>
#include <stdio.h>
#include <string.h>

#include "../../hash/sha256.h"
#include "../../hash/ripemd160.h"

#define CUDA_CHECK(call) do { \
    cudaError_t err = (call); \
    if (err != cudaSuccess) { \
        fprintf(stderr, "[CUDA] %s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(err)); \
        return 0; \
    } \
} while(0)

static __device__ __forceinline__ uint32_t rotr(uint32_t x, int n) {
    return (x >> n) | (x << (32 - n));
}

static __device__ __forceinline__ uint32_t Ch(uint32_t e, uint32_t f, uint32_t g) {
    return (e & f) ^ ((~e) & g);
}

static __device__ __forceinline__ uint32_t Maj(uint32_t a, uint32_t b, uint32_t c) {
    return (a & b) ^ (a & c) ^ (b & c);
}

static __device__ __forceinline__ uint32_t Sig0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

static __device__ __forceinline__ uint32_t Sig1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

static __device__ __forceinline__ uint32_t sig0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

static __device__ __forceinline__ uint32_t sig1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

static __device__ uint32_t load_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static __device__ void store_be32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static __device__ void sha256_oneblock(const uint8_t block[64], uint8_t out[32]) {
    static const uint32_t K[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };

    uint32_t w[64];
    for (int i = 0; i < 16; i++) w[i] = load_be32(block + i * 4);
    for (int i = 16; i < 64; i++)
        w[i] = sig1(w[i - 2]) + w[i - 7] + sig0(w[i - 15]) + w[i - 16];

    uint32_t a = 0x6a09e667, b = 0xbb67ae85, c = 0x3c6ef372, d = 0xa54ff53a;
    uint32_t e = 0x510e527f, f = 0x9b05688c, g = 0x1f83d9ab, h = 0x5be0cd19;

    for (int i = 0; i < 64; i++) {
        uint32_t T1 = h + Sig1(e) + Ch(e, f, g) + K[i] + w[i];
        uint32_t T2 = Sig0(a) + Maj(a, b, c);
        h = g; g = f; f = e; e = d + T1;
        d = c; c = b; b = a; a = T1 + T2;
    }

    store_be32(out + 0, a + 0x6a09e667);
    store_be32(out + 4, b + 0xbb67ae85);
    store_be32(out + 8, c + 0x3c6ef372);
    store_be32(out + 12, d + 0xa54ff53a);
    store_be32(out + 16, e + 0x510e527f);
    store_be32(out + 20, f + 0x9b05688c);
    store_be32(out + 24, g + 0x1f83d9ab);
    store_be32(out + 28, h + 0x5be0cd19);
}

static __device__ __forceinline__ uint32_t rmd_f(int j, uint32_t x, uint32_t y, uint32_t z) {
    if (j < 16) return x ^ y ^ z;
    if (j < 32) return (x & y) | (~x & z);
    if (j < 48) return (x | ~y) ^ z;
    if (j < 64) return (x & z) | (y & ~z);
    return x ^ (y | ~z);
}

static __device__ __forceinline__ uint32_t rol(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

static __device__ void ripemd160_oneblock(const uint8_t block[64], uint8_t out[20]) {
    static const int r[80] = {
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
        7,4,13,1,10,6,15,3,12,0,9,5,2,14,11,8,
        3,10,14,4,9,15,8,1,2,7,0,6,13,11,5,12,
        1,9,11,10,0,8,12,4,13,3,7,15,14,5,6,2,
        4,0,5,9,7,12,2,10,14,1,3,8,11,6,15,13
    };
    static const int rp[80] = {
        5,14,7,0,9,2,11,4,13,6,15,8,1,10,3,12,
        6,11,3,7,0,13,5,10,14,15,8,12,4,9,1,2,
        15,5,1,3,7,14,6,9,11,8,12,2,10,0,4,13,
        8,6,4,1,3,11,15,0,5,12,2,13,9,7,10,14,
        12,15,10,4,1,5,8,7,9,14,2,13,0,3,11,6
    };
    static const int s[80] = {
        11,14,15,12,5,8,7,9,11,13,14,15,6,7,9,8,
        7,6,8,13,11,9,7,15,7,12,15,9,11,7,13,12,
        11,13,6,7,14,9,13,15,14,8,13,6,5,12,7,5,
        11,12,14,15,14,15,9,8,9,14,5,6,8,6,5,12,
        9,15,5,11,6,8,13,12,5,12,13,14,11,8,5,6
    };
    static const int sp[80] = {
        8,9,9,11,13,15,15,5,7,7,8,11,14,14,12,6,
        9,13,15,7,12,8,9,11,7,7,12,7,6,15,13,11,
        9,7,15,11,8,6,6,14,12,13,5,14,13,13,7,5,
        15,5,8,11,14,14,6,14,6,9,12,9,12,5,15,8,
        8,5,12,9,12,5,14,6,8,13,6,5,15,13,11,11
    };
    static const uint32_t K[5] = {0,0x5a827999,0x6ed9eba1,0x8f1bbcdc,0xa953fd4e};
    static const uint32_t Kp[5] = {0x50a28be6,0x5c4dd124,0x6d703ef3,0x7a6d76e9,0};

    uint32_t w[16];
    for (int i = 0; i < 16; i++) {
        const uint8_t *p = block + i * 4;
        w[i] = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    }

    uint32_t a = 0x67452301, b = 0xefcdab89, c = 0x98badcfe, d = 0x10325476, e = 0xc3d2e1f0;
    uint32_t ap = a, bp = b, cp = c, dp = d, ep = e;

    for (int j = 0; j < 80; j++) {
        int round = j / 16;
        uint32_t t = rol(a + rmd_f(j, b, c, d) + w[r[j]] + K[round], s[j]) + e;
        a = e; e = d; d = rol(c, 10); c = b; b = t;

        t = rol(ap + rmd_f(79 - j, bp, cp, dp) + w[rp[j]] + Kp[round], sp[j]) + ep;
        ap = ep; ep = dp; dp = rol(cp, 10); cp = bp; bp = t;
    }

    uint32_t na = 0x67452301 + c + dp;
    uint32_t nb = 0xefcdab89 + d + ep;
    uint32_t nc = 0x98badcfe + e + ap;
    uint32_t nd = 0x10325476 + a + bp;
    uint32_t ne = 0xc3d2e1f0 + b + cp;

    out[0]=(uint8_t)na; out[1]=(uint8_t)(na>>8); out[2]=(uint8_t)(na>>16); out[3]=(uint8_t)(na>>24);
    out[4]=(uint8_t)nb; out[5]=(uint8_t)(nb>>8); out[6]=(uint8_t)(nb>>16); out[7]=(uint8_t)(nb>>24);
    out[8]=(uint8_t)nc; out[9]=(uint8_t)(nc>>8); out[10]=(uint8_t)(nc>>16); out[11]=(uint8_t)(nc>>24);
    out[12]=(uint8_t)nd; out[13]=(uint8_t)(nd>>8); out[14]=(uint8_t)(nd>>16); out[15]=(uint8_t)(nd>>24);
    out[16]=(uint8_t)ne; out[17]=(uint8_t)(ne>>8); out[18]=(uint8_t)(ne>>16); out[19]=(uint8_t)(ne>>24);
}

static __device__ void hash160_compressed33(const uint8_t *key33, uint8_t out20[20]) {
    uint8_t block[64];
    for (int i = 0; i < 33; i++) block[i] = key33[i];
    block[33] = 0x80;
    for (int i = 34; i < 62; i++) block[i] = 0;
    block[62] = 0x01;
    block[63] = 0x08;

    uint8_t sha[32];
    sha256_oneblock(block, sha);

    for (int i = 0; i < 32; i++) block[i] = sha[i];
    block[32] = 0x80;
    for (int i = 33; i < 56; i++) block[i] = 0;
    /* RIPEMD-160 length is little-endian bit count = 256 = 0x0100 */
    block[56] = 0x00; block[57] = 0x01; block[58] = 0x00; block[59] = 0x00;
    block[60] = 0x00; block[61] = 0x00; block[62] = 0x00; block[63] = 0x00;

    ripemd160_oneblock(block, out20);
}

__global__ void hash160_33_kernel(const uint8_t *keys, uint8_t *out, int count) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) return;
    hash160_compressed33(keys + i * 33, out + i * 20);
}

extern "C" int tcuda_hash160_33_batch(const uint8_t *host_keys, int count, uint8_t *host_out) {
    if (count <= 0 || !host_keys || !host_out) return 0;

    uint8_t *d_keys = NULL, *d_out = NULL;
    size_t keys_bytes = (size_t)count * 33;
    size_t out_bytes = (size_t)count * 20;

    CUDA_CHECK(cudaMalloc(&d_keys, keys_bytes));
    CUDA_CHECK(cudaMalloc(&d_out, out_bytes));
    CUDA_CHECK(cudaMemcpy(d_keys, host_keys, keys_bytes, cudaMemcpyHostToDevice));

    int threads = 256;
    int blocks = (count + threads - 1) / threads;
    hash160_33_kernel<<<blocks, threads>>>(d_keys, d_out, count);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    CUDA_CHECK(cudaMemcpy(host_out, d_out, out_bytes, cudaMemcpyDeviceToHost));

    cudaFree(d_keys);
    cudaFree(d_out);
    return 1;
}

extern "C" int tcuda_hash160_33_selftest(void) {
    /* Compressed pubkey of privkey=1 (secp256k1 G) */
    const uint8_t pubkey[33] = {
        0x02, 0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0, 0x62,
        0x95, 0xce, 0x87, 0x0b, 0x07, 0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28,
        0xd9, 0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98
    };
    /* hash160(compressed G) */
    static const uint8_t expected[20] = {
        0x75, 0x1e, 0x76, 0xe8, 0x19, 0x91, 0x96, 0xd4, 0x54, 0x94,
        0x1c, 0x45, 0xd1, 0xb3, 0xa3, 0x23, 0xf1, 0x43, 0x3b, 0xd6
    };

    uint8_t keys[33 * 16];
    uint8_t out[20 * 16];
    for (int i = 0; i < 16; i++) memcpy(keys + i * 33, pubkey, 33);

    if (!tcuda_hash160_33_batch(keys, 16, out)) {
        fprintf(stderr, "[E] tcuda_hash160_33_selftest: batch failed.\n");
        return 0;
    }
    if (memcmp(out, expected, 20) != 0) {
        fprintf(stderr, "[E] tcuda_hash160_33_selftest: hash mismatch.\n  got: ");
        for (int i = 0; i < 20; i++) fprintf(stderr, "%02x", out[i]);
        fprintf(stderr, "\n  exp: ");
        for (int i = 0; i < 20; i++) fprintf(stderr, "%02x", expected[i]);
        fprintf(stderr, "\n");
        return 0;
    }
    printf("[+] CUDA hash160 self-test passed.\n");
    return 1;
}
