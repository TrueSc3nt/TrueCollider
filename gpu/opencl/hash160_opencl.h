#ifndef HASH160_OPENCL_H
#define HASH160_OPENCL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int topencl_hash160_33_selftest(void);
int topencl_hash160_33_batch(const uint8_t *host_keys, int count, uint8_t *host_out);

#ifdef __cplusplus
}
#endif

#endif
