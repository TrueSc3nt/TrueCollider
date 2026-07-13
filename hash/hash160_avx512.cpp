/*
 * AVX-512 batch hash160 (SHA-256 + RIPEMD-160) for Bitcoin public keys.
 *
 * This file implements 16-way SIMD versions of SHA-256 and RIPEMD-160
 * intended for address/rmd160/vanity search. Only the compressed public-key
 * path (33 bytes -> 20-byte hash160) is implemented in this first version.
 *
 * Compile with -mavx512f -mavx512bw. Runtime dispatch must verify AVX-512BW
 * before calling these functions.
 */

#include "hash160_avx512.h"

#if HASH160_AVX512_AVAILABLE

#include <immintrin.h>
#include <string.h>
#include <stdio.h>

#include "sha256.h"
#include "ripemd160.h"

/* ------------------------------------------------------------------------- */
/* SHA-256 constants and helpers                                               */
/* ------------------------------------------------------------------------- */

static const uint32_t K256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
    0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
    0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
    0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
    0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
    0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static const uint32_t IV256[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

static inline __m512i set1_u32(uint32_t x) { return _mm512_set1_epi32((int)x); }

static inline __m512i rotr32(__m512i x, unsigned int n) {
    return _mm512_or_si512(_mm512_srli_epi32(x, n), _mm512_slli_epi32(x, 32 - n));
}

static inline __m512i bswap32(__m512i v) {
    const __m512i mask = _mm512_set_epi8(
        12, 13, 14, 15,  8,  9, 10, 11,  4,  5,  6,  7,  0,  1,  2,  3,
        12, 13, 14, 15,  8,  9, 10, 11,  4,  5,  6,  7,  0,  1,  2,  3,
        12, 13, 14, 15,  8,  9, 10, 11,  4,  5,  6,  7,  0,  1,  2,  3,
        12, 13, 14, 15,  8,  9, 10, 11,  4,  5,  6,  7,  0,  1,  2,  3
    );
    return _mm512_shuffle_epi8(v, mask);
}

/* Gather 16 big-endian 32-bit words from 16 input buffers at offset off. */
static inline __m512i gather_word(const uint8_t *inputs[16], int off) {
    return _mm512_setr_epi32(
        (int)*(uint32_t*)(inputs[ 0] + off),
        (int)*(uint32_t*)(inputs[ 1] + off),
        (int)*(uint32_t*)(inputs[ 2] + off),
        (int)*(uint32_t*)(inputs[ 3] + off),
        (int)*(uint32_t*)(inputs[ 4] + off),
        (int)*(uint32_t*)(inputs[ 5] + off),
        (int)*(uint32_t*)(inputs[ 6] + off),
        (int)*(uint32_t*)(inputs[ 7] + off),
        (int)*(uint32_t*)(inputs[ 8] + off),
        (int)*(uint32_t*)(inputs[ 9] + off),
        (int)*(uint32_t*)(inputs[10] + off),
        (int)*(uint32_t*)(inputs[11] + off),
        (int)*(uint32_t*)(inputs[12] + off),
        (int)*(uint32_t*)(inputs[13] + off),
        (int)*(uint32_t*)(inputs[14] + off),
        (int)*(uint32_t*)(inputs[15] + off)
    );
}

/* Scatter 16 big-endian 32-bit words from vector v to 16 output buffers. */
static inline void scatter_word(__m512i v, uint8_t *outputs[16], int off) {
    uint32_t tmp[16];
    _mm512_storeu_si512((__m512i*)tmp, bswap32(v));
    for (int i = 0; i < 16; i++) {
        *(uint32_t*)(outputs[i] + off) = tmp[i];
    }
}

static inline __m512i sha_Ch(__m512i e, __m512i f, __m512i g) {
    return _mm512_xor_si512(_mm512_and_si512(e, f), _mm512_andnot_si512(e, g));
}

static inline __m512i sha_Maj(__m512i a, __m512i b, __m512i c) {
    return _mm512_xor_si512(_mm512_xor_si512(_mm512_and_si512(a, b), _mm512_and_si512(a, c)), _mm512_and_si512(b, c));
}

static inline __m512i sha_Sig0(__m512i x) {
    return _mm512_xor_si512(_mm512_xor_si512(rotr32(x, 2), rotr32(x, 13)), rotr32(x, 22));
}

static inline __m512i sha_Sig1(__m512i x) {
    return _mm512_xor_si512(_mm512_xor_si512(rotr32(x, 6), rotr32(x, 11)), rotr32(x, 25));
}

static inline __m512i sha_sig0(__m512i x) {
    return _mm512_xor_si512(_mm512_xor_si512(rotr32(x, 7), rotr32(x, 18)), _mm512_srli_epi32(x, 3));
}

static inline __m512i sha_sig1(__m512i x) {
    return _mm512_xor_si512(_mm512_xor_si512(rotr32(x, 17), rotr32(x, 19)), _mm512_srli_epi32(x, 10));
}

static inline __m512i sha_add(__m512i a, __m512i b) { return _mm512_add_epi32(a, b); }
static inline __m512i sha_add3(__m512i a, __m512i b, __m512i c) {
    return _mm512_add_epi32(_mm512_add_epi32(a, b), c);
}
static inline __m512i sha_add4(__m512i a, __m512i b, __m512i c, __m512i d) {
    return _mm512_add_epi32(_mm512_add_epi32(a, b), _mm512_add_epi32(c, d));
}

/*
 * SHA-256 compression on 16 independent 64-byte blocks.
 * Initial state in *a..*h; updated in place (with IV added at end).
 */
static void sha256_avx512_compress_16(const uint8_t *input[16],
    __m512i *a, __m512i *b, __m512i *c, __m512i *d,
    __m512i *e, __m512i *f, __m512i *g, __m512i *h) {
    __m512i w[16];
    for (int i = 0; i < 16; i++) {
        w[i] = bswap32(gather_word(input, i * 4));
    }

    __m512i aa = *a, bb = *b, cc = *c, dd = *d;
    __m512i ee = *e, ff = *f, gg = *g, hh = *h;

    for (int round = 0; round < 64; round++) {
        __m512i wt;
        if (round < 16) {
            wt = w[round];
        } else {
            wt = sha_add4(sha_sig0(w[(round - 15) & 15]), w[(round - 16) & 15],
                          sha_sig1(w[(round - 2) & 15]), w[(round - 7) & 15]);
            w[round & 15] = wt;
        }

        __m512i T1 = sha_add4(hh, sha_Sig1(ee), sha_Ch(ee, ff, gg), sha_add(set1_u32(K256[round]), wt));
        __m512i T2 = sha_add(sha_Sig0(aa), sha_Maj(aa, bb, cc));
        hh = gg; gg = ff; ff = ee; ee = sha_add(dd, T1);
        dd = cc; cc = bb; bb = aa; aa = sha_add(T1, T2);
    }

    *a = sha_add(aa, *a);
    *b = sha_add(bb, *b);
    *c = sha_add(cc, *c);
    *d = sha_add(dd, *d);
    *e = sha_add(ee, *e);
    *f = sha_add(ff, *f);
    *g = sha_add(gg, *g);
    *h = sha_add(hh, *h);
}

/*
 * SHA-256 transform on 16 independent 64-byte blocks (single block message).
 */
static void sha256_avx512_transform_16(const uint8_t *input[16], uint8_t *digest[16]) {
    __m512i a = set1_u32(IV256[0]);
    __m512i b = set1_u32(IV256[1]);
    __m512i c = set1_u32(IV256[2]);
    __m512i d = set1_u32(IV256[3]);
    __m512i e = set1_u32(IV256[4]);
    __m512i f = set1_u32(IV256[5]);
    __m512i g = set1_u32(IV256[6]);
    __m512i h = set1_u32(IV256[7]);

    sha256_avx512_compress_16(input, &a, &b, &c, &d, &e, &f, &g, &h);

    scatter_word(a, digest, 0);
    scatter_word(b, digest, 4);
    scatter_word(c, digest, 8);
    scatter_word(d, digest, 12);
    scatter_word(e, digest, 16);
    scatter_word(f, digest, 20);
    scatter_word(g, digest, 24);
    scatter_word(h, digest, 28);
}

/*
 * SHA-256 on 16 independent 65-byte messages (two 64-byte blocks each).
 */
static void sha256_avx512_2block_16(const uint8_t *block0[16], const uint8_t *block1[16],
                                    uint8_t *digest[16]) {
    __m512i a = set1_u32(IV256[0]);
    __m512i b = set1_u32(IV256[1]);
    __m512i c = set1_u32(IV256[2]);
    __m512i d = set1_u32(IV256[3]);
    __m512i e = set1_u32(IV256[4]);
    __m512i f = set1_u32(IV256[5]);
    __m512i g = set1_u32(IV256[6]);
    __m512i h = set1_u32(IV256[7]);

    sha256_avx512_compress_16(block0, &a, &b, &c, &d, &e, &f, &g, &h);
    sha256_avx512_compress_16(block1, &a, &b, &c, &d, &e, &f, &g, &h);

    scatter_word(a, digest, 0);
    scatter_word(b, digest, 4);
    scatter_word(c, digest, 8);
    scatter_word(d, digest, 12);
    scatter_word(e, digest, 16);
    scatter_word(f, digest, 20);
    scatter_word(g, digest, 24);
    scatter_word(h, digest, 28);
}

/* ------------------------------------------------------------------------- */
/* RIPEMD-160 constants and helpers                                            */
/* ------------------------------------------------------------------------- */

static const uint32_t RMD_K[5] = { 0x00000000, 0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xa953fd4e };
static const uint32_t RMD_Kp[5] = { 0x50a28be6, 0x5c4dd124, 0x6d703ef3, 0x7a6d76e9, 0x00000000 };

static const int RMD_r[5][16] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
    { 7, 4, 13, 1, 10, 6, 15, 3, 12, 0, 9, 5, 2, 14, 11, 8 },
    { 3, 10, 14, 4, 9, 15, 8, 1, 2, 7, 0, 6, 13, 11, 5, 12 },
    { 1, 9, 11, 10, 0, 8, 12, 4, 13, 3, 7, 15, 14, 5, 6, 2 },
    { 4, 0, 5, 9, 7, 12, 2, 10, 14, 1, 3, 8, 11, 6, 15, 13 }
};

static const int RMD_rp[5][16] = {
    { 5, 14, 7, 0, 9, 2, 11, 4, 13, 6, 15, 8, 1, 10, 3, 12 },
    { 6, 11, 3, 7, 0, 13, 5, 10, 14, 15, 8, 12, 4, 9, 1, 2 },
    { 15, 5, 1, 3, 7, 14, 6, 9, 11, 8, 12, 2, 10, 0, 4, 13 },
    { 8, 6, 4, 1, 3, 11, 15, 0, 5, 12, 2, 13, 9, 7, 10, 14 },
    { 12, 15, 10, 4, 1, 5, 8, 7, 9, 14, 2, 13, 0, 3, 11, 6 }
};

static const int RMD_s[5][16] = {
    { 11, 14, 15, 12, 5, 8, 7, 9, 11, 13, 14, 15, 6, 7, 9, 8 },
    { 7, 6, 8, 13, 11, 9, 7, 15, 7, 12, 15, 9, 11, 7, 13, 12 },
    { 11, 13, 6, 7, 14, 9, 13, 15, 14, 8, 13, 6, 5, 12, 7, 5 },
    { 11, 12, 14, 15, 14, 15, 9, 8, 9, 14, 5, 6, 8, 6, 5, 12 },
    { 9, 15, 5, 11, 6, 8, 13, 12, 5, 12, 13, 14, 11, 8, 5, 6 }
};

static const int RMD_sp[5][16] = {
    { 8, 9, 9, 11, 13, 15, 15, 5, 7, 7, 8, 11, 14, 14, 12, 6 },
    { 9, 13, 15, 7, 12, 8, 9, 11, 7, 7, 12, 7, 6, 15, 13, 11 },
    { 9, 7, 15, 11, 8, 6, 6, 14, 12, 13, 5, 14, 13, 13, 7, 5 },
    { 15, 5, 8, 11, 14, 14, 6, 14, 6, 9, 12, 9, 12, 5, 15, 8 },
    { 8, 5, 12, 9, 12, 5, 14, 6, 8, 13, 6, 5, 15, 13, 11, 11 }
};

static const uint32_t RMD_IV[5] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0 };

static inline __m512i rmd_f(int j, __m512i x, __m512i y, __m512i z) {
    if (j < 16)      return _mm512_xor_si512(_mm512_xor_si512(x, y), z);
    else if (j < 32) return _mm512_or_si512(_mm512_and_si512(x, y), _mm512_andnot_si512(x, z));
    else if (j < 48) return _mm512_xor_si512(_mm512_or_si512(x, _mm512_xor_si512(y, set1_u32(0xffffffff))), z);
    else if (j < 64) return _mm512_or_si512(_mm512_and_si512(x, z), _mm512_and_si512(y, _mm512_xor_si512(z, set1_u32(0xffffffff))));
    else             return _mm512_xor_si512(x, _mm512_or_si512(y, _mm512_xor_si512(z, set1_u32(0xffffffff))));
}

static inline __m512i rmd_rol(__m512i x, int n) {
    return _mm512_or_si512(_mm512_slli_epi32(x, n), _mm512_srli_epi32(x, 32 - n));
}

/*
 * RIPEMD-160 transform on 16 independent 64-byte blocks.
 */
static void ripemd160_avx512_transform_16(const uint8_t *input[16], uint8_t *digest[16]) {
    __m512i w[16];
    for (int i = 0; i < 16; i++) {
        w[i] = bswap32(gather_word(input, i * 4));
    }

    __m512i a = set1_u32(RMD_IV[0]);
    __m512i b = set1_u32(RMD_IV[1]);
    __m512i c = set1_u32(RMD_IV[2]);
    __m512i d = set1_u32(RMD_IV[3]);
    __m512i e = set1_u32(RMD_IV[4]);
    __m512i ap = a, bp = b, cp = c, dp = d, ep = e;

    for (int j = 0; j < 80; j++) {
        int round = j / 16;
        int idx = j % 16;

        __m512i f1 = rmd_f(j, b, c, d);
        __m512i t1 = sha_add4(a, f1, w[RMD_r[round][idx]], set1_u32(RMD_K[round]));
        a = e; e = d; d = rmd_rol(c, 10); c = b;
        b = sha_add(rmd_rol(t1, RMD_s[round][idx]), b);

        __m512i f2 = rmd_f(79 - j, bp, cp, dp);
        __m512i t2 = sha_add4(ap, f2, w[RMD_rp[round][idx]], set1_u32(RMD_Kp[round]));
        ap = ep; ep = dp; dp = rmd_rol(cp, 10); cp = bp;
        bp = sha_add(rmd_rol(t2, RMD_sp[round][idx]), bp);
    }

    /* Finalization: h0 = h1 + c1 + d2, h1 = h2 + d1 + e2, ... */
    __m512i na = sha_add3(set1_u32(RMD_IV[1]), c, dp);
    __m512i nb = sha_add3(set1_u32(RMD_IV[2]), d, ep);
    __m512i nc = sha_add3(set1_u32(RMD_IV[3]), e, ap);
    __m512i nd = sha_add3(set1_u32(RMD_IV[4]), a, bp);
    __m512i ne = sha_add3(set1_u32(RMD_IV[0]), b, cp);

    scatter_word(na, digest, 0);
    scatter_word(nb, digest, 4);
    scatter_word(nc, digest, 8);
    scatter_word(nd, digest, 12);
    scatter_word(ne, digest, 16);
}

/* ------------------------------------------------------------------------- */
/* Public API                                                                  */
/* ------------------------------------------------------------------------- */

static const uint8_t sha256_pad_33[64] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 8
};

static const uint8_t sha256_pad_65_block1[63] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 8
};

static const uint8_t sha256_pad_22[42] = {
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0xB0
};

static void ripemd160_avx512_from_sha32(uint8_t sha_out[16][32], uint8_t *outputs[16]) {
    uint8_t rmd_blocks[16][64];
    const uint8_t *in_ptrs[16];
    uint8_t *sha_ptrs[16];

    for (int lane = 0; lane < 16; lane++) {
        memcpy(rmd_blocks[lane], sha_out[lane], 32);
        rmd_blocks[lane][32] = 0x80;
        memset(rmd_blocks[lane] + 33, 0, 23);
        rmd_blocks[lane][62] = 1;
        rmd_blocks[lane][63] = 0;
        in_ptrs[lane] = rmd_blocks[lane];
        sha_ptrs[lane] = sha_out[lane];
    }

    ripemd160_avx512_transform_16(in_ptrs, sha_ptrs);

    for (int lane = 0; lane < 16; lane++) {
        memcpy(outputs[lane], sha_out[lane], 20);
    }
}

void hash160_avx512_16x33(const uint8_t *inputs[16], uint8_t *outputs[16]) {
    uint8_t sha_out[16][32];
    uint8_t *sha_ptrs[16];
    const uint8_t *in_ptrs[16];

    uint8_t blocks[16][64];
    for (int lane = 0; lane < 16; lane++) {
        memcpy(blocks[lane], inputs[lane], 33);
        memcpy(blocks[lane] + 33, sha256_pad_33 + 1, 31);
        sha_ptrs[lane] = sha_out[lane];
        in_ptrs[lane] = blocks[lane];
    }

    sha256_avx512_transform_16(in_ptrs, sha_ptrs);
    ripemd160_avx512_from_sha32(sha_out, outputs);
}

void hash160_avx512_16x65(const uint8_t *inputs[16], uint8_t *outputs[16]) {
    uint8_t sha_out[16][32];
    uint8_t *sha_ptrs[16];
    const uint8_t *block0[16];
    const uint8_t *block1[16];
    uint8_t buf[16][128];

    for (int lane = 0; lane < 16; lane++) {
        memcpy(buf[lane], inputs[lane], 65);
        memcpy(buf[lane] + 65, sha256_pad_65_block1, 63);
        block0[lane] = buf[lane];
        block1[lane] = buf[lane] + 64;
        sha_ptrs[lane] = sha_out[lane];
    }

    sha256_avx512_2block_16(block0, block1, sha_ptrs);
    ripemd160_avx512_from_sha32(sha_out, outputs);
}

void hash160_avx512_16x22(const uint8_t *inputs[16], uint8_t *outputs[16]) {
    uint8_t sha_out[16][32];
    uint8_t *sha_ptrs[16];
    const uint8_t *in_ptrs[16];
    uint8_t blocks[16][64];

    for (int lane = 0; lane < 16; lane++) {
        memcpy(blocks[lane], inputs[lane], 22);
        memcpy(blocks[lane] + 22, sha256_pad_22, 42);
        sha_ptrs[lane] = sha_out[lane];
        in_ptrs[lane] = blocks[lane];
    }

    sha256_avx512_transform_16(in_ptrs, sha_ptrs);
    ripemd160_avx512_from_sha32(sha_out, outputs);
}

/* ------------------------------------------------------------------------- */
/* Self-test                                                                   */
/* ------------------------------------------------------------------------- */

static void scalar_hash160_msg(const uint8_t *msg, int len, uint8_t *out) {
    uint8_t sha[32];
    uint8_t padded[128];
    if (len == 33) {
        memcpy(padded, msg, 33);
        memcpy(padded + 33, sha256_pad_33 + 1, 31);
        sha256_33(padded, sha);
    } else if (len == 65) {
        memcpy(padded, msg, 65);
        memcpy(padded + 65, sha256_pad_65_block1, 63);
        sha256_65(padded, sha);
    } else if (len == 22) {
        sha256((uint8_t*)msg, 22, sha);
    }
    ripemd160_32(sha, out);
}

int hash160_avx512_selftest(void) {
    /* Compressed public key: 0279be667...1798 */
    const uint8_t pubkey_comp[33] = {
        0x02, 0x79, 0xbe, 0x67, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0, 0x62,
        0x95, 0xce, 0x87, 0x0b, 0x07, 0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28,
        0xd9, 0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98
    };
    /* Uncompressed: 04 + x + y (generator point 1*G) */
    const uint8_t pubkey_uncomp[65] = {
        0x04, 0x79, 0xbe, 0x67, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0, 0x62,
        0x95, 0xce, 0x87, 0x0b, 0x07, 0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28,
        0xd9, 0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98,
        0x48, 0x3a, 0xda, 0x77, 0x26, 0xa3, 0xc4, 0x65, 0x5d, 0xa4, 0xfb, 0xfc,
        0x0e, 0x11, 0x08, 0xa8, 0xfd, 0x17, 0xb4, 0x48, 0xa6, 0x85, 0x54, 0x19,
        0x9c, 0x47, 0xd0, 0x8f, 0xfb, 0x10, 0xd4, 0xb8
    };

    uint8_t scalar_comp[20], scalar_uncomp[20], scalar_script[20];
    scalar_hash160_msg(pubkey_comp, 33, scalar_comp);
    scalar_hash160_msg(pubkey_uncomp, 65, scalar_uncomp);

    uint8_t script[22];
    script[0] = 0x00;
    script[1] = 0x14;
    memcpy(script + 2, scalar_comp, 20);
    scalar_hash160_msg(script, 22, scalar_script);

    uint8_t avx_out[16][20];
    const uint8_t *in_ptrs[16];
    uint8_t *out_ptrs[16];
    uint8_t avx_in[16][128];

    for (int i = 0; i < 16; i++) {
        out_ptrs[i] = avx_out[i];
    }

    for (int i = 0; i < 16; i++) {
        memcpy(avx_in[i], pubkey_comp, 33);
        in_ptrs[i] = avx_in[i];
    }
    hash160_avx512_16x33(in_ptrs, out_ptrs);
    if (memcmp(avx_out[0], scalar_comp, 20) != 0) {
        fprintf(stderr, "[E] hash160_avx512_selftest failed: compressed key mismatch.\n");
        return 0;
    }

    for (int i = 0; i < 16; i++) {
        memcpy(avx_in[i], pubkey_uncomp, 65);
        in_ptrs[i] = avx_in[i];
    }
    hash160_avx512_16x65(in_ptrs, out_ptrs);
    if (memcmp(avx_out[0], scalar_uncomp, 20) != 0) {
        fprintf(stderr, "[E] hash160_avx512_selftest failed: uncompressed key mismatch.\n");
        return 0;
    }

    for (int i = 0; i < 16; i++) {
        memcpy(avx_in[i], script, 22);
        in_ptrs[i] = avx_in[i];
    }
    hash160_avx512_16x22(in_ptrs, out_ptrs);
    if (memcmp(avx_out[0], scalar_script, 20) != 0) {
        fprintf(stderr, "[E] hash160_avx512_selftest failed: P2SH script mismatch.\n");
        return 0;
    }

    printf("[+] hash160_avx512 self-test passed (compressed, uncompressed, P2SH).\n");
    return 1;
}

#endif /* HASH160_AVX512_AVAILABLE */
