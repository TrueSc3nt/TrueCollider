# GPU Backend (Experimental)

This directory contains scaffolding for optional CUDA and OpenCL backends. The current code only detects devices and prints a count; the actual EC math kernels are not yet implemented.

## Why GPU?

Bitcoin private key search is embarrassingly parallel: each candidate key is independent, and the hot loop is secp256k1 scalar multiplication followed by SHA-256 / RIPEMD-160 hashing. A GPU can execute thousands of these in parallel, making it the biggest potential speedup for 2026-era tools.

## Status

| Backend | Status | Notes |
|---------|--------|-------|
| CUDA    | Scaffolding | Device detection stub compiles; kernels TODO |
| OpenCL  | Scaffolding | Device detection stub compiles; kernels TODO |

## Building with GPU support

```bash
# CUDA
cmake -B build-cuda -DENABLE_CUDA=ON
cmake --build build-cuda -j$(nproc)

# OpenCL
cmake -B build-opencl -DENABLE_OPENCL=ON
cmake --build build-opencl -j$(nproc)
```

## Planned GPU kernel layout

1. `secp256k1_mul_kernel` — batch scalar multiplications of the base point G.
2. `hash160_kernel` — batch SHA-256 then RIPEMD-160 of the compressed public key.
3. `filter_lookup_kernel` — XOR-fuse filter membership test on device.
4. `reduce_kernel` — collect candidate keys that pass both filter and confirmation.

## Integration plan

The main `keyhunt.cpp` will eventually delegate to a `GPUWorkDispatcher` that:

- Fills a GPU input buffer with `N` candidate private keys.
- Runs the EC + hash160 pipeline in one or many CUDA streams / OpenCL command queues.
- Copies back a small list of candidates for the CPU to confirm.
- Keeps the CPU busy with the next batch while the GPU works.

A `CPU_GRP_SIZE` value will remain for CPU-only builds, and a new `GPU_BATCH_SIZE` will control how many keys are dispatched to the GPU per launch.

## Contributing

If you have CUDA/OpenCL experience, the missing pieces are:

- A secp256k1 point-multiplication kernel that matches the `Int/Point` logic in `secp256k1/`.
- A GPU-friendly Bloom/binary-fuse lookup.
- Benchmark harness comparing CPU vs GPU throughput.

Open a PR against `https://github.com/TrueSc3nt/TrueCollider`.
