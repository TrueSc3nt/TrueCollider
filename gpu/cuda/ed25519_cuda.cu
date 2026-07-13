/*
 * CUDA Solana path — device SHA-512 of 32-byte seeds + ed25519 clamp.
 * Host ge_scalarmult_base lives in ed25519_cuda_host.cpp (MSVC, not nvcc).
 */
#include "ed25519_cuda.h"
#include <cuda_runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static __device__ __forceinline__ uint64_t rotr64(uint64_t x, int n) {
    return (x >> n) | (x << (64 - n));
}
static __device__ __forceinline__ uint64_t Ch64(uint64_t e, uint64_t f, uint64_t g) {
    return (e & f) ^ ((~e) & g);
}
static __device__ __forceinline__ uint64_t Maj64(uint64_t a, uint64_t b, uint64_t c) {
    return (a & b) ^ (a & c) ^ (b & c);
}
static __device__ __forceinline__ uint64_t Sig0_64(uint64_t x) {
    return rotr64(x, 28) ^ rotr64(x, 34) ^ rotr64(x, 39);
}
static __device__ __forceinline__ uint64_t Sig1_64(uint64_t x) {
    return rotr64(x, 14) ^ rotr64(x, 18) ^ rotr64(x, 41);
}
static __device__ __forceinline__ uint64_t sig0_64(uint64_t x) {
    return rotr64(x, 1) ^ rotr64(x, 8) ^ (x >> 7);
}
static __device__ __forceinline__ uint64_t sig1_64(uint64_t x) {
    return rotr64(x, 19) ^ rotr64(x, 61) ^ (x >> 6);
}
static __device__ uint64_t load_be64(const uint8_t *p) {
    return ((uint64_t)p[0] << 56) | ((uint64_t)p[1] << 48) |
           ((uint64_t)p[2] << 40) | ((uint64_t)p[3] << 32) |
           ((uint64_t)p[4] << 24) | ((uint64_t)p[5] << 16) |
           ((uint64_t)p[6] << 8) | (uint64_t)p[7];
}
static __device__ void store_be64(uint8_t *p, uint64_t v) {
    p[0] = (uint8_t)(v >> 56); p[1] = (uint8_t)(v >> 48);
    p[2] = (uint8_t)(v >> 40); p[3] = (uint8_t)(v >> 32);
    p[4] = (uint8_t)(v >> 24); p[5] = (uint8_t)(v >> 16);
    p[6] = (uint8_t)(v >> 8);  p[7] = (uint8_t)v;
}

static __device__ void sha512_32(const uint8_t seed[32], uint8_t out[64]) {
    static const uint64_t K[80] = {
        0x428a2f98d728ae22ULL,0x7137449123ef65cdULL,0xb5c0fbcfec4d3b2fULL,0xe9b5dba58189dbbcULL,
        0x3956c25bf348b538ULL,0x59f111f1b605d019ULL,0x923f82a4af194f9bULL,0xab1c5ed5da6d8118ULL,
        0xd807aa98a3030242ULL,0x12835b0145706fbeULL,0x243185be4ee4b28cULL,0x550c7dc3d5ffb4e2ULL,
        0x72be5d74f27b896fULL,0x80deb1fe3b1696b1ULL,0x9bdc06a725c71235ULL,0xc19bf174cf692694ULL,
        0xe49b69c19ef14ad2ULL,0xefbe4786384f25e3ULL,0x0fc19dc68b8cd5b5ULL,0x240ca1cc77ac9c65ULL,
        0x2de92c6f592b0275ULL,0x4a7484aa6ea6e483ULL,0x5cb0a9dcbd41fbd4ULL,0x76f988da831153b5ULL,
        0x983e5152ee66dfabULL,0xa831c66d2db43210ULL,0xb00327c898fb213fULL,0xbf597fc7beef0ee4ULL,
        0xc6e00bf33da88fc2ULL,0xd5a79147930aa725ULL,0x06ca6351e003826fULL,0x142929670a0e6e70ULL,
        0x27b70a8546d22ffcULL,0x2e1b21385c26c926ULL,0x4d2c6dfc5ac42aedULL,0x53380d139d95b3dfULL,
        0x650a73548baf63deULL,0x766a0abb3c77b2a8ULL,0x81c2c92e47edaee6ULL,0x92722c851482353bULL,
        0xa2bfe8a14cf10364ULL,0xa81a664bbc423001ULL,0xc24b8b70d0f89791ULL,0xc76c51a30654be30ULL,
        0xd192e819d6ef5218ULL,0xd69906245565a910ULL,0xf40e35855771202aULL,0x106aa07032bbd1b8ULL,
        0x19a4c116b8d2d0c8ULL,0x1e376c085141ab53ULL,0x2748774cdf8eeb99ULL,0x34b0bcb5e19b48a8ULL,
        0x391c0cb3c5c95a63ULL,0x4ed8aa4ae3418acbULL,0x5b9cca4f7763e373ULL,0x682e6ff3d6b2b8a3ULL,
        0x748f82ee5defb2fcULL,0x78a5636f43172f60ULL,0x84c87814a1f0ab72ULL,0x8cc702081a6439ecULL,
        0x90befffa23631e28ULL,0xa4506cebde82bde9ULL,0xbef9a3f7b2c67915ULL,0xc67178f2e372532bULL,
        0xca273eceea26619cULL,0xd186b8c721c0c207ULL,0xeada7dd6cde0eb1eULL,0xf57d4f7fee6ed178ULL,
        0x06f067aa72176fbaULL,0x0a637dc5a2c898a6ULL,0x113f9804bef90daeULL,0x1b710b35131c471bULL,
        0x28db77f523047d84ULL,0x32caab7b40c72493ULL,0x3c9ebe0a15c9bebcULL,0x431d67c49c100d4cULL,
        0x4cc5d4becb3e42b6ULL,0x597f299cfc657e2aULL,0x5fcb6fab3ad6faecULL,0x6c44198c4a475817ULL
    };
    uint64_t H[8] = {
        0x6a09e667f3bcc908ULL,0xbb67ae8584caa73bULL,0x3c6ef372fe94f82bULL,0xa54ff53a5f1d36f1ULL,
        0x510e527fade682d1ULL,0x9b05688c2b3e6c1fULL,0x1f83d9abfb41bd6bULL,0x5be0cd19137e2179ULL
    };
    uint8_t block[128];
    for (int i = 0; i < 32; i++) block[i] = seed[i];
    block[32] = 0x80;
    for (int i = 33; i < 128; i++) block[i] = 0;
    block[126] = 0x01;
    block[127] = 0x00;

    uint64_t w[80];
    for (int i = 0; i < 16; i++) w[i] = load_be64(block + i * 8);
    for (int i = 16; i < 80; i++)
        w[i] = sig1_64(w[i - 2]) + w[i - 7] + sig0_64(w[i - 15]) + w[i - 16];
    uint64_t a = H[0], b = H[1], c = H[2], d = H[3];
    uint64_t e = H[4], f = H[5], g = H[6], h = H[7];
    for (int i = 0; i < 80; i++) {
        uint64_t t1 = h + Sig1_64(e) + Ch64(e, f, g) + K[i] + w[i];
        uint64_t t2 = Sig0_64(a) + Maj64(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    H[0] += a; H[1] += b; H[2] += c; H[3] += d;
    H[4] += e; H[5] += f; H[6] += g; H[7] += h;
    for (int i = 0; i < 8; i++) store_be64(out + i * 8, H[i]);
}

__global__ void k_ed25519_sha512_clamp(const uint8_t *seeds, uint8_t *scalars, int count) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) return;
    uint8_t dig[64];
    sha512_32(seeds + (size_t)i * 32, dig);
    dig[0] &= 248;
    dig[31] &= 63;
    dig[31] |= 64;
    for (int j = 0; j < 32; j++)
        scalars[(size_t)i * 32 + j] = dig[j];
}

extern "C" int tcuda_ed25519_sha512_clamp_batch(const uint8_t *seeds32, int count, uint8_t *scalars32) {
    if (!seeds32 || !scalars32 || count <= 0) return 0;
    if (count > 1048576) count = 1048576;
    uint8_t *d_seeds = NULL, *d_scalars = NULL;
    size_t bytes = (size_t)count * 32;
    if (cudaMalloc(&d_seeds, bytes) != cudaSuccess ||
        cudaMalloc(&d_scalars, bytes) != cudaSuccess) {
        if (d_seeds) cudaFree(d_seeds);
        if (d_scalars) cudaFree(d_scalars);
        return 0;
    }
    if (cudaMemcpy(d_seeds, seeds32, bytes, cudaMemcpyHostToDevice) != cudaSuccess) {
        cudaFree(d_seeds); cudaFree(d_scalars); return 0;
    }
    int threads = 256;
    int blocks = (count + threads - 1) / threads;
    k_ed25519_sha512_clamp<<<blocks, threads>>>(d_seeds, d_scalars, count);
    if (cudaDeviceSynchronize() != cudaSuccess ||
        cudaMemcpy(scalars32, d_scalars, bytes, cudaMemcpyDeviceToHost) != cudaSuccess) {
        cudaFree(d_seeds); cudaFree(d_scalars); return 0;
    }
    cudaFree(d_seeds);
    cudaFree(d_scalars);
    return 1;
}
