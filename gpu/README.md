# GPU backends (TrueCollider / KeyCollider)

Runtime selection: `-U none` (default) · `-U cuda` · `-U opencl`.

Dispatcher: `gpu/gpu_dispatcher.cpp`.

## Custom CUDA edition

`keyhunt_cuda.exe` is the **custom CUDA edition**: **CLI parity with `keyhunt.exe`** (every `-m` mode and every flag accepted), plus KeyHunt-class **sequential device GRP** (MIT clean-room). Nothing is compiled out of the CUDA binary’s mode/flag surface.

- GRP window: `TCUDA_GRP_SIZE` (1024) in `gpu/cuda/secp256k1_cuda.h`
- Packed targets: `scripts/addr_to_hash160.py` → `-f file.hash160`
- Runtime: NVIDIA driver + CUDA runtime (`cudart64_*.dll` — copy next to the exe on Windows if needed)
- **Do not** expect EC Mk/s rates from mnemonic mode

---

## CLI parity (`keyhunt.exe` vs `keyhunt_cuda.exe`)

Help dumps match for all `-m` modes and flag letters (banner / exe name differ only). `pub2rmd` is removed on **both** (use `-m rmd160`). Research aliases (`shadow160`, `weakrng`, `hybrid-dl`, `gaudry`, `wif-mask`, `hex-mask`, `kangaroo-mod`, `CreateAccountWithSeed`) map to the same host modes on both binaries.

| Mode / flag | Accepted on CUDA binary | Device acceleration | Host-side remainder |
|-------------|-------------------------|---------------------|---------------------|
| `-m address` + `-x sequential/rseq` | Yes | **GRP** stride-1 BTC-family | Filter confirm / hit I/O |
| `-m address` + `-x chaos/auto/…` | Yes | Host plans bases → **CUDA sequential batches** | Walk planner |
| `-m rmd160` + `-x *` | Yes | Same as address | Walk planner |
| `-m xpoint` / `vanity` / `pubkey2addr` | Yes | GRP or per-key EC | Prefix / table checks |
| `-m bsgs` | Yes | Baby-table + GRP giant-step | Bloom / tables in RAM |
| `-m kangaroo` | Yes | Batch scan (≤2²⁴) / DP walkers | DP table / collision |
| `-m mnemonic` / `poetry` / `brainwallet` | Yes | CUDA EC after derive | **PBKDF2 / BIP39 / paths on host** |
| `-m minikeys` | Yes | CUDA EC batch | Minikey gen on host |
| `-c eth` / `etc` | Yes | GPU EC | **keccak160 encode host** |
| `-c troot` | Yes | GPU EC | Taproot tweak host |
| `-c sol` | Yes | Device ed25519 ge when ready | base58 / fallback |
| `-e` endomorphism | Yes (never rejected) | **Skipped** — CPU endo GRP | Full λ-path on CPU |
| `-x` planners / `--walk` / research `-R` | Yes | Bases feed CUDA when secp path armed | Planner logic host |
| `-U opencl` | Yes if built | Hash160 only | EC on CPU |
| `-U both` | Yes | Even threads CUDA / odd CPU | Hybrid |

### Backend matrix

| Backend | EC (secp256k1) | Address encode | Bloom / filter | Vendors |
|---------|----------------|----------------|----------------|---------|
| **CUDA** | On GPU (`secp256k1_cuda.cu`) | **Device** hash160 (BTC-family) or host keccak (ETH) | **Device** bloom when hash160 self-test passes; host fallback | NVIDIA |
| **OpenCL** | Host CPU | On GPU hash160 (`hash160_opencl`) | Host | NVIDIA, AMD, Intel |
| **CPU** | Host | SSE / AVX2 / AVX-512 | Fuse or bloom | All |

### CUDA path (`-U cuda`)

1. Init: hash160 + secp self-tests → set device vs host filter; `secp_ready`.
2. Upload bloom to device when hash160 OK (else host-only copy).
3. Batch privkeys → device EC (+ device hash160/bloom when enabled).
4. ETH still host keccak; host confirms hits and writes keys.
5. `-e` / non-accelerable bits: **warn and continue on CPU** (never “not available on CUDA”).

**Honest limits:** BIP39 PBKDF2 host; ETH keccak host; taproot tweak host; `-e` CPU-only; BSGS GRP correctness-first (serial cycles); prefer `-t 1` with CUDA.

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
