# GPU backends (TrueCollider / KeyCollider)

Runtime selection: `-U none` (default) · `-U cuda` · `-U opencl`.

Dispatcher: `gpu/gpu_dispatcher.cpp`.

---

## Status (honest)

| Backend | EC (secp256k1) | Address encode | Bloom / filter | Vendors |
|---------|----------------|----------------|----------------|---------|
| **CUDA** | On GPU (`secp256k1_cuda.cu`) | Host hash160 **or** host keccak (ETH) | Host bloom | NVIDIA |
| **OpenCL** | Host CPU | On GPU hash160 (`hash160_opencl`) | Host | NVIDIA, AMD, Intel |
| **CPU** | Host | SSE / AVX2 / AVX-512 | Fuse or bloom | All |

### What CUDA accelerates today

| Path | Status |
|------|--------|
| BTC / LTC / DOGE / XRP / BCH / BTG / `all` — `address` / `rmd160` | GPU EC + host hash160 + host bloom |
| ETH / ETC — `address` | GPU EC (uncompressed) + host keccak + host bloom |
| Taproot (`troot`) — `address` | GPU EC + host taproot tweak + filter |
| Batch size | Default **128**, clamp **1..256** (`-G`); device launches chunked (128) for TDR |
| Device hash160 bloom search | Implemented but **not** production (self-test historically failed) |
| BSGS / vanity / SOL / kangaroo | CPU (GPU BSGS / ed25519 / kangaroo on roadmap) |

### CUDA path (`-U cuda`)

1. Init: secp self-test → `secp_ready`.
2. Upload bloom metadata for host-filter mode.
3. Batch privkeys → `tcuda_secp_pubkey_batch` (device EC, chunked).
4. Host encode (`hash160` or `keccak`) + `bloom_check`.
5. CPU confirms hits and writes keys.

**Limits:** no `-e` on GPU EC; prefer low `-t` with CUDA; Solana stays CPU.

### OpenCL path (`-U opencl`)

1. Enumerate platforms (prefers NVIDIA/AMD by compute units).
2. Self-test batch hash160 of compressed pubkeys.
3. Main loop: CPU EC → `gpu_dispatcher_hash160_33` → host filter (when AVX-512 path is not active and `-e` is off).

---

## Build

```bash
# NVIDIA CUDA
cmake -B build-cuda -DENABLE_CUDA=ON
cmake --build build-cuda -j
# Windows: build_cuda_vs2022.bat → keyhunt_cuda.exe

# OpenCL (AMD / NVIDIA / Intel)
cmake -B build-opencl -DENABLE_OPENCL=ON
cmake --build build-opencl -j
```

---

## Examples

```bash
# BTC-family on CUDA
./keyhunt -m address -f targets.txt -U cuda -G 128 -t 1 -l compress -q -s 5

# Ethereum on CUDA
./keyhunt -m address -c eth -f eth.txt -U cuda -G 128 -t 1 -q -s 5

# Taproot on CUDA
./keyhunt -m address -c troot -f troot.txt -U cuda -G 128 -t 1 -q -s 5

# OpenCL hash offload
./keyhunt -m address -f targets.txt -U opencl -t 8 -l compress
```

Windows:

```bat
keyhunt_cuda.exe -m address -f tests\66.txt -b 66 -l compress -U cuda -G 128 -t 1 -q -s 5
```

Or double-click / run: `run_gpu_cuda_example.bat`.

---

## Flags

| Flag | Meaning |
|------|---------|
| `-U cuda\|opencl\|none` | Backend |
| `-G N` | Batch size hint (CUDA EC uses `1..256`, default `128`) |

---

## API surface (for developers)

| Symbol | Role |
|--------|------|
| `gpu_dispatcher_search_privkeys(..., encode)` | GPU EC + host hash160 or ETH keccak + bloom |
| `gpu_dispatcher_pubkey_batch` | GPU EC only (Taproot / future BSGS helpers) |
| `gpu_dispatcher_hash160_33` | CUDA/OpenCL hash160 of compressed pubs |
| `GPU_ENCODE_HASH160` / `GPU_ENCODE_ETH` | Encode selectors |

---

## Roadmap (GPU)

1. Trusted device hash160 → full on-device bloom path.
2. GPU BSGS giant-step kernels (baby tables stay RAM-heavy).
3. GPU Kangaroo (SOTA for large pubkey puzzles).
4. GPU Solana ed25519 grind.
5. OpenCL secp EC twin for AMD.

See also: [docs/ROADMAP.md](../docs/ROADMAP.md).

---

## Layout

```
gpu/
  gpu_dispatcher.cpp|.h     # -U routing
  cuda/                     # CUDA secp + hash160 + bridge
  opencl/                   # OpenCL hash160
```
