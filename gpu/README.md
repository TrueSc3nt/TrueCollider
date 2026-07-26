# GPU backends (TrueCollider / KeyCollider)

Runtime selection: `-U none` (default) · `-U cuda` · `-U opencl`.

Dispatcher: `gpu/gpu_dispatcher.cpp`.

## Custom CUDA edition

`keyhunt_cuda.exe` is the **custom CUDA edition**: all TrueCollider modes on `-U cuda`, plus KeyHunt-class **sequential device GRP** (MIT clean-room). CPU twin remains `keyhunt.exe`.

- GRP window: `TCUDA_GRP_SIZE` (1024) in `gpu/cuda/secp256k1_cuda.h`
- Packed targets: `scripts/addr_to_hash160.py` → `-f file.hash160`
- Runtime: NVIDIA driver + CUDA runtime (`cudart64_*.dll` — copy next to the exe on Windows if needed)
- **Do not** expect EC Mk/s rates from mnemonic mode

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
| BTC / LTC / DOGE / XRP / BCH / BTG / `all` — `address` / `rmd160` | **Sequential GRP=1024** (device G-table + batch inv + hash160/bloom) when stride=1; else per-key `scalar_mul_g` |
| ETH / ETC — `address` | GPU EC (uncompressed) + host keccak + host bloom |
| Taproot (`troot`) — `address` | GPU EC + host taproot tweak + filter |
| Batch size | **`-M auto`/`-M MB`** sizes from free VRAM (Rotor/Collider style); **`-G`** optional override. Device launches use 256-thread grids in TDR-safe chunks (up to 64K) |
| Device hash160 bloom search | **Shipped** (`secp_grp_search_kernel` for stride-1; `secp_search_kernel` fallback) |
| vanity / xpoint / pubkey2addr | Stride-1 uses GRP pubkey/search; sparse/random lists keep per-key EC |
| minikeys / mnemonic / poetry / brainwallet | GPU EC + filter (per-key; non-consecutive) |
| BSGS | Baby-table GPU EC + **device GRP giant-step** (`tcuda_bsgs_grp_*`; host bloom). Currently serial per-cycle launches (correct, not yet throughput-tuned) |
| SOL (`-c sol`) | **Full device** ed25519 `ge_scalarmult_base` (SHA512+clamp+ge); host-ge fallback |
| Kangaroo | **CUDA** (`-m kangaroo -U cuda`): ≤2²⁴ GPU batch EC scan; larger multi-walker DP (device jumps, host DP table). CPU fallback |
| OpenCL | Host EC + GPU hash160 only |

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
# Windows: bats/00_build/build_cuda_vs2022.bat → keyhunt_cuda.exe

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

Or double-click / run: `examples/run_gpu_cuda_example.bat`.

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
3. GPU Kangaroo (scan + multi-walker DP). **Done** (not RCKangaroo/SOTA throughput yet; correct collisions).
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
