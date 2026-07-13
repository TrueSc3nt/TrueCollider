/*
 * CUDA backend scaffolding for TrueCollider.
 *
 * This is a minimal stub that compiles when -DENABLE_CUDA=ON is passed.
 * A production implementation would add secp256k1 EC math kernels,
 * batch hash160, and device-side filter lookup.
 */

#include <cuda_runtime.h>
#include <stdio.h>

extern "C" int tcuda_device_count(void) {
    int count = 0;
    cudaError_t err = cudaGetDeviceCount(&count);
    if (err != cudaSuccess) {
        return 0;
    }
    return count;
}

extern "C" int tcuda_hello(void) {
    int count = tcuda_device_count();
    printf("[CUDA] Devices detected: %d\n", count);
    if (count > 0) {
        /* EC kernels need more than the default ~1KB per-thread stack. */
        cudaDeviceSetLimit(cudaLimitStackSize, 64 * 1024);
    }
    return count;
}
