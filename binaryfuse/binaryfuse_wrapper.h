#ifndef BINARYFUSE_WRAPPER_H
#define BINARYFUSE_WRAPPER_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "binaryfusefilter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BF_MAX_COLLECT  1
#define BF_STATE_COLLECT 0
#define BF_STATE_BUILT  1

struct binaryfuse_wrapper {
    binary_fuse8_t filter;
    binary_fuse16_t filter16;
    uint64_t *keys;
    uint32_t capacity;
    uint32_t count;
    int state;
    long double error;
    int use_bloom_fallback;
    int use_fuse16; /* 1 = binary_fuse16 backend (huge lists) */
};

static inline void bf_init(struct binaryfuse_wrapper *bf, uint32_t expected, long double error) {
    bf->capacity = expected > 1000 ? expected : 1000;
    bf->count = 0;
    bf->state = BF_STATE_COLLECT;
    bf->error = error > 0 ? error : 0.000001;
    bf->use_bloom_fallback = 0;
    bf->use_fuse16 = 0;
    bf->keys = (uint64_t *)malloc(bf->capacity * sizeof(uint64_t));
    if (!bf->keys) {
        bf->capacity = 0;
    }
    memset(&bf->filter, 0, sizeof(binary_fuse8_t));
    memset(&bf->filter16, 0, sizeof(binary_fuse16_t));
}

static inline void bf_init_fuse16(struct binaryfuse_wrapper *bf, uint32_t expected, long double error) {
    bf_init(bf, expected, error);
    bf->use_fuse16 = 1;
}

static inline int bf_add(struct binaryfuse_wrapper *bf, const void *buffer, int len) {
    if (bf->state != BF_STATE_COLLECT) return -1;
    if (bf->count >= bf->capacity) {
        uint32_t newcap = bf->capacity * 2;
        uint64_t *tmp = (uint64_t *)realloc(bf->keys, newcap * sizeof(uint64_t));
        if (!tmp) return -1;
        bf->keys = tmp;
        bf->capacity = newcap;
    }
    uint64_t h = 0;
    const uint8_t *p = (const uint8_t *)buffer;
    if (len >= 8) {
        memcpy(&h, p, 8);
    } else {
        for (int i = 0; i < len; i++) {
            h = (h << 8) | p[i];
        }
    }
    bf->keys[bf->count++] = h;
    return 0;
}

static inline int bf_build(struct binaryfuse_wrapper *bf) {
    /* Tiny sets are a poor fit for binary fuse (slow/fail-prone); use bloom. */
    if (bf->count < 256) {
        bf->use_bloom_fallback = 1;
        return -1;
    }
    if (bf->use_fuse16) {
        if (!binary_fuse16_allocate(bf->count, &bf->filter16)) {
            bf->use_bloom_fallback = 1;
            return -1;
        }
        if (!binary_fuse16_populate(bf->keys, bf->count, &bf->filter16)) {
            binary_fuse16_free(&bf->filter16);
            bf->use_bloom_fallback = 1;
            return -1;
        }
    } else {
        if (!binary_fuse8_allocate(bf->count, &bf->filter)) {
            bf->use_bloom_fallback = 1;
            return -1;
        }
        if (!binary_fuse8_populate(bf->keys, bf->count, &bf->filter)) {
            binary_fuse8_free(&bf->filter);
            bf->use_bloom_fallback = 1;
            return -1;
        }
    }
    free(bf->keys);
    bf->keys = NULL;
    bf->state = BF_STATE_BUILT;
    return 0;
}

static inline int bf_check(struct binaryfuse_wrapper *bf, const void *buffer, int len) {
    if (bf->state != BF_STATE_BUILT) return -1;
    if (bf->use_bloom_fallback) return -1;
    uint64_t h = 0;
    const uint8_t *p = (const uint8_t *)buffer;
    if (len >= 8) {
        memcpy(&h, p, 8);
    } else {
        for (int i = 0; i < len; i++) {
            h = (h << 8) | p[i];
        }
    }
    if (bf->use_fuse16)
        return binary_fuse16_contain(h, &bf->filter16) ? 1 : 0;
    return binary_fuse8_contain(h, &bf->filter) ? 1 : 0;
}

static inline void bf_free(struct binaryfuse_wrapper *bf) {
    if (bf->keys) { free(bf->keys); bf->keys = NULL; }
    binary_fuse8_free(&bf->filter);
    binary_fuse16_free(&bf->filter16);
    bf->count = 0;
    bf->capacity = 0;
    bf->state = 0;
    bf->use_fuse16 = 0;
}

static inline size_t bf_size_in_bytes(struct binaryfuse_wrapper *bf) {
    if (bf->state == BF_STATE_BUILT) {
        if (bf->use_fuse16)
            return binary_fuse16_size_in_bytes(&bf->filter16);
        return binary_fuse8_size_in_bytes(&bf->filter);
    }
    return bf->capacity * sizeof(uint64_t);
}

#ifdef __cplusplus
}
#endif

#endif
