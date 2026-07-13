/*
 * CUDA secp256k1: field/Jacobian EC math, scalar*G, hash160, bloom probe.
 * Self-contained (duplicates minimal SHA-256 / RIPEMD-160 / XXH64 on device).
 */

#include "secp256k1_cuda.h"

#include <cuda_runtime.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CUDA_CHECK(call) do { \
    cudaError_t err_ = (call); \
    if (err_ != cudaSuccess) { \
        fprintf(stderr, "[CUDA] %s:%d: %s\n", __FILE__, __LINE__, \
                cudaGetErrorString(err_)); \
        return 0; \
    } \
} while (0)

/* ---- Host / device bloom state ---- */
static uint8_t  *g_d_bloom = NULL;
static uint64_t  g_bloom_bits = 0;
static uint64_t  g_bloom_bytes = 0;
static uint8_t   g_bloom_hashes = 0;
static int       g_bloom_ready = 0;

static uint8_t  *g_d_privkeys = NULL;
static uint8_t  *g_d_hitmask = NULL;
static int       g_cap_count = 0;
static uint8_t  *g_d_pubs = NULL;
static uint8_t  *g_h_bloom = NULL;
static int       g_host_filter = 1; /* 1 = hash+bloom on host (correct); 0 = device hash */

/* =========================================================================
 * Device field arithmetic: 8 x uint32 little-endian limbs, mod secp256k1 p
 * p = 2^256 - 2^32 - 977
 * ========================================================================= */

struct Fe {
    uint32_t d[8];
};

struct JPoint {
    Fe x, y, z;
};

__device__ __constant__ uint32_t SECP_P[8] = {
    0xFFFFFC2Fu, 0xFFFFFFFEu, 0xFFFFFFFFu, 0xFFFFFFFFu,
    0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu
};

/* Generator G (affine), LE limbs */
__device__ __constant__ uint32_t SECP_GX[8] = {
    0x16F81798u, 0x59F2815Bu, 0x2DCE28D9u, 0x029BFCDBu,
    0xCE870B07u, 0x55A06295u, 0xF9DCBBACu, 0x79BE667Eu
};
__device__ __constant__ uint32_t SECP_GY[8] = {
    0xFB10D4B8u, 0x9C47D08Fu, 0xA6855419u, 0xFD17B448u,
    0x0E1108A8u, 0x5DA4FBFCu, 0x26A3C465u, 0x483ADA77u
};

static __device__ __forceinline__ void fe_clear(Fe *a) {
#pragma unroll
    for (int i = 0; i < 8; i++) a->d[i] = 0;
}

static __device__ __forceinline__ void fe_set(Fe *r, const Fe *a) {
#pragma unroll
    for (int i = 0; i < 8; i++) r->d[i] = a->d[i];
}

static __device__ __forceinline__ void fe_set_u32(Fe *r, const uint32_t *v) {
#pragma unroll
    for (int i = 0; i < 8; i++) r->d[i] = v[i];
}

static __device__ __forceinline__ int fe_is_zero(const Fe *a) {
    uint32_t x = 0;
#pragma unroll
    for (int i = 0; i < 8; i++) x |= a->d[i];
    return x == 0;
}

static __device__ __forceinline__ int fe_cmp(const Fe *a, const Fe *b) {
    for (int i = 7; i >= 0; i--) {
        if (a->d[i] < b->d[i]) return -1;
        if (a->d[i] > b->d[i]) return 1;
    }
    return 0;
}

static __device__ __forceinline__ int fe_cmp_p(const Fe *a) {
    for (int i = 7; i >= 0; i--) {
        if (a->d[i] < SECP_P[i]) return -1;
        if (a->d[i] > SECP_P[i]) return 1;
    }
    return 0;
}

static __device__ void fe_normalize(Fe *r) {
    if (fe_cmp_p(r) >= 0) {
        uint64_t borrow = 0;
#pragma unroll
        for (int i = 0; i < 8; i++) {
            uint64_t t = (uint64_t)r->d[i] - SECP_P[i] - borrow;
            r->d[i] = (uint32_t)t;
            borrow = (t >> 63) & 1;
        }
    }
}

static __device__ void fe_add(Fe *r, const Fe *a, const Fe *b) {
    uint64_t c = 0;
#pragma unroll
    for (int i = 0; i < 8; i++) {
        c += (uint64_t)a->d[i] + b->d[i];
        r->d[i] = (uint32_t)c;
        c >>= 32;
    }
    if (c || fe_cmp_p(r) >= 0) {
        uint64_t borrow = 0;
#pragma unroll
        for (int i = 0; i < 8; i++) {
            uint64_t t = (uint64_t)r->d[i] - SECP_P[i] - borrow;
            r->d[i] = (uint32_t)t;
            borrow = (t >> 63) & 1;
        }
    }
}

static __device__ void fe_sub(Fe *r, const Fe *a, const Fe *b) {
    uint64_t borrow = 0;
#pragma unroll
    for (int i = 0; i < 8; i++) {
        uint64_t t = (uint64_t)a->d[i] - b->d[i] - borrow;
        r->d[i] = (uint32_t)t;
        borrow = (t >> 63) & 1;
    }
    if (borrow) {
        uint64_t c = 0;
#pragma unroll
        for (int i = 0; i < 8; i++) {
            c += (uint64_t)r->d[i] + SECP_P[i];
            r->d[i] = (uint32_t)c;
            c >>= 32;
        }
    }
}

/* 512-bit product in 16 x uint32 limbs -> r mod p
 * p = 2^256 - 2^32 - 977  =>  2^256 ≡ 2^32 + 977 (mod p)
 * r = lo + 977*hi + (hi << 32) */
static __device__ void fe_reduce512(Fe *r, uint64_t t[16]) {
    uint64_t c = 0;
#pragma unroll
    for (int i = 0; i < 16; i++) {
        c += t[i];
        t[i] = (uint32_t)c;
        c >>= 32;
    }

    uint64_t acc[10];
#pragma unroll
    for (int i = 0; i < 10; i++)
        acc[i] = 0;

#pragma unroll
    for (int i = 0; i < 8; i++) {
        acc[i] += t[i];
        acc[i] += t[i + 8] * 977ull;
        acc[i + 1] += t[i + 8];
    }

    c = 0;
#pragma unroll
    for (int i = 0; i < 10; i++) {
        c += acc[i];
        acc[i] = (uint32_t)c;
        c >>= 32;
    }

    if (acc[8] || acc[9]) {
        uint64_t hi0 = acc[8], hi1 = acc[9];
        acc[8] = acc[9] = 0;
        acc[0] += hi0 * 977ull;
        acc[1] += hi0 + hi1 * 977ull;
        acc[2] += hi1;
        c = 0;
#pragma unroll
        for (int i = 0; i < 10; i++) {
            c += acc[i];
            acc[i] = (uint32_t)c;
            c >>= 32;
        }
    }

#pragma unroll
    for (int i = 0; i < 8; i++)
        r->d[i] = (uint32_t)acc[i];
    fe_normalize(r);
    fe_normalize(r);
}

static __device__ void fe_mul(Fe *r, const Fe *a, const Fe *b) {
    uint64_t t[16];
#pragma unroll
    for (int i = 0; i < 16; i++)
        t[i] = 0;
#pragma unroll
    for (int i = 0; i < 8; i++) {
        uint64_t c = 0;
#pragma unroll
        for (int j = 0; j < 8; j++) {
            uint64_t u = t[i + j] + (uint64_t)a->d[i] * (uint64_t)b->d[j] + c;
            t[i + j] = (uint32_t)u;
            c = u >> 32;
        }
        t[i + 8] += c;
    }
    fe_reduce512(r, t);
}

static __device__ void fe_sqr(Fe *r, const Fe *a) {
    fe_mul(r, a, a);
}

/* r = a^(p-2) mod p via square-and-multiply */
static __device__ void fe_inv(Fe *r, const Fe *a) {
    /* p-2 limbs LE */
    const uint32_t e[8] = {
        0xFFFFFC2Du, 0xFFFFFFFEu, 0xFFFFFFFFu, 0xFFFFFFFFu,
        0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu
    };
    Fe base, acc;
    fe_set(&base, a);
    fe_clear(&acc);
    acc.d[0] = 1;

    for (int i = 0; i < 8; i++) {
        uint32_t w = e[i];
        for (int bit = 0; bit < 32; bit++) {
            if (w & 1u) {
                Fe t;
                fe_mul(&t, &acc, &base);
                fe_set(&acc, &t);
            }
            Fe s;
            fe_sqr(&s, &base);
            fe_set(&base, &s);
            w >>= 1;
        }
    }
    fe_set(r, &acc);
}

static __device__ void fe_from_bytes_be(Fe *r, const uint8_t *b) {
#pragma unroll
    for (int i = 0; i < 8; i++) {
        int o = 28 - 4 * i;
        r->d[i] = ((uint32_t)b[o] << 24) | ((uint32_t)b[o + 1] << 16) |
                  ((uint32_t)b[o + 2] << 8) | (uint32_t)b[o + 3];
    }
}

static __device__ void fe_to_bytes_be(const Fe *a, uint8_t *b) {
#pragma unroll
    for (int i = 0; i < 8; i++) {
        int o = 28 - 4 * i;
        uint32_t v = a->d[i];
        b[o]     = (uint8_t)(v >> 24);
        b[o + 1] = (uint8_t)(v >> 16);
        b[o + 2] = (uint8_t)(v >> 8);
        b[o + 3] = (uint8_t)v;
    }
}

static __device__ int fe_is_odd(const Fe *a) {
    return (int)(a->d[0] & 1u);
}

/* =========================================================================
 * Jacobian point arithmetic (secp256k1, a=0)
 * ========================================================================= */

static __device__ void jp_set_infty(JPoint *p) {
    fe_clear(&p->x);
    fe_clear(&p->y);
    fe_clear(&p->z);
    p->y.d[0] = 1; /* conventional; z=0 marks infinity */
}

static __device__ int jp_is_infty(const JPoint *p) {
    return fe_is_zero(&p->z);
}

static __device__ void jp_set_affine(JPoint *p, const Fe *x, const Fe *y) {
    fe_set(&p->x, x);
    fe_set(&p->y, y);
    fe_clear(&p->z);
    p->z.d[0] = 1;
}

static __device__ void jp_double(JPoint *r, const JPoint *p) {
    if (jp_is_infty(p) || fe_is_zero(&p->y)) {
        jp_set_infty(r);
        return;
    }
    Fe A, B, C, D, E, F, X3, Y3, Z3, t1;

    fe_sqr(&A, &p->x);          /* A = X^2 */
    fe_add(&t1, &A, &A);
    fe_add(&A, &t1, &A);        /* A = 3*X^2  (a=0) */

    fe_sqr(&B, &p->y);          /* B = Y^2 */
    fe_mul(&C, &p->x, &B);
    fe_add(&C, &C, &C);
    fe_add(&C, &C, &C);         /* C = 4*X*Y^2 */

    fe_sqr(&D, &B);
    fe_add(&D, &D, &D);
    fe_add(&D, &D, &D);
    fe_add(&D, &D, &D);         /* D = 8*Y^4 */

    fe_sqr(&E, &A);
    fe_sub(&E, &E, &C);
    fe_sub(&X3, &E, &C);        /* X3 = A^2 - 2C */

    fe_sub(&F, &C, &X3);
    fe_mul(&t1, &A, &F);
    fe_sub(&Y3, &t1, &D);       /* Y3 = A*(C-X3) - D */

    fe_add(&t1, &p->y, &p->y);
    fe_mul(&Z3, &t1, &p->z);    /* Z3 = 2*Y*Z */

    fe_set(&r->x, &X3);
    fe_set(&r->y, &Y3);
    fe_set(&r->z, &Z3);
}

static __device__ void jp_add(JPoint *r, const JPoint *p, const JPoint *q) {
    if (jp_is_infty(p)) { *r = *q; return; }
    if (jp_is_infty(q)) { *r = *p; return; }

    Fe Z1Z1, Z2Z2, U1, U2, S1, S2, H, I, J, r2, V, X3, Y3, Z3, t;

    fe_sqr(&Z1Z1, &p->z);
    fe_sqr(&Z2Z2, &q->z);
    fe_mul(&U1, &p->x, &Z2Z2);
    fe_mul(&U2, &q->x, &Z1Z1);
    fe_mul(&t, &q->z, &Z2Z2);
    fe_mul(&S1, &p->y, &t);
    fe_mul(&t, &p->z, &Z1Z1);
    fe_mul(&S2, &q->y, &t);
    fe_sub(&H, &U2, &U1);
    fe_sub(&t, &S2, &S1);

    if (fe_is_zero(&H)) {
        if (fe_is_zero(&t)) {
            jp_double(r, p);
            return;
        }
        jp_set_infty(r);
        return;
    }

    fe_add(&I, &H, &H);
    fe_sqr(&I, &I);             /* I = (2H)^2 */
    fe_mul(&J, &H, &I);         /* J = H*I */
    fe_set(&r2, &t);
    fe_add(&r2, &r2, &r2);      /* r = 2*(S2-S1) */
    fe_mul(&V, &U1, &I);

    fe_sqr(&X3, &r2);
    fe_sub(&X3, &X3, &J);
    fe_sub(&X3, &X3, &V);
    fe_sub(&X3, &X3, &V);       /* X3 = r^2 - J - 2V */

    fe_sub(&t, &V, &X3);
    fe_mul(&Y3, &r2, &t);
    fe_mul(&t, &S1, &J);
    fe_add(&t, &t, &t);
    fe_sub(&Y3, &Y3, &t);       /* Y3 = r*(V-X3) - 2*S1*J */

    fe_add(&t, &p->z, &q->z);
    fe_sqr(&Z3, &t);
    fe_sub(&Z3, &Z3, &Z1Z1);
    fe_sub(&Z3, &Z3, &Z2Z2);
    fe_mul(&Z3, &Z3, &H);       /* Z3 = ((Z1+Z2)^2 - Z1Z1 - Z2Z2)*H */

    fe_set(&r->x, &X3);
    fe_set(&r->y, &Y3);
    fe_set(&r->z, &Z3);
}

static __device__ void jp_to_affine(Fe *x, Fe *y, const JPoint *p) {
    if (jp_is_infty(p)) {
        fe_clear(x);
        fe_clear(y);
        return;
    }
    int z_one = (p->z.d[0] == 1u);
#pragma unroll
    for (int i = 1; i < 8; i++) z_one &= (p->z.d[i] == 0u);
    if (z_one) {
        fe_set(x, &p->x);
        fe_set(y, &p->y);
        return;
    }
    Fe zinv, z2, z3;
    fe_inv(&zinv, &p->z);
    fe_sqr(&z2, &zinv);
    fe_mul(&z3, &z2, &zinv);
    fe_mul(x, &p->x, &z2);
    fe_mul(y, &p->y, &z3);
}

/* Binary scalar multiply of G: k is 32-byte big-endian */
static __device__ void scalar_mul_g(Fe *ox, Fe *oy, const uint8_t *k32) {
    JPoint R, G;
    Fe gx, gy;
    fe_set_u32(&gx, SECP_GX);
    fe_set_u32(&gy, SECP_GY);
    jp_set_affine(&G, &gx, &gy);
    jp_set_infty(&R);

    for (int byte = 0; byte < 32; byte++) {
        uint8_t b = k32[byte];
        for (int bit = 7; bit >= 0; bit--) {
            jp_double(&R, &R);
            if ((b >> bit) & 1) {
                JPoint T;
                jp_add(&T, &R, &G);
                R = T;
            }
        }
    }
    jp_to_affine(ox, oy, &R);
}

/* =========================================================================
 * SHA-256 / RIPEMD-160 / hash160 (device)
 * ========================================================================= */

static __device__ __forceinline__ uint32_t rotr32(uint32_t x, int n) {
    return (x >> n) | (x << (32 - n));
}

static __device__ __forceinline__ uint32_t load_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static __device__ __forceinline__ void store_be32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

static __device__ __forceinline__ uint32_t load_le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static __device__ __forceinline__ void store_le32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static __device__ void sha256_transform(uint32_t state[8], const uint8_t block[64]) {
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
    for (int i = 16; i < 64; i++) {
        uint32_t s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

    for (int i = 0; i < 64; i++) {
        uint32_t S1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t T1 = h + S1 + ch + K[i] + w[i];
        uint32_t S0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t T2 = S0 + maj;
        h = g; g = f; f = e; e = d + T1;
        d = c; c = b; b = a; a = T1 + T2;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

static __device__ void sha256_oneblock(const uint8_t block[64], uint8_t out[32]) {
    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    sha256_transform(state, block);
#pragma unroll
    for (int i = 0; i < 8; i++)
        store_be32(out + i * 4, state[i]);
}

static __device__ __forceinline__ uint32_t rmd_f(int j, uint32_t x, uint32_t y, uint32_t z) {
    if (j < 16) return x ^ y ^ z;
    if (j < 32) return (x & y) | (~x & z);
    if (j < 48) return (x | ~y) ^ z;
    if (j < 64) return (x & z) | (y & ~z);
    return x ^ (y | ~z);
}

static __device__ __forceinline__ uint32_t rol32(uint32_t x, int n) {
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
    static const uint32_t K[5]  = {0, 0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xa953fd4e};
    static const uint32_t Kp[5] = {0x50a28be6, 0x5c4dd124, 0x6d703ef3, 0x7a6d76e9, 0};

    uint32_t w[16];
#pragma unroll
    for (int i = 0; i < 16; i++)
        w[i] = load_le32(block + i * 4);

    uint32_t a = 0x67452301, b = 0xefcdab89, c = 0x98badcfe, d = 0x10325476, e = 0xc3d2e1f0;
    uint32_t ap = a, bp = b, cp = c, dp = d, ep = e;

    for (int j = 0; j < 80; j++) {
        int round = j / 16;
        uint32_t t = rol32(a + rmd_f(j, b, c, d) + w[r[j]] + K[round], s[j]) + e;
        a = e; e = d; d = rol32(c, 10); c = b; b = t;

        t = rol32(ap + rmd_f(79 - j, bp, cp, dp) + w[rp[j]] + Kp[round], sp[j]) + ep;
        ap = ep; ep = dp; dp = rol32(cp, 10); cp = bp; bp = t;
    }

    uint32_t na = 0x67452301 + c + dp;
    uint32_t nb = 0xefcdab89 + d + ep;
    uint32_t nc = 0x98badcfe + e + ap;
    uint32_t nd = 0x10325476 + a + bp;
    uint32_t ne = 0xc3d2e1f0 + b + cp;

    store_le32(out + 0, na);
    store_le32(out + 4, nb);
    store_le32(out + 8, nc);
    store_le32(out + 12, nd);
    store_le32(out + 16, ne);
}

static __device__ void hash160_compressed33(const uint8_t *key33, uint8_t out20[20]) {
    uint8_t block[64];
#pragma unroll
    for (int i = 0; i < 33; i++) block[i] = key33[i];
    block[33] = 0x80;
#pragma unroll
    for (int i = 34; i < 62; i++) block[i] = 0;
    block[62] = 0x01;
    block[63] = 0x08;

    uint8_t sha[32];
    sha256_oneblock(block, sha);

#pragma unroll
    for (int i = 0; i < 32; i++) block[i] = sha[i];
    block[32] = 0x80;
#pragma unroll
    for (int i = 33; i < 56; i++) block[i] = 0;
    /* RIPEMD-160 length is little-endian bit count = 256 */
    block[56] = 0x00; block[57] = 0x01; block[58] = 0x00; block[59] = 0x00;
    block[60] = 0x00; block[61] = 0x00; block[62] = 0x00; block[63] = 0x00;

    ripemd160_oneblock(block, out20);
}

static __device__ void hash160_uncompressed65(const uint8_t *key65, uint8_t out20[20]) {
    uint8_t block[64];
    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
#pragma unroll
    for (int i = 0; i < 64; i++) block[i] = key65[i];
    sha256_transform(state, block);

    block[0] = key65[64];
    block[1] = 0x80;
#pragma unroll
    for (int i = 2; i < 56; i++) block[i] = 0;
    /* bit length 520 = 0x208, big-endian in SHA-256 */
    block[56] = 0; block[57] = 0; block[58] = 0; block[59] = 0;
    block[60] = 0; block[61] = 0; block[62] = 0x02; block[63] = 0x08;
    sha256_transform(state, block);

    uint8_t sha[32];
#pragma unroll
    for (int i = 0; i < 8; i++)
        store_be32(sha + i * 4, state[i]);

#pragma unroll
    for (int i = 0; i < 32; i++) block[i] = sha[i];
    block[32] = 0x80;
#pragma unroll
    for (int i = 33; i < 56; i++) block[i] = 0;
    block[56] = 0x00; block[57] = 0x01; block[58] = 0x00; block[59] = 0x00;
    block[60] = 0x00; block[61] = 0x00; block[62] = 0x00; block[63] = 0x00;
    ripemd160_oneblock(block, out20);
}

/* =========================================================================
 * XXH64 (device) — matches xxHash / bloom.cpp usage
 * ========================================================================= */

static __device__ __forceinline__ uint64_t xxh_rotl64(uint64_t x, int r) {
    return (x << r) | (x >> (64 - r));
}

static __device__ __forceinline__ uint64_t xxh_get64(const uint8_t *p) {
    return (uint64_t)p[0] | ((uint64_t)p[1] << 8) | ((uint64_t)p[2] << 16) |
           ((uint64_t)p[3] << 24) | ((uint64_t)p[4] << 32) | ((uint64_t)p[5] << 40) |
           ((uint64_t)p[6] << 48) | ((uint64_t)p[7] << 56);
}

static __device__ __forceinline__ uint32_t xxh_get32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static __device__ uint64_t xxh64(const void *input, size_t len, uint64_t seed) {
    const uint64_t PRIME64_1 = 0x9E3779B185EBCA87ULL;
    const uint64_t PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;
    const uint64_t PRIME64_3 = 0x165667B19E3779F9ULL;
    const uint64_t PRIME64_4 = 0x85EBCA77C2B2AE63ULL;
    const uint64_t PRIME64_5 = 0x27D4EB2F165667C5ULL;

    const uint8_t *p = (const uint8_t *)input;
    const uint8_t *const end = p + len;
    uint64_t h64;

    if (len >= 32) {
        const uint8_t *const limit = end - 32;
        uint64_t v1 = seed + PRIME64_1 + PRIME64_2;
        uint64_t v2 = seed + PRIME64_2;
        uint64_t v3 = seed + 0;
        uint64_t v4 = seed - PRIME64_1;

        do {
            v1 = xxh_rotl64(v1 + xxh_get64(p) * PRIME64_2, 31) * PRIME64_1;
            p += 8;
            v2 = xxh_rotl64(v2 + xxh_get64(p) * PRIME64_2, 31) * PRIME64_1;
            p += 8;
            v3 = xxh_rotl64(v3 + xxh_get64(p) * PRIME64_2, 31) * PRIME64_1;
            p += 8;
            v4 = xxh_rotl64(v4 + xxh_get64(p) * PRIME64_2, 31) * PRIME64_1;
            p += 8;
        } while (p <= limit);

        h64 = xxh_rotl64(v1, 1) + xxh_rotl64(v2, 7) +
              xxh_rotl64(v3, 12) + xxh_rotl64(v4, 18);

        v1 = xxh_rotl64(v1 * PRIME64_2, 31) * PRIME64_1;
        h64 ^= v1;
        h64 = h64 * PRIME64_1 + PRIME64_4;
        v2 = xxh_rotl64(v2 * PRIME64_2, 31) * PRIME64_1;
        h64 ^= v2;
        h64 = h64 * PRIME64_1 + PRIME64_4;
        v3 = xxh_rotl64(v3 * PRIME64_2, 31) * PRIME64_1;
        h64 ^= v3;
        h64 = h64 * PRIME64_1 + PRIME64_4;
        v4 = xxh_rotl64(v4 * PRIME64_2, 31) * PRIME64_1;
        h64 ^= v4;
        h64 = h64 * PRIME64_1 + PRIME64_4;
    } else {
        h64 = seed + PRIME64_5;
    }

    h64 += (uint64_t)len;

    while (p + 8 <= end) {
        uint64_t k1 = xxh_rotl64(xxh_get64(p) * PRIME64_2, 31) * PRIME64_1;
        h64 ^= k1;
        h64 = xxh_rotl64(h64, 27) * PRIME64_1 + PRIME64_4;
        p += 8;
    }
    if (p + 4 <= end) {
        h64 ^= (uint64_t)xxh_get32(p) * PRIME64_1;
        h64 = xxh_rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
        p += 4;
    }
    while (p < end) {
        h64 ^= (*p++) * PRIME64_5;
        h64 = xxh_rotl64(h64, 11) * PRIME64_1;
    }

    h64 ^= h64 >> 33;
    h64 *= PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= PRIME64_3;
    h64 ^= h64 >> 32;
    return h64;
}

static __device__ int bloom_check_dev(const uint8_t *bf, uint64_t bits, uint8_t hashes,
                                      const uint8_t *buf, int len) {
    uint64_t a = xxh64(buf, (size_t)len, 0x59f2815b16f81798ULL);
    uint64_t b = xxh64(buf, (size_t)len, a);
    for (uint8_t i = 0; i < hashes; i++) {
        uint64_t x = (a + b * (uint64_t)i) % bits;
        uint64_t byte = x >> 3;
        uint8_t mask = (uint8_t)(1u << (x & 7));
        if ((bf[byte] & mask) == 0)
            return 0;
    }
    return 1;
}

/* =========================================================================
 * Kernels
 * ========================================================================= */

__global__ void secp_pubkey_kernel(const uint8_t *privkeys, int count, int compressed,
                                   uint8_t *pubs) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) return;

    const uint8_t *k = privkeys + (size_t)i * 32;
    int nz = 0;
#pragma unroll
    for (int j = 0; j < 32; j++) nz |= k[j];
    uint8_t *out = pubs + (size_t)i * 65;
#pragma unroll
    for (int j = 0; j < 65; j++) out[j] = 0;
    if (!nz) return;

    Fe x, y;
    scalar_mul_g(&x, &y, k);

    if (compressed) {
        out[0] = fe_is_odd(&y) ? 0x03 : 0x02;
        fe_to_bytes_be(&x, out + 1);
    } else {
        out[0] = 0x04;
        fe_to_bytes_be(&x, out + 1);
        fe_to_bytes_be(&y, out + 33);
    }
}

__global__ void secp_search_kernel(const uint8_t *privkeys, int count, int compressed,
                                   const uint8_t *bloom, uint64_t bloom_bits,
                                   uint8_t bloom_hashes, uint8_t *hitmask) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= count) return;
    hitmask[i] = 0;

    const uint8_t *k = privkeys + (size_t)i * 32;
    int nz = 0;
#pragma unroll
    for (int j = 0; j < 32; j++) nz |= k[j];
    if (!nz) return;

    Fe x, y;
    scalar_mul_g(&x, &y, k);

    uint8_t h160[20];
    if (compressed) {
        uint8_t pub[33];
        pub[0] = fe_is_odd(&y) ? 0x03 : 0x02;
        fe_to_bytes_be(&x, pub + 1);
        hash160_compressed33(pub, h160);
    } else {
        uint8_t pub[65];
        pub[0] = 0x04;
        fe_to_bytes_be(&x, pub + 1);
        fe_to_bytes_be(&y, pub + 33);
        hash160_uncompressed65(pub, h160);
    }

    if (bloom_check_dev(bloom, bloom_bits, bloom_hashes, h160, 20))
        hitmask[i] = 1;
}

__global__ void secp_selftest_kernel(uint8_t *pub33, uint8_t *h160) {
    uint8_t k[32];
#pragma unroll
    for (int i = 0; i < 32; i++) k[i] = 0;
    k[31] = 1;

    Fe x, y;
    scalar_mul_g(&x, &y, k);
    pub33[0] = fe_is_odd(&y) ? 0x03 : 0x02;
    fe_to_bytes_be(&x, pub33 + 1);
    hash160_compressed33(pub33, h160);
}

/* =========================================================================
 * Host helpers (XXH64 / bloom bit set for selftest)
 * ========================================================================= */

static uint64_t host_rotl64(uint64_t x, int r) {
    return (x << r) | (x >> (64 - r));
}

static uint64_t host_get64(const uint8_t *p) {
    uint64_t v;
    memcpy(&v, p, 8);
    return v;
}

static uint32_t host_get32(const uint8_t *p) {
    uint32_t v;
    memcpy(&v, p, 4);
    return v;
}

static uint64_t host_xxh64(const void *input, size_t len, uint64_t seed) {
    const uint64_t PRIME64_1 = 0x9E3779B185EBCA87ULL;
    const uint64_t PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;
    const uint64_t PRIME64_3 = 0x165667B19E3779F9ULL;
    const uint64_t PRIME64_4 = 0x85EBCA77C2B2AE63ULL;
    const uint64_t PRIME64_5 = 0x27D4EB2F165667C5ULL;

    const uint8_t *p = (const uint8_t *)input;
    const uint8_t *const end = p + len;
    uint64_t h64;

    if (len >= 32) {
        const uint8_t *const limit = end - 32;
        uint64_t v1 = seed + PRIME64_1 + PRIME64_2;
        uint64_t v2 = seed + PRIME64_2;
        uint64_t v3 = seed + 0;
        uint64_t v4 = seed - PRIME64_1;
        do {
            v1 = host_rotl64(v1 + host_get64(p) * PRIME64_2, 31) * PRIME64_1; p += 8;
            v2 = host_rotl64(v2 + host_get64(p) * PRIME64_2, 31) * PRIME64_1; p += 8;
            v3 = host_rotl64(v3 + host_get64(p) * PRIME64_2, 31) * PRIME64_1; p += 8;
            v4 = host_rotl64(v4 + host_get64(p) * PRIME64_2, 31) * PRIME64_1; p += 8;
        } while (p <= limit);
        h64 = host_rotl64(v1, 1) + host_rotl64(v2, 7) +
              host_rotl64(v3, 12) + host_rotl64(v4, 18);
        v1 = host_rotl64(v1 * PRIME64_2, 31) * PRIME64_1;
        h64 ^= v1; h64 = h64 * PRIME64_1 + PRIME64_4;
        v2 = host_rotl64(v2 * PRIME64_2, 31) * PRIME64_1;
        h64 ^= v2; h64 = h64 * PRIME64_1 + PRIME64_4;
        v3 = host_rotl64(v3 * PRIME64_2, 31) * PRIME64_1;
        h64 ^= v3; h64 = h64 * PRIME64_1 + PRIME64_4;
        v4 = host_rotl64(v4 * PRIME64_2, 31) * PRIME64_1;
        h64 ^= v4; h64 = h64 * PRIME64_1 + PRIME64_4;
    } else {
        h64 = seed + PRIME64_5;
    }
    h64 += (uint64_t)len;
    while (p + 8 <= end) {
        uint64_t k1 = host_rotl64(host_get64(p) * PRIME64_2, 31) * PRIME64_1;
        h64 ^= k1;
        h64 = host_rotl64(h64, 27) * PRIME64_1 + PRIME64_4;
        p += 8;
    }
    if (p + 4 <= end) {
        h64 ^= (uint64_t)host_get32(p) * PRIME64_1;
        h64 = host_rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
        p += 4;
    }
    while (p < end) {
        h64 ^= (*p++) * PRIME64_5;
        h64 = host_rotl64(h64, 11) * PRIME64_1;
    }
    h64 ^= h64 >> 33;
    h64 *= PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= PRIME64_3;
    h64 ^= h64 >> 32;
    return h64;
}

static void host_bloom_set(uint8_t *bf, uint64_t bits, uint8_t hashes,
                           const uint8_t *buf, int len) {
    uint64_t a = host_xxh64(buf, (size_t)len, 0x59f2815b16f81798ULL);
    uint64_t b = host_xxh64(buf, (size_t)len, a);
    for (uint8_t i = 0; i < hashes; i++) {
        uint64_t x = (a + b * (uint64_t)i) % bits;
        bf[x >> 3] |= (uint8_t)(1u << (x & 7));
    }
}

static int ensure_batch_bufs(int count) {
    if (count <= g_cap_count && g_d_privkeys && g_d_pubs && g_d_hitmask)
        return 1;
    if (g_d_privkeys) { cudaFree(g_d_privkeys); g_d_privkeys = NULL; }
    if (g_d_pubs) { cudaFree(g_d_pubs); g_d_pubs = NULL; }
    if (g_d_hitmask)  { cudaFree(g_d_hitmask);  g_d_hitmask = NULL; }
    g_cap_count = 0;
    if (cudaMalloc(&g_d_privkeys, (size_t)count * 32) != cudaSuccess) return 0;
    if (cudaMalloc(&g_d_pubs, (size_t)count * 65) != cudaSuccess) return 0;
    if (cudaMalloc(&g_d_hitmask, (size_t)count) != cudaSuccess) return 0;
    g_cap_count = count;
    return 1;
}

/* =========================================================================
 * Public C API
 * ========================================================================= */

extern "C" int tcuda_secp_search_init(const uint8_t *bloom_bf, uint64_t bloom_bits,
                                      uint64_t bloom_bytes, uint8_t bloom_hashes) {
    if (!bloom_bf || bloom_bits == 0 || bloom_bytes == 0 || bloom_hashes == 0)
        return 0;
    tcuda_secp_search_free();
    /* Host-filter mode: keep bloom on host only (device hash160 is currently wrong). */
    g_host_filter = 1;
    free(g_h_bloom);
    g_h_bloom = (uint8_t*)malloc(bloom_bytes);
    if (!g_h_bloom) return 0;
    memcpy(g_h_bloom, bloom_bf, bloom_bytes);
    g_bloom_bits = bloom_bits;
    g_bloom_bytes = bloom_bytes;
    g_bloom_hashes = bloom_hashes;
    g_bloom_ready = 1;
    return 1;
}

extern "C" void tcuda_secp_search_free(void) {
    if (g_d_bloom) { cudaFree(g_d_bloom); g_d_bloom = NULL; }
    if (g_d_privkeys) { cudaFree(g_d_privkeys); g_d_privkeys = NULL; }
    if (g_d_pubs) { cudaFree(g_d_pubs); g_d_pubs = NULL; }
    if (g_d_hitmask) { cudaFree(g_d_hitmask); g_d_hitmask = NULL; }
    free(g_h_bloom); g_h_bloom = NULL;
    g_bloom_bits = 0;
    g_bloom_bytes = 0;
    g_bloom_hashes = 0;
    g_bloom_ready = 0;
    g_cap_count = 0;
}

/* Host bloom probe only (hashing is done in MSVC dispatcher code). */
static int host_bloom_check(const uint8_t *buf, int len) {
    if (!g_h_bloom || g_bloom_bits == 0) return 0;
    uint64_t a = host_xxh64(buf, (size_t)len, 0x59f2815b16f81798ULL);
    uint64_t b = host_xxh64(buf, (size_t)len, a);
    for (uint8_t i = 0; i < g_bloom_hashes; i++) {
        uint64_t x = (a + b * (uint64_t)i) % g_bloom_bits;
        if ((g_h_bloom[x >> 3] & (uint8_t)(1u << (x & 7))) == 0)
            return 0;
    }
    return 1;
}

extern "C" int tcuda_secp_pubkey_batch(const uint8_t *privkeys, int count, int compressed,
                                       uint8_t *out_pubs65) {
    if (!privkeys || !out_pubs65 || count <= 0)
        return -1;
    if (!ensure_batch_bufs(count))
        return -1;

    /* Chunked launches stay under Windows TDR / device stack limits. */
    const int chunk = 128;
    for (int off = 0; off < count; off += chunk) {
        int n = count - off;
        if (n > chunk) n = chunk;
        size_t key_bytes = (size_t)n * 32;
        if (cudaMemcpy(g_d_privkeys, privkeys + (size_t)off * 32, key_bytes,
                       cudaMemcpyHostToDevice) != cudaSuccess)
            return -1;
        int threads = n;
        int blocks = 1;
        secp_pubkey_kernel<<<blocks, threads>>>(g_d_privkeys, n, compressed ? 1 : 0, g_d_pubs);
        if (cudaGetLastError() != cudaSuccess) return -1;
        if (cudaDeviceSynchronize() != cudaSuccess) return -1;
        if (cudaMemcpy(out_pubs65 + (size_t)off * 65, g_d_pubs, (size_t)n * 65,
                       cudaMemcpyDeviceToHost) != cudaSuccess)
            return -1;
    }
    return 0;
}

extern "C" int tcuda_secp_search_batch(const uint8_t *privkeys, int count, int compressed,
                                       uint32_t *match_indices, int max_matches,
                                       uint32_t *out_hit_count) {
    /* Host-filter mode: EC on GPU only. Caller (dispatcher) must hash+bloom.
     * This entry point is kept for the optional full-device path. */
    if (!g_bloom_ready || !privkeys || count <= 0)
        return -1;
    if (g_host_filter)
        return -2; /* use tcuda_secp_pubkey_batch + host filter */

    if (!ensure_batch_bufs(count))
        return -1;

    size_t key_bytes = (size_t)count * 32;
    if (cudaMemcpy(g_d_privkeys, privkeys, key_bytes, cudaMemcpyHostToDevice) != cudaSuccess)
        return -1;

    int threads = (count < 256) ? count : 256;
    if (threads < 1) threads = 1;
    int blocks = (count + threads - 1) / threads;

    secp_search_kernel<<<blocks, threads>>>(
        g_d_privkeys, count, compressed ? 1 : 0,
        g_d_bloom, g_bloom_bits, g_bloom_hashes, g_d_hitmask);
    if (cudaGetLastError() != cudaSuccess) return -1;
    if (cudaDeviceSynchronize() != cudaSuccess) return -1;

    uint8_t *hitmask = (uint8_t *)malloc((size_t)count);
    if (!hitmask) return -1;
    if (cudaMemcpy(hitmask, g_d_hitmask, (size_t)count, cudaMemcpyDeviceToHost) != cudaSuccess) {
        free(hitmask);
        return -1;
    }

    uint32_t hits = 0;
    uint32_t stored = 0;
    for (int i = 0; i < count; i++) {
        if (!hitmask[i]) continue;
        hits++;
        if (match_indices && stored < (uint32_t)max_matches)
            match_indices[stored++] = (uint32_t)i;
    }
    free(hitmask);
    if (out_hit_count)
        *out_hit_count = hits;
    return (int)hits;
}

extern "C" int tcuda_secp_selftest(void) {
    /* Known compressed pubkey for privkey = 1 (secp256k1 G) */
    static const uint8_t expect_pub[33] = {
        0x02, 0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0, 0x62,
        0x95, 0xce, 0x87, 0x0b, 0x07, 0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28,
        0xd9, 0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98
    };

    uint8_t *d_pub = NULL, *d_h = NULL;
    uint8_t pub[33], h160_dev[20];
    CUDA_CHECK(cudaMalloc(&d_pub, 33));
    CUDA_CHECK(cudaMalloc(&d_h, 20));
    secp_selftest_kernel<<<1, 1>>>(d_pub, d_h);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    CUDA_CHECK(cudaMemcpy(pub, d_pub, 33, cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(h160_dev, d_h, 20, cudaMemcpyDeviceToHost));
    cudaFree(d_pub);
    cudaFree(d_h);

    if (memcmp(pub, expect_pub, 33) != 0) {
        fprintf(stderr, "[E] tcuda_secp_selftest: pubkey mismatch.\n  got: ");
        for (int i = 0; i < 33; i++) fprintf(stderr, "%02x", pub[i]);
        fprintf(stderr, "\n  exp: ");
        for (int i = 0; i < 33; i++) fprintf(stderr, "%02x", expect_pub[i]);
        fprintf(stderr, "\n");
        fflush(stderr);
        return 0;
    }

    /* Device hash160 is known-bad; host hashing runs in MSVC code (dispatcher). */
    g_host_filter = 1;
    fprintf(stderr, "[+] CUDA secp256k1 self-test passed (GPU EC ok; host filter mode).\n");
    fflush(stderr);
    return 1;
}
