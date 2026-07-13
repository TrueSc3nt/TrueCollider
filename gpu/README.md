# GPU backends

Runtime selection: `-U none` (default) · `-U cuda` · `-U opencl`.

Dispatcher: `gpu/gpu_dispatcher.cpp`.

## Status (honest)

| Backend | EC (secp256k1) | hash160 | Bloom / filter | Vendors |
|---------|----------------|---------|----------------|---------|
| **CUDA** | On GPU (`secp256k1_cuda.cu`) | Host (device self-test fails) | Host bloom | NVIDIA |
| **OpenCL** | Host CPU | On GPU (`hash160_opencl`) | Host | NVIDIA, AMD, Intel |
| **CPU** | Host | SSE / AVX-512 | Fuse or bloom | All |

### CUDA path (`-U cuda`)

1. Upload / keep bloom on host (`tcuda_secp_search_init`, host-filter mode).
2. Batch privkeys → `tcuda_secp_pubkey_batch` (device EC).
3. Host SHA256 + RIPEMD160 + `bloom_check`.
4. CPU confirms hits and writes keys.

**Limits today:** search batch forced to **1** for driver stability on some Windows hosts; no `-e` on the GPU secp helper; vanity/BSGS/etc. stay CPU.

### OpenCL path (`-U opencl`)

1. Enumerate GPU platforms (prefers NVIDIA/AMD by compute units).
2. Self-test batch hash160 of compressed pubkeys.
3. Main loop: CPU EC → `gpu_dispatcher_hash160_33` → host filter (when AVX-512 path is not active and `-e` is off).

**Requires:** OpenCL ICD + headers. Works with Adrenalin (AMD), NVIDIA OpenCL, Intel GPU runtimes.

## Build

```bash
# NVIDIA CUDA
cmake -B build-cuda -DENABLE_CUDA=ON
cmake --build build-cuda -j
# Windows helper: build_cuda_vs2022.bat → keyhunt_cuda.exe

# OpenCL (AMD / NVIDIA / Intel)
cmake -B build-opencl -DENABLE_OPENCL=ON
cmake --build build-opencl -j
```

```bash
./keyhunt -m address -f targets.txt -U cuda -t 1 -l compress
./keyhunt -m address -f targets.txt -U opencl -t 8 -l compress
```

## Flags

| Flag | Meaning |
|------|---------|
| `-U cuda\|opencl\|none` | Backend |
| `-G N` | Batch size **hint** (CUDA may clamp) |

## Roadmap

1. Correct CUDA device hash160 → device-side bloom.
2. Larger safe CUDA batches (chunked launches under TDR limits).
3. OpenCL secp256k1 EC (parity with CUDA for AMD).
4. Vanity on GPU secp helper.
5. Optional large pages for huge host filters.

## Layout

```
gpu/
  gpu_dispatcher.cpp|.h     # -U routing
  cuda/                     # CUDA secp + hash160 + bridge
  opencl/                   # OpenCL hash160
```
