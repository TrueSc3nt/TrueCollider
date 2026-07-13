/*
 * OpenCL backend scaffolding for TrueCollider.
 *
 * This is a minimal stub that compiles when -DENABLE_OPENCL=ON is passed.
 * A production implementation would compile EC math kernels at runtime,
 * support AMD/Intel/NVIDIA GPUs, and dispatch batch search work.
 */

#include <cstdio>

#if defined(__APPLE__)
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

extern "C" int topencl_device_count(void) {
    cl_uint platform_count = 0;
    cl_int err = clGetPlatformIDs(0, nullptr, &platform_count);
    if (err != CL_SUCCESS || platform_count == 0) {
        return 0;
    }

    cl_uint total_devices = 0;
    for (cl_uint p = 0; p < platform_count; p++) {
        cl_uint device_count = 0;
        clGetDeviceIDs(nullptr, CL_DEVICE_TYPE_GPU, 0, nullptr, &device_count);
        total_devices += device_count;
    }
    return (int)total_devices;
}

extern "C" int topencl_hello(void) {
    int count = topencl_device_count();
    std::printf("[OpenCL] GPU devices detected: %d\n", count);
    return count;
}
