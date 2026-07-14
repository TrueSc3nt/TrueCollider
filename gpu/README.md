# GPU backends (TrueCollider / KeyCollider)

Runtime selection: `-U none` (default) · `-U cuda` · `-U opencl`.

Dispatcher: `gpu/gpu_dispatcher.cpp`.

---

## Status (honest)

| Backend | EC (secp256k1) | Address encode | Bloom / filter | Vendors |
|---------|----------------|----------------|----------------|---------|
| **CUDA** | On GPU (`secp256k1_cuda.cu`) | **Device** hash160 (BTC-family) or host keccak (ETH) | **Device** bloom when hash160 self-test passes; host fallback | NVIDIA |
| **OpenCL** | Host CPU | On GPU hash160 (`hash160_opencl`) | Host | NVIDIA, AMD, Intel |
| **CPU** | Host | SSE / AVX2 / AVX-512 | Fuse or bloom | All |

### What CUDA accelerates today

| Path | Status |
|------|--------|
| BTC / LTC / DOGE / XRP / BCH / BTG / `all` — `address` / `rmd160` | GPU EC + **device** hash160 + **device** bloom (host fallback if self-test fails) |
| ETH / ETC — `address` | GPU EC (uncompressed) + host keccak + host bloom |
| Taproot (`troot`) — `address` | GPU EC + host taproot tweak + filter |
| Batch size | **`-M auto`/`-M MB`** sizes from free VRAM (Rotor/Collider style); **`-G`** optional override. Device launches use 256-thread grids in TDR-safe chunks (up to 64K) |
| Device hash160 bloom search | **Shipped** (`secp_search_kernel` when self-test passes; `g_host_filter=0`) |
| vanity / xpoint / pubkey2addr / minikeys / mnemonic / poetry / brainwallet | GPU EC + filter (device hash160+bloom when ready; else host) |
| BSGS | Baby-table GPU EC + **device GRP giant-step** (`tcuda_bsgs_grp_*`; host bloom). Currently serial per-cycle launches (correct, not yet throughput-tuned) |
| SOL (`-c sol`) | **Full device** ed25519 `ge_scalarmult_base` (SHA512+clamp+ge); host-ge fallback |
| Kangaroo | CPU (`-m kangaroo`) |

### CUDA path (`-U cuda`)

1. Init: hash160 + secp self-tests → set device vs host filter; `secp_ready`.
2. Upload bloom to device when hash160 OK (else host-only copy).
3. Batch privkeys → device EC (+ device hash160/bloom when enabled).
4. ETH still host keccak; host confirms hits and writes keys.

**Limits:** no `-e` on GPU EC; prefer low `-t` with CUDA; BSGS GRP on GPU is correctness-first (serial cycles).

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
| `-G N` | Batch size hint (clamped by `-M` / VRAM plan; large grids chunked) |
| `-M MB\|auto\|2G` | Host/VRAM memory budget (CUDA sizes batches; BSGS scales blooms) |

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

1. Trusted device hash160 → full on-device bloom path. **Done.**
2. GPU BSGS giant-step GRP kernels (baby tables stay RAM-heavy). **Done** (serial; optimize next).
3. GPU Kangaroo (SOTA for large pubkey puzzles).
4. Throughput-tune device Solana ed25519 grind + base58 prefilter.
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
