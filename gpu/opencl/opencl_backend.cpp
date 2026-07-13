/*
 * OpenCL hash160 backend for 33-byte compressed public keys (phase 1).
 */

#include "hash160_opencl.h"
#include "hash160_opencl_src.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#if defined(__APPLE__)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "../../hash/sha256.h"
#include "../../hash/ripemd160.h"

struct OpenCLHashState {
    cl_context ctx;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    int ready;
};

static OpenCLHashState g_ocl = {0};

static int ocl_init(void) {
    if (g_ocl.ready) return 1;

    cl_uint nplat = 0;
    if (clGetPlatformIDs(0, NULL, &nplat) != CL_SUCCESS || nplat == 0) return 0;

    std::vector<cl_platform_id> plats(nplat);
    clGetPlatformIDs(nplat, plats.data(), NULL);

    cl_device_id device = NULL;
    for (cl_uint p = 0; p < nplat && !device; p++) {
        cl_uint ndev = 0;
        if (clGetDeviceIDs(plats[p], CL_DEVICE_TYPE_GPU, 0, NULL, &ndev) != CL_SUCCESS || ndev == 0)
            continue;
        std::vector<cl_device_id> devs(ndev);
        clGetDeviceIDs(plats[p], CL_DEVICE_TYPE_GPU, ndev, devs.data(), NULL);
        device = devs[0];
    }
    if (!device) {
        /* Fall back to any device (CPU OpenCL). */
        for (cl_uint p = 0; p < nplat && !device; p++) {
            cl_uint ndev = 0;
            if (clGetDeviceIDs(plats[p], CL_DEVICE_TYPE_ALL, 0, NULL, &ndev) != CL_SUCCESS || ndev == 0)
                continue;
            std::vector<cl_device_id> devs(ndev);
            clGetDeviceIDs(plats[p], CL_DEVICE_TYPE_ALL, ndev, devs.data(), NULL);
            device = devs[0];
        }
    }
    if (!device) return 0;

    cl_int err = 0;
    g_ocl.ctx = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) return 0;
    g_ocl.queue = clCreateCommandQueue(g_ocl.ctx, device, 0, &err);
    if (err != CL_SUCCESS) return 0;

    const char *src = HASH160_OPENCL_SRC;
    size_t len = strlen(src);
    g_ocl.program = clCreateProgramWithSource(g_ocl.ctx, 1, &src, &len, &err);
    if (err != CL_SUCCESS) return 0;
    err = clBuildProgram(g_ocl.program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        size_t log_size = 0;
        clGetProgramBuildInfo(g_ocl.program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        std::vector<char> log(log_size + 1);
        clGetProgramBuildInfo(g_ocl.program, device, CL_PROGRAM_BUILD_LOG, log_size, log.data(), NULL);
        fprintf(stderr, "[OpenCL] build failed:\n%s\n", log.data());
        return 0;
    }
    g_ocl.kernel = clCreateKernel(g_ocl.program, "hash160_33_kernel", &err);
    if (err != CL_SUCCESS) return 0;
    g_ocl.ready = 1;
    return 1;
}

extern "C" int topencl_device_count(void) {
    cl_uint nplat = 0;
    if (clGetPlatformIDs(0, NULL, &nplat) != CL_SUCCESS || nplat == 0) return 0;
    std::vector<cl_platform_id> plats(nplat);
    clGetPlatformIDs(nplat, plats.data(), NULL);
    int total = 0;
    for (cl_uint p = 0; p < nplat; p++) {
        cl_uint ndev = 0;
        if (clGetDeviceIDs(plats[p], CL_DEVICE_TYPE_GPU, 0, NULL, &ndev) == CL_SUCCESS)
            total += (int)ndev;
    }
    return total;
}

extern "C" int topencl_hello(void) {
    int count = topencl_device_count();
    printf("[OpenCL] GPU devices detected: %d\n", count);
    return count;
}

extern "C" int topencl_hash160_33_batch(const uint8_t *host_keys, int count, uint8_t *host_out) {
    if (count <= 0 || !host_keys || !host_out) return 0;
    if (!ocl_init()) return 0;

    cl_int err = 0;
    size_t keys_bytes = (size_t)count * 33;
    size_t out_bytes = (size_t)count * 20;
    cl_mem d_keys = clCreateBuffer(g_ocl.ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                   keys_bytes, (void*)host_keys, &err);
    if (err != CL_SUCCESS) return 0;
    cl_mem d_out = clCreateBuffer(g_ocl.ctx, CL_MEM_WRITE_ONLY, out_bytes, NULL, &err);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(d_keys);
        return 0;
    }

    clSetKernelArg(g_ocl.kernel, 0, sizeof(cl_mem), &d_keys);
    clSetKernelArg(g_ocl.kernel, 1, sizeof(cl_mem), &d_out);
    clSetKernelArg(g_ocl.kernel, 2, sizeof(int), &count);

    size_t global = (size_t)((count + 255) / 256) * 256;
    err = clEnqueueNDRangeKernel(g_ocl.queue, g_ocl.kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(d_keys);
        clReleaseMemObject(d_out);
        return 0;
    }
    clEnqueueReadBuffer(g_ocl.queue, d_out, CL_TRUE, 0, out_bytes, host_out, 0, NULL, NULL);
    clReleaseMemObject(d_keys);
    clReleaseMemObject(d_out);
    return 1;
}

extern "C" int topencl_hash160_33_selftest(void) {
    const uint8_t pubkey[33] = {
        0x02, 0x79, 0xbe, 0x67, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0, 0x62,
        0x95, 0xce, 0x87, 0x0b, 0x07, 0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce, 0x28,
        0xd9, 0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98
    };
    uint8_t expected[20], sha[32], padded[64];
    memcpy(padded, pubkey, 33);
    padded[33] = 0x80;
    memset(padded + 34, 0, 29);
    padded[62] = 0x01;
    padded[63] = 0x08;
    sha256_33(padded, sha);
    ripemd160_32(sha, expected);

    uint8_t keys[33 * 16];
    uint8_t out[20 * 16];
    for (int i = 0; i < 16; i++) memcpy(keys + i * 33, pubkey, 33);
    if (!topencl_hash160_33_batch(keys, 16, out)) {
        fprintf(stderr, "[E] topencl_hash160_33_selftest: batch failed.\n");
        return 0;
    }
    if (memcmp(out, expected, 20) != 0) {
        fprintf(stderr, "[E] topencl_hash160_33_selftest: hash mismatch.\n");
        return 0;
    }
    printf("[+] OpenCL hash160 self-test passed.\n");
    return 1;
}
