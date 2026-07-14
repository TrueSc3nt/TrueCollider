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
static int       g_launch_chunk = 16384; /* TDR-safe kernel chunk (keys) */
static uint64_t  g_mem_budget_bytes = 0;
static uint32_t  g_recommended_batch = 8192;
static uint8_t  *g_h_pin_priv = NULL;
static uint8_t  *g_h_pin_pub = NULL;
static int       g_pin_cap = 0;
static const int kCudaThreads = 256;

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

static __device__ void ripemd160_transform(uint32_t s[5], const uint8_t chunk[64]) {
    uint32_t a1 = s[0], b1 = s[1], c1 = s[2], d1 = s[3], e1 = s[4];
    uint32_t a2 = a1, b2 = b1, c2 = c1, d2 = d1, e2 = e1;
    uint32_t w[16];
#pragma unroll
    for (int i = 0; i < 16; i++)
        w[i] = (uint32_t)chunk[i*4] | ((uint32_t)chunk[i*4+1]<<8) |
               ((uint32_t)chunk[i*4+2]<<16) | ((uint32_t)chunk[i*4+3]<<24);

#define ROL(x,n) (((x)<<(n))|((x)>>(32-(n))))
#define f1(x,y,z) ((x)^(y)^(z))
#define f2(x,y,z) (((x)&(y))|(~(x)&(z)))
#define f3(x,y,z) (((x)|~(y))^(z))
#define f4(x,y,z) (((x)&(z))|(~(z)&(y)))
#define f5(x,y,z) ((x)^((y)|~(z)))
#define Round(a,b,c,d,e,f,x,k,r) do { a = ROL(a + (f) + (x) + (k), (r)) + (e); c = ROL(c, 10); } while(0)
#define R11(a,b,c,d,e,x,r) Round(a,b,c,d,e,f1(b,c,d),x,0,r)
#define R21(a,b,c,d,e,x,r) Round(a,b,c,d,e,f2(b,c,d),x,0x5A827999u,r)
#define R31(a,b,c,d,e,x,r) Round(a,b,c,d,e,f3(b,c,d),x,0x6ED9EBA1u,r)
#define R41(a,b,c,d,e,x,r) Round(a,b,c,d,e,f4(b,c,d),x,0x8F1BBCDCu,r)
#define R51(a,b,c,d,e,x,r) Round(a,b,c,d,e,f5(b,c,d),x,0xA953FD4Eu,r)
#define R12(a,b,c,d,e,x,r) Round(a,b,c,d,e,f5(b,c,d),x,0x50A28BE6u,r)
#define R22(a,b,c,d,e,x,r) Round(a,b,c,d,e,f4(b,c,d),x,0x5C4DD124u,r)
#define R32(a,b,c,d,e,x,r) Round(a,b,c,d,e,f3(b,c,d),x,0x6D703EF3u,r)
#define R42(a,b,c,d,e,x,r) Round(a,b,c,d,e,f2(b,c,d),x,0x7A6D76E9u,r)
#define R52(a,b,c,d,e,x,r) Round(a,b,c,d,e,f1(b,c,d),x,0,r)

    R11(a1, b1, c1, d1, e1, w[0], 11);
    R12(a2, b2, c2, d2, e2, w[5], 8);
    R11(e1, a1, b1, c1, d1, w[1], 14);
    R12(e2, a2, b2, c2, d2, w[14], 9);
    R11(d1, e1, a1, b1, c1, w[2], 15);
    R12(d2, e2, a2, b2, c2, w[7], 9);
    R11(c1, d1, e1, a1, b1, w[3], 12);
    R12(c2, d2, e2, a2, b2, w[0], 11);
    R11(b1, c1, d1, e1, a1, w[4], 5);
    R12(b2, c2, d2, e2, a2, w[9], 13);
    R11(a1, b1, c1, d1, e1, w[5], 8);
    R12(a2, b2, c2, d2, e2, w[2], 15);
    R11(e1, a1, b1, c1, d1, w[6], 7);
    R12(e2, a2, b2, c2, d2, w[11], 15);
    R11(d1, e1, a1, b1, c1, w[7], 9);
    R12(d2, e2, a2, b2, c2, w[4], 5);
    R11(c1, d1, e1, a1, b1, w[8], 11);
    R12(c2, d2, e2, a2, b2, w[13], 7);
    R11(b1, c1, d1, e1, a1, w[9], 13);
    R12(b2, c2, d2, e2, a2, w[6], 7);
    R11(a1, b1, c1, d1, e1, w[10], 14);
    R12(a2, b2, c2, d2, e2, w[15], 8);
    R11(e1, a1, b1, c1, d1, w[11], 15);
    R12(e2, a2, b2, c2, d2, w[8], 11);
    R11(d1, e1, a1, b1, c1, w[12], 6);
    R12(d2, e2, a2, b2, c2, w[1], 14);
    R11(c1, d1, e1, a1, b1, w[13], 7);
    R12(c2, d2, e2, a2, b2, w[10], 14);
    R11(b1, c1, d1, e1, a1, w[14], 9);
    R12(b2, c2, d2, e2, a2, w[3], 12);
    R11(a1, b1, c1, d1, e1, w[15], 8);
    R12(a2, b2, c2, d2, e2, w[12], 6);
    R21(e1, a1, b1, c1, d1, w[7], 7);
    R22(e2, a2, b2, c2, d2, w[6], 9);
    R21(d1, e1, a1, b1, c1, w[4], 6);
    R22(d2, e2, a2, b2, c2, w[11], 13);
    R21(c1, d1, e1, a1, b1, w[13], 8);
    R22(c2, d2, e2, a2, b2, w[3], 15);
    R21(b1, c1, d1, e1, a1, w[1], 13);
    R22(b2, c2, d2, e2, a2, w[7], 7);
    R21(a1, b1, c1, d1, e1, w[10], 11);
    R22(a2, b2, c2, d2, e2, w[0], 12);
    R21(e1, a1, b1, c1, d1, w[6], 9);
    R22(e2, a2, b2, c2, d2, w[13], 8);
    R21(d1, e1, a1, b1, c1, w[15], 7);
    R22(d2, e2, a2, b2, c2, w[5], 9);
    R21(c1, d1, e1, a1, b1, w[3], 15);
    R22(c2, d2, e2, a2, b2, w[10], 11);
    R21(b1, c1, d1, e1, a1, w[12], 7);
    R22(b2, c2, d2, e2, a2, w[14], 7);
    R21(a1, b1, c1, d1, e1, w[0], 12);
    R22(a2, b2, c2, d2, e2, w[15], 7);
    R21(e1, a1, b1, c1, d1, w[9], 15);
    R22(e2, a2, b2, c2, d2, w[8], 12);
    R21(d1, e1, a1, b1, c1, w[5], 9);
    R22(d2, e2, a2, b2, c2, w[12], 7);
    R21(c1, d1, e1, a1, b1, w[2], 11);
    R22(c2, d2, e2, a2, b2, w[4], 6);
    R21(b1, c1, d1, e1, a1, w[14], 7);
    R22(b2, c2, d2, e2, a2, w[9], 15);
    R21(a1, b1, c1, d1, e1, w[11], 13);
    R22(a2, b2, c2, d2, e2, w[1], 13);
    R21(e1, a1, b1, c1, d1, w[8], 12);
    R22(e2, a2, b2, c2, d2, w[2], 11);
    R31(d1, e1, a1, b1, c1, w[3], 11);
    R32(d2, e2, a2, b2, c2, w[15], 9);
    R31(c1, d1, e1, a1, b1, w[10], 13);
    R32(c2, d2, e2, a2, b2, w[5], 7);
    R31(b1, c1, d1, e1, a1, w[14], 6);
    R32(b2, c2, d2, e2, a2, w[1], 15);
    R31(a1, b1, c1, d1, e1, w[4], 7);
    R32(a2, b2, c2, d2, e2, w[3], 11);
    R31(e1, a1, b1, c1, d1, w[9], 14);
    R32(e2, a2, b2, c2, d2, w[7], 8);
    R31(d1, e1, a1, b1, c1, w[15], 9);
    R32(d2, e2, a2, b2, c2, w[14], 6);
    R31(c1, d1, e1, a1, b1, w[8], 13);
    R32(c2, d2, e2, a2, b2, w[6], 6);
    R31(b1, c1, d1, e1, a1, w[1], 15);
    R32(b2, c2, d2, e2, a2, w[9], 14);
    R31(a1, b1, c1, d1, e1, w[2], 14);
    R32(a2, b2, c2, d2, e2, w[11], 12);
    R31(e1, a1, b1, c1, d1, w[7], 8);
    R32(e2, a2, b2, c2, d2, w[8], 13);
    R31(d1, e1, a1, b1, c1, w[0], 13);
    R32(d2, e2, a2, b2, c2, w[12], 5);
    R31(c1, d1, e1, a1, b1, w[6], 6);
    R32(c2, d2, e2, a2, b2, w[2], 14);
    R31(b1, c1, d1, e1, a1, w[13], 5);
    R32(b2, c2, d2, e2, a2, w[10], 13);
    R31(a1, b1, c1, d1, e1, w[11], 12);
    R32(a2, b2, c2, d2, e2, w[0], 13);
    R31(e1, a1, b1, c1, d1, w[5], 7);
    R32(e2, a2, b2, c2, d2, w[4], 7);
    R31(d1, e1, a1, b1, c1, w[12], 5);
    R32(d2, e2, a2, b2, c2, w[13], 5);
    R41(c1, d1, e1, a1, b1, w[1], 11);
    R42(c2, d2, e2, a2, b2, w[8], 15);
    R41(b1, c1, d1, e1, a1, w[9], 12);
    R42(b2, c2, d2, e2, a2, w[6], 5);
    R41(a1, b1, c1, d1, e1, w[11], 14);
    R42(a2, b2, c2, d2, e2, w[4], 8);
    R41(e1, a1, b1, c1, d1, w[10], 15);
    R42(e2, a2, b2, c2, d2, w[1], 11);
    R41(d1, e1, a1, b1, c1, w[0], 14);
    R42(d2, e2, a2, b2, c2, w[3], 14);
    R41(c1, d1, e1, a1, b1, w[8], 15);
    R42(c2, d2, e2, a2, b2, w[11], 14);
    R41(b1, c1, d1, e1, a1, w[12], 9);
    R42(b2, c2, d2, e2, a2, w[15], 6);
    R41(a1, b1, c1, d1, e1, w[4], 8);
    R42(a2, b2, c2, d2, e2, w[0], 14);
    R41(e1, a1, b1, c1, d1, w[13], 9);
    R42(e2, a2, b2, c2, d2, w[5], 6);
    R41(d1, e1, a1, b1, c1, w[3], 14);
    R42(d2, e2, a2, b2, c2, w[12], 9);
    R41(c1, d1, e1, a1, b1, w[7], 5);
    R42(c2, d2, e2, a2, b2, w[2], 12);
    R41(b1, c1, d1, e1, a1, w[15], 6);
    R42(b2, c2, d2, e2, a2, w[13], 9);
    R41(a1, b1, c1, d1, e1, w[14], 8);
    R42(a2, b2, c2, d2, e2, w[9], 12);
    R41(e1, a1, b1, c1, d1, w[5], 6);
    R42(e2, a2, b2, c2, d2, w[7], 5);
    R41(d1, e1, a1, b1, c1, w[6], 5);
    R42(d2, e2, a2, b2, c2, w[10], 15);
    R41(c1, d1, e1, a1, b1, w[2], 12);
    R42(c2, d2, e2, a2, b2, w[14], 8);
    R51(b1, c1, d1, e1, a1, w[4], 9);
    R52(b2, c2, d2, e2, a2, w[12], 8);
    R51(a1, b1, c1, d1, e1, w[0], 15);
    R52(a2, b2, c2, d2, e2, w[15], 5);
    R51(e1, a1, b1, c1, d1, w[5], 5);
    R52(e2, a2, b2, c2, d2, w[10], 12);
    R51(d1, e1, a1, b1, c1, w[9], 11);
    R52(d2, e2, a2, b2, c2, w[4], 9);
    R51(c1, d1, e1, a1, b1, w[7], 6);
    R52(c2, d2, e2, a2, b2, w[1], 12);
    R51(b1, c1, d1, e1, a1, w[12], 8);
    R52(b2, c2, d2, e2, a2, w[5], 5);
    R51(a1, b1, c1, d1, e1, w[2], 13);
    R52(a2, b2, c2, d2, e2, w[8], 14);
    R51(e1, a1, b1, c1, d1, w[10], 12);
    R52(e2, a2, b2, c2, d2, w[7], 6);
    R51(d1, e1, a1, b1, c1, w[14], 5);
    R52(d2, e2, a2, b2, c2, w[6], 8);
    R51(c1, d1, e1, a1, b1, w[1], 12);
    R52(c2, d2, e2, a2, b2, w[2], 13);
    R51(b1, c1, d1, e1, a1, w[3], 13);
    R52(b2, c2, d2, e2, a2, w[13], 6);
    R51(a1, b1, c1, d1, e1, w[8], 14);
    R52(a2, b2, c2, d2, e2, w[14], 5);
    R51(e1, a1, b1, c1, d1, w[11], 11);
    R52(e2, a2, b2, c2, d2, w[0], 15);
    R51(d1, e1, a1, b1, c1, w[6], 8);
    R52(d2, e2, a2, b2, c2, w[3], 13);
    R51(c1, d1, e1, a1, b1, w[15], 5);
    R52(c2, d2, e2, a2, b2, w[9], 11);
    R51(b1, c1, d1, e1, a1, w[13], 6);
    R52(b2, c2, d2, e2, a2, w[11], 11);

    uint32_t t = s[0];
    s[0] = s[1] + c1 + d2;
    s[1] = s[2] + d1 + e2;
    s[2] = s[3] + e1 + a2;
    s[3] = s[4] + a1 + b2;
    s[4] = t + b1 + c2;
#undef ROL
#undef f1
#undef f2
#undef f3
#undef f4
#undef f5
#undef Round
#undef R11
#undef R21
#undef R31
#undef R41
#undef R51
#undef R12
#undef R22
#undef R32
#undef R42
#undef R52
}

static __device__ void ripemd160_oneblock(const uint8_t block[64], uint8_t out[20]) {
    uint32_t s[5] = {0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u, 0xC3D2E1F0u};
    ripemd160_transform(s, block);
#pragma unroll
    for (int i = 0; i < 5; i++)
        store_le32(out + i * 4, s[i]);
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

static void free_pin_bufs(void) {
    if (g_h_pin_priv) { cudaFreeHost(g_h_pin_priv); g_h_pin_priv = NULL; }
    if (g_h_pin_pub) { cudaFreeHost(g_h_pin_pub); g_h_pin_pub = NULL; }
    g_pin_cap = 0;
}

static int ensure_pin_bufs(int count) {
    if (count <= g_pin_cap && g_h_pin_priv && g_h_pin_pub)
        return 1;
    free_pin_bufs();
    if (cudaHostAlloc((void **)&g_h_pin_priv, (size_t)count * 32, cudaHostAllocDefault) != cudaSuccess)
        return 0;
    if (cudaHostAlloc((void **)&g_h_pin_pub, (size_t)count * 65, cudaHostAllocDefault) != cudaSuccess) {
        free_pin_bufs();
        return 0;
    }
    g_pin_cap = count;
    return 1;
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

extern "C" int tcuda_secp_device_filter(void) {
    return g_bloom_ready && !g_host_filter && g_d_bloom != NULL;
}

extern "C" int tcuda_secp_search_init(const uint8_t *bloom_bf, uint64_t bloom_bits,
                                      uint64_t bloom_bytes, uint8_t bloom_hashes) {
    if (!bloom_bf || bloom_bits == 0 || bloom_bytes == 0 || bloom_hashes == 0)
        return 0;
    tcuda_secp_search_free();
    /* Keep host bloom always (fallback + ETH path). Upload device copy when hash160 ok. */
    free(g_h_bloom);
    g_h_bloom = (uint8_t*)malloc(bloom_bytes);
    if (!g_h_bloom) return 0;
    memcpy(g_h_bloom, bloom_bf, bloom_bytes);
    g_bloom_bits = bloom_bits;
    g_bloom_bytes = bloom_bytes;
    g_bloom_hashes = bloom_hashes;

    if (!g_host_filter) {
        if (cudaMalloc(&g_d_bloom, bloom_bytes) != cudaSuccess) {
            fprintf(stderr, "[W] CUDA device bloom alloc failed; falling back to host filter.\n");
            g_d_bloom = NULL;
            g_host_filter = 1;
        } else if (cudaMemcpy(g_d_bloom, bloom_bf, bloom_bytes, cudaMemcpyHostToDevice) != cudaSuccess) {
            fprintf(stderr, "[W] CUDA device bloom upload failed; falling back to host filter.\n");
            cudaFree(g_d_bloom);
            g_d_bloom = NULL;
            g_host_filter = 1;
        }
    }
    g_bloom_ready = 1;
    return 1;
}

extern "C" void tcuda_secp_search_free(void) {
    if (g_d_bloom) { cudaFree(g_d_bloom); g_d_bloom = NULL; }
    if (g_d_privkeys) { cudaFree(g_d_privkeys); g_d_privkeys = NULL; }
    if (g_d_pubs) { cudaFree(g_d_pubs); g_d_pubs = NULL; }
    if (g_d_hitmask) { cudaFree(g_d_hitmask); g_d_hitmask = NULL; }
    free_pin_bufs();
    free(g_h_bloom); g_h_bloom = NULL;
    g_bloom_bits = 0;
    g_bloom_bytes = 0;
    g_bloom_hashes = 0;
    g_bloom_ready = 0;
    g_cap_count = 0;
}

extern "C" int tcuda_memory_info(uint64_t *free_bytes, uint64_t *total_bytes) {
    size_t free_b = 0, total_b = 0;
    if (cudaMemGetInfo(&free_b, &total_b) != cudaSuccess)
        return 0;
    if (free_bytes) *free_bytes = (uint64_t)free_b;
    if (total_bytes) *total_bytes = (uint64_t)total_b;
    return 1;
}

extern "C" uint32_t tcuda_apply_memory_budget(uint64_t budget_bytes) {
    uint64_t free_b = 0, total_b = 0;
    tcuda_memory_info(&free_b, &total_b);

    int auto_mode = (budget_bytes == 0);
    if (auto_mode) {
        /* Leave headroom for the driver / display; use most of free VRAM. */
        budget_bytes = (free_b * 60ull) / 100ull;
        if (budget_bytes < (64ull << 20))
            budget_bytes = (free_b > (128ull << 20)) ? (64ull << 20) : (free_b / 2);
    }

    /* Never claim more than free - 256MB (or half free on small cards). */
    uint64_t headroom = (256ull << 20);
    if (free_b < headroom * 2)
        headroom = free_b / 4;
    if (free_b > headroom && budget_bytes > free_b - headroom)
        budget_bytes = free_b - headroom;

    g_mem_budget_bytes = budget_bytes;

    /* Device: priv(32)+pub(65)+hit(1) ≈ 98B; pin staging ≈ 97B; round to 160B/key. */
    const uint64_t bytes_per_key = 160ull;
    uint64_t keys = budget_bytes / bytes_per_key;
    if (keys < 1024ull) keys = 1024ull;
    if (keys > 1048576ull) keys = 1048576ull; /* 1M host batch ceiling */

    /* Launch chunk: large enough for occupancy, small enough for Windows TDR. */
    int chunk = (int)keys;
    if (chunk > 65536) chunk = 65536;
    if (chunk < 1024) chunk = 1024;
    /* Align to thread block size */
    chunk = (chunk / kCudaThreads) * kCudaThreads;
    if (chunk < kCudaThreads) chunk = kCudaThreads;
    g_launch_chunk = chunk;

    if (!ensure_batch_bufs(g_launch_chunk)) {
        fprintf(stderr, "[W] CUDA: failed to allocate device buffers for %d-key chunk.\n",
                g_launch_chunk);
        g_launch_chunk = 1024;
        ensure_batch_bufs(g_launch_chunk);
    }
    ensure_pin_bufs(g_launch_chunk);

    g_recommended_batch = (uint32_t)keys;

    fprintf(stderr,
            "[+] GPU memory plan: budget %.1f MB (%s) | VRAM free %.1f / total %.1f MB | "
            "batch up to %u keys | launch chunk %d | ~%.1f MB device buffers\n",
            (double)budget_bytes / (1024.0 * 1024.0),
            auto_mode ? "auto" : "user",
            (double)free_b / (1024.0 * 1024.0),
            (double)total_b / (1024.0 * 1024.0),
            g_recommended_batch,
            g_launch_chunk,
            (double)g_launch_chunk * 98.0 / (1024.0 * 1024.0));
    fflush(stderr);
    return g_recommended_batch;
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

    int chunk = g_launch_chunk > 0 ? g_launch_chunk : 16384;
    if (chunk > count) chunk = count;
    if (chunk < 1) chunk = 1;
    /* Align chunk down to block size when possible */
    if (chunk >= kCudaThreads)
        chunk = (chunk / kCudaThreads) * kCudaThreads;
    if (chunk < 1) chunk = count < kCudaThreads ? count : kCudaThreads;

    if (!ensure_batch_bufs(chunk))
        return -1;
    int use_pin = ensure_pin_bufs(chunk);

    for (int off = 0; off < count; off += chunk) {
        int n = count - off;
        if (n > chunk) n = chunk;
        size_t key_bytes = (size_t)n * 32;
        size_t pub_bytes = (size_t)n * 65;

        const uint8_t *src = privkeys + (size_t)off * 32;
        if (use_pin) {
            memcpy(g_h_pin_priv, src, key_bytes);
            src = g_h_pin_priv;
        }
        if (cudaMemcpy(g_d_privkeys, src, key_bytes, cudaMemcpyHostToDevice) != cudaSuccess)
            return -1;

        int threads = (n < kCudaThreads) ? n : kCudaThreads;
        if (threads < 1) threads = 1;
        int blocks = (n + threads - 1) / threads;
        secp_pubkey_kernel<<<blocks, threads>>>(g_d_privkeys, n, compressed ? 1 : 0, g_d_pubs);
        if (cudaGetLastError() != cudaSuccess) return -1;
        if (cudaDeviceSynchronize() != cudaSuccess) return -1;

        uint8_t *dst = out_pubs65 + (size_t)off * 65;
        if (use_pin) {
            if (cudaMemcpy(g_h_pin_pub, g_d_pubs, pub_bytes, cudaMemcpyDeviceToHost) != cudaSuccess)
                return -1;
            memcpy(dst, g_h_pin_pub, pub_bytes);
        } else {
            if (cudaMemcpy(dst, g_d_pubs, pub_bytes, cudaMemcpyDeviceToHost) != cudaSuccess)
                return -1;
        }
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

    static const uint8_t expect_h160[20] = {
        0x75, 0x1e, 0x76, 0xe8, 0x19, 0x91, 0x96, 0xd4, 0x54, 0x94,
        0x1c, 0x45, 0xd1, 0xb3, 0xa3, 0x23, 0xf1, 0x43, 0x3b, 0xd6
    };

    if (memcmp(pub, expect_pub, 33) != 0) {
        fprintf(stderr, "[E] tcuda_secp_selftest: pubkey mismatch.\n  got: ");
        for (int i = 0; i < 33; i++) fprintf(stderr, "%02x", pub[i]);
        fprintf(stderr, "\n  exp: ");
        for (int i = 0; i < 33; i++) fprintf(stderr, "%02x", expect_pub[i]);
        fprintf(stderr, "\n");
        fflush(stderr);
        return 0;
    }

    if (memcmp(h160_dev, expect_h160, 20) != 0) {
        fprintf(stderr, "[E] tcuda_secp_selftest: device hash160 mismatch.\n  got: ");
        for (int i = 0; i < 20; i++) fprintf(stderr, "%02x", h160_dev[i]);
        fprintf(stderr, "\n  exp: ");
        for (int i = 0; i < 20; i++) fprintf(stderr, "%02x", expect_h160[i]);
        fprintf(stderr, "\n");
        fflush(stderr);
        g_host_filter = 1;
        fprintf(stderr, "[+] CUDA secp256k1 self-test: GPU EC ok; host filter mode (hash160 fail).\n");
        fflush(stderr);
        return 1; /* EC still usable */
    }

    g_host_filter = 0;
    fprintf(stderr, "[+] CUDA secp256k1 self-test passed (GPU EC + device hash160).\n");
    fflush(stderr);
    return 1;
}

/* =========================================================================
 * BSGS giant-step GRP (device affine EC; host does bloom)
 * ========================================================================= */

static uint8_t *g_d_gsn = NULL;
static uint8_t *g_d_twogsn = NULL;
static Fe      *g_d_bsgs_ws = NULL; /* scratch: half Fe dx + half Fe gsn_x + half Fe gsn_y */
static int      g_bsgs_half = 0;
static int      g_bsgs_ready = 0;

static __device__ void affine_add_xy(Fe *rx, Fe *ry,
                                     const Fe *ax, const Fe *ay,
                                     const Fe *bx, const Fe *by,
                                     const Fe *inv_dx) {
    Fe dy, s, p, t;
    fe_sub(&dy, by, ay);
    fe_mul(&s, &dy, inv_dx);
    fe_sqr(&p, &s);
    fe_sub(rx, &p, ax);
    fe_sub(rx, rx, bx);
    fe_sub(&t, bx, rx);
    fe_mul(ry, &s, &t);
    fe_sub(ry, ry, by);
}

/*
 * One GRP cycle per launch (serial on thread 0). Matches keyhunt thread_process_bsgs
 * point layout: out_x[i] = pts[i].x for i in [0, 2*half).
 */
__global__ void bsgs_grp_one_cycle_kernel(const uint8_t *gsn64, const uint8_t *twogsn64,
                                          int half, Fe *ws, uint8_t *start_xy,
                                          uint8_t *out_x) {
    if (threadIdx.x != 0 || blockIdx.x != 0) return;

    Fe *dx = ws;
    Fe *gsn_x = ws + (half + 2);
    Fe *gsn_y = gsn_x + half;

    Fe spx, spy;
    fe_from_bytes_be(&spx, start_xy);
    fe_from_bytes_be(&spy, start_xy + 32);

    for (int i = 0; i < half; i++) {
        fe_from_bytes_be(&gsn_x[i], gsn64 + (size_t)i * 64);
        fe_from_bytes_be(&gsn_y[i], gsn64 + (size_t)i * 64 + 32);
    }
    Fe t2x, t2y;
    fe_from_bytes_be(&t2x, twogsn64);
    fe_from_bytes_be(&t2y, twogsn64 + 32);

    int hLength = half - 1;
    for (int i = 0; i < hLength; i++)
        fe_sub(&dx[i], &gsn_x[i], &spx);
    fe_sub(&dx[hLength], &gsn_x[hLength], &spx);
    fe_sub(&dx[hLength + 1], &t2x, &spx);

    for (int i = 0; i < hLength + 2; i++) {
        Fe t; fe_set(&t, &dx[i]);
        fe_inv(&dx[i], &t);
    }

    /* center at index half */
    fe_to_bytes_be(&spx, out_x + (size_t)half * 32);

    for (int i = 0; i < hLength; i++) {
        Fe ppx, ppy, pnx, pny, ngsn_y;
        affine_add_xy(&ppx, &ppy, &spx, &spy, &gsn_x[i], &gsn_y[i], &dx[i]);
        fe_set(&ngsn_y, &gsn_y[i]);
        Fe z; fe_clear(&z);
        fe_sub(&ngsn_y, &z, &ngsn_y);
        affine_add_xy(&pnx, &pny, &spx, &spy, &gsn_x[i], &ngsn_y, &dx[i]);
        fe_to_bytes_be(&ppx, out_x + (size_t)(half + (i + 1)) * 32);
        fe_to_bytes_be(&pnx, out_x + (size_t)(half - (i + 1)) * 32);
    }
    /* pts[0] = startP - GSn[hLength] */
    {
        Fe pnx, pny, ngsn_y, z;
        fe_clear(&z);
        fe_set(&ngsn_y, &gsn_y[hLength]);
        fe_sub(&ngsn_y, &z, &ngsn_y);
        affine_add_xy(&pnx, &pny, &spx, &spy, &gsn_x[hLength], &ngsn_y, &dx[hLength]);
        fe_to_bytes_be(&pnx, out_x);
    }
    /* startP += _2GSn */
    {
        Fe nx, ny;
        affine_add_xy(&nx, &ny, &spx, &spy, &t2x, &t2y, &dx[hLength + 1]);
        fe_to_bytes_be(&nx, start_xy);
        fe_to_bytes_be(&ny, start_xy + 32);
    }
}

extern "C" void tcuda_bsgs_grp_free(void) {
    if (g_d_gsn) { cudaFree(g_d_gsn); g_d_gsn = NULL; }
    if (g_d_twogsn) { cudaFree(g_d_twogsn); g_d_twogsn = NULL; }
    if (g_d_bsgs_ws) { cudaFree(g_d_bsgs_ws); g_d_bsgs_ws = NULL; }
    g_bsgs_half = 0;
    g_bsgs_ready = 0;
}

extern "C" int tcuda_bsgs_grp_ready(void) {
    return g_bsgs_ready;
}

extern "C" int tcuda_bsgs_grp_init(const uint8_t *gsn_xy64, int half,
                                   const uint8_t *twogsn_xy64) {
    if (!gsn_xy64 || !twogsn_xy64 || half < 2 || half > 4096)
        return 0;
    tcuda_bsgs_grp_free();
    size_t gsn_bytes = (size_t)half * 64;
    size_t ws_count = (size_t)(half + 2) + (size_t)half * 2;
    if (cudaMalloc(&g_d_gsn, gsn_bytes) != cudaSuccess ||
        cudaMalloc(&g_d_twogsn, 64) != cudaSuccess ||
        cudaMalloc(&g_d_bsgs_ws, ws_count * sizeof(Fe)) != cudaSuccess) {
        tcuda_bsgs_grp_free();
        return 0;
    }
    if (cudaMemcpy(g_d_gsn, gsn_xy64, gsn_bytes, cudaMemcpyHostToDevice) != cudaSuccess ||
        cudaMemcpy(g_d_twogsn, twogsn_xy64, 64, cudaMemcpyHostToDevice) != cudaSuccess) {
        tcuda_bsgs_grp_free();
        return 0;
    }
    g_bsgs_half = half;
    g_bsgs_ready = 1;
    fprintf(stderr, "[+] CUDA BSGS GRP ready (half=%d, GSn on device).\n", half);
    fflush(stderr);
    return 1;
}

extern "C" int tcuda_bsgs_grp_run(uint8_t *start_xy64, int n_cycles, uint8_t *out_x32) {
    if (!g_bsgs_ready || !start_xy64 || !out_x32 || n_cycles <= 0)
        return 0;
    int half = g_bsgs_half;
    int grp = half * 2;
    uint8_t *d_start = NULL, *d_out = NULL;
    size_t out_bytes = (size_t)n_cycles * (size_t)grp * 32;
    if (cudaMalloc(&d_start, 64) != cudaSuccess ||
        cudaMalloc(&d_out, out_bytes) != cudaSuccess) {
        if (d_start) cudaFree(d_start);
        if (d_out) cudaFree(d_out);
        return 0;
    }
    if (cudaMemcpy(d_start, start_xy64, 64, cudaMemcpyHostToDevice) != cudaSuccess) {
        cudaFree(d_start); cudaFree(d_out); return 0;
    }
    for (int c = 0; c < n_cycles; c++) {
        uint8_t *ox = d_out + (size_t)c * (size_t)grp * 32;
        bsgs_grp_one_cycle_kernel<<<1, 1>>>(g_d_gsn, g_d_twogsn, half, g_d_bsgs_ws,
                                            d_start, ox);
        if (cudaGetLastError() != cudaSuccess || cudaDeviceSynchronize() != cudaSuccess) {
            cudaFree(d_start); cudaFree(d_out); return 0;
        }
    }
    if (cudaMemcpy(out_x32, d_out, out_bytes, cudaMemcpyDeviceToHost) != cudaSuccess ||
        cudaMemcpy(start_xy64, d_start, 64, cudaMemcpyDeviceToHost) != cudaSuccess) {
        cudaFree(d_start); cudaFree(d_out); return 0;
    }
    cudaFree(d_start);
    cudaFree(d_out);
    return 1;
}
