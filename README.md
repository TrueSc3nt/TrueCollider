# TrueCollider

[![CI](https://github.com/TrueSc3nt/TrueCollider/actions/workflows/ci.yml/badge.svg)](https://github.com/TrueSc3nt/TrueCollider/actions/workflows/ci.yml)
[![Release](https://github.com/TrueSc3nt/TrueCollider/actions/workflows/release.yml/badge.svg)](https://github.com/TrueSc3nt/TrueCollider/releases)

**Multi-currency cryptocurrency key search** — CPU vectorization (SSE / AVX-512), optional **CUDA** (NVIDIA GPU EC) and **OpenCL** (NVIDIA / AMD / Intel hash160 offload), binary-fuse / bloom filters, BIP-39, BSGS, and more.

Based on Keyhunt by Alberto · Developed & modified by **TrueScent**

---

## Features

| Area | What you get |
|------|----------------|
| **Search modes (CPU)** | `address`, `pubkey2addr`, `rmd160`, `xpoint`, `bsgs`, `vanity`, `minikeys`, `mnemonic`, `poetry`, `brainwallet` |
| **Currencies** | BTC, ETH, LTC, DOGE, XRP, BCH, BTG, ETC, Taproot (`troot`), `all`, `auto` |
| **CPU speed** | Multi-thread · GLV endomorphism (`-e`) · SSE 4-wide hash · AVX-512 16-wide hash (`-A avx512`) |
| **CUDA (NVIDIA)** | On-device secp256k1 pubkey derivation + host hash/bloom for `address` / `rmd160` |
| **OpenCL (NVIDIA/AMD/Intel)** | Batch hash160 offload for compress `address` / `rmd160` (EC stays on CPU) |
| **Filters** | Binary fuse (falls back to bloom on tiny sets) · BSGS multi-tier blooms |
| **Patterns** | `sequential`, `random`, `chaos`, `gravity`, `spiral`, `reverse`, `auto` |
| **Other** | BIP-32 `-p` / `-D` · BIP-39 10 languages · node balance `-N` · vanity `-v` |

> **Solana (`-c sol`):** flag exists for future work — **ed25519 is not implemented** yet. Do not use for real SOL searches.

---

## CPU vs GPU — what actually runs where

| Mode | CPU | CUDA (`-U cuda`) | OpenCL (`-U opencl`) |
|------|-----|------------------|----------------------|
| `address` / `rmd160` | Full | GPU EC + host hash/bloom (stable batch=1) · optional hash offload | Host EC + GPU hash160 (compress, no `-e`) |
| `vanity` | Full | CPU only | CPU only |
| `xpoint` | Full | CPU only | CPU only |
| `bsgs` | Full | CPU only | CPU only |
| `minikeys` / `mnemonic` / `poetry` / `brainwallet` / `pubkey2addr` | Full | CPU only | CPU only |

**Notes**
- CUDA device **hash160 self-test currently fails** → live CUDA path uses **host** SHA256+RIPEMD160 + host bloom (EC still on GPU).
- CUDA **does not use `-e`** (endomorphism). For max coverage with endo, use CPU `-e`.
- On many CPUs (SSE/AVX-512), **CPU throughput beats** the current CUDA batch=1 path. Use GPU when you want GPU EC offload / dual-path testing.
- Larger CUDA batches are clamped for Windows TDR / stability; `-G` sets a hint (may be clamped).

---

## Quick start

```bash
# Help
./keyhunt -h

# Fast CPU search (AVX-512 if your CPU has it; else auto SSE)
./keyhunt -m address -f targets.txt -A avx512 -e -t 8

# CUDA (NVIDIA) — GPU EC
./keyhunt -m address -f targets.txt -U cuda -t 1 -l compress -s 5

# OpenCL (NVIDIA / AMD / Intel) — GPU hash160, CPU EC
./keyhunt -m address -f targets.txt -U opencl -t 8 -l compress

# BSGS puzzle
./keyhunt -m bsgs -f pubkeys.txt -B sequential -n 0x1000000 -t 4
```

Windows Desktop binaries (when built locally):

```text
keyhunt.exe          # MinGW CPU build
keyhunt_cuda.exe     # MSVC + CUDA build
```

---

## Build

### Linux / macOS (CPU)

```bash
git clone https://github.com/TrueSc3nt/TrueCollider.git
cd TrueCollider
make -j$(nproc)          # or: cmake -B build && cmake --build build -j
./keyhunt -h
```

### Windows native CPU (MSYS2 MinGW)

```bat
build_mingw_native.bat
REM → keyhunt.exe
```

### Windows CUDA (NVIDIA + VS 2022 Build Tools + CUDA 12.x)

```bat
build_cuda_vs2022.bat
REM → keyhunt_cuda.exe  (ENABLE_CUDA=ON)
```

### OpenCL (AMD / NVIDIA / Intel)

Requires OpenCL ICD + SDK headers (`CL/cl.h`).

```bash
cmake -B build-opencl -DENABLE_OPENCL=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-opencl -j
./build-opencl/keyhunt -m address -f targets.txt -U opencl -t 8 -l compress
```

**Drivers**
- **NVIDIA:** CUDA Toolkit (for `-U cuda`) and/or NVIDIA OpenCL ICD (for `-U opencl`)
- **AMD:** Adrenalin / ROCm OpenCL ICD
- **Intel:** Intel OpenCL runtime (iGPU)

See [docs/BUILD.md](docs/BUILD.md) for Termux, MinGW cross-compile, and CMake options.

---

## Performance tips (CPU / RAM / pagefile)

### Will a larger Windows pagefile make AVX faster?

**No.** AVX / AVX-512 speed comes from **CPU instructions and cache**, not from disk-backed virtual memory.

| Change | Effect on key search |
|--------|----------------------|
| Bigger **pagefile** | Only helps if you are **out of RAM** (avoids crashes). If the OS is **paging** the working set, search **slows down** a lot. |
| More **physical RAM** | Lets larger blooms / BSGS tables / fuse filters stay in memory → fewer stalls. |
| **`-A avx512`** on a real AVX-512 CPU | Biggest CPU hash win (16-wide). Auto falls back to SSE if unsupported. |
| **`-e` endomorphism** | ~3× more address checks per EC base (CPU `address` / `rmd160` / `vanity`). |
| **More threads `-t`** | Scale with physical cores; oversubscribe can hurt. |
| **Large pages** | Not implemented yet; possible future win for huge bloom/BSGS tables. |

**Practical advice:** size RAM for your filter/table; keep pagefile as a safety net, not a speed knob. Prefer CPU `-A avx512 -e -t N` on this class of workload unless you are developing/testing the CUDA path.

### CPU vector levels (`-A`)

**Default is `auto`:** at startup the binary CPUID-detects the best ISA and uses it.

| Level | What runs |
|-------|-----------|
| `auto` (default) | Detect best: AVX-512 → AVX2 → AVX → SSE → scalar |
| `none` | Scalar SHA256 / RIPEMD160 |
| `sse` | 4-wide SSE kernels |
| `avx` / `avx2` | Detected & selected; hash path uses SSE 4-wide (no separate AVX2 hash kernel yet) |
| `avx512` | 16-wide hash160 when AVX-512F+BW present + self-test OK |

```bash
./keyhunt -m address -f targets.txt -A auto -e -t 8   # default behavior
./keyhunt -m address -f targets.txt -A avx512 -e -t 8 # force AVX-512 (falls back if unsupported)
```


---

## GPU backends (`-U`)

```bash
-U none      # CPU only (default)
-U cuda      # NVIDIA: GPU secp256k1 + host filter (address/rmd160)
-U opencl    # NVIDIA/AMD/Intel: GPU hash160; EC on CPU
-G N         # GPU batch size hint (CUDA may clamp for stability)
```

Details and roadmap: [gpu/README.md](gpu/README.md).

---

## Search modes (`-m`)

### `address` (default)

Private key → pubkey → address vs target file.

```bash
./keyhunt -m address -f targets.txt -t 8
./keyhunt -m address -c eth -f eth.txt -t 8
./keyhunt -m address -c all -f mixed.txt -t 8
./keyhunt -m address -c auto -f mixed.txt -t 8
./keyhunt -m address -f targets.txt -b 66 -l compress -e -t 8
./keyhunt -m address -f targets.txt -x auto -e -t 8
```

### `rmd160`

Match raw 40-hex RIPEMD-160 digests (no Base58).

```bash
./keyhunt -m rmd160 -f hashes.rmd -l compress -t 8
```

### `vanity`

```bash
./keyhunt -m vanity -v 1Cool -e -t 8
```

### `bsgs`

Public-key discrete log (puzzle) search. CPU only.

```bash
./keyhunt -m bsgs -f pubkeys.txt -B sequential -n 0x1000000 -t 4
./keyhunt -m bsgs -f pubkeys.txt -x auto -S -t 8
```

### `xpoint`

Match pubkey X coordinates.

```bash
./keyhunt -m xpoint -f pubkeys.txt -t 8
```

### `pubkey2addr`

Random-key address search (defaults toward random coverage).

```bash
./keyhunt -m pubkey2addr -f targets.txt -x auto -t 4
```

### `mnemonic` / `poetry` / `brainwallet` / `minikeys`

```bash
./keyhunt -m mnemonic -w 24 -L english -f targets.txt -t 4
./keyhunt -m poetry -f targets.txt -t 4
./keyhunt -m brainwallet -w 3 -f targets.txt -t 8
./keyhunt -m minikeys -f targets.txt -t 4
```

---

## Cryptocurrencies (`-c`)

| Flag | Notes |
|------|--------|
| `btc` | Legacy / P2SH / bc1q (default) |
| `eth` / `etc` | Keccak |
| `ltc` / `doge` / `xrp` / `bch` / `btg` | Coin-specific encodings |
| `troot` | Taproot bc1p / x-only |
| `all` | Multi-currency in one run |
| `auto` | Detect from file content |
| `sol` | **Not implemented** (placeholder) |

---

## Important flags

| Flag | Meaning |
|------|---------|
| `-m mode` | Search mode |
| `-f file` | Targets |
| `-t N` | Threads |
| `-e` | GLV endomorphism (CPU) |
| `-A` | `none` / `sse` / `avx` / `avx2` / `avx512` |
| `-U` | `none` / `cuda` / `opencl` |
| `-G N` | GPU batch hint |
| `-l` | `compress` / `uncompress` / `both` |
| `-x` | Search pattern |
| `-r` / `-b` / `-T` | Range / bits / timestamp window |
| `-p` / `-D` | BIP-32 path + index count |
| `-s` / `-q` / `-V` | Stats interval / quiet / verbose |
| `-N[url]` | Balance check on find |

Full help: `./keyhunt -h`

---

## File formats

- **Addresses:** one per line (BTC/ETH/LTC/…)
- **RMD160:** 40 hex chars per line
- **Pubkeys (BSGS/xpoint):** 64 / 66 / 130 hex
- **Found keys:** `KEYFOUNDKEYFOUND.txt`

---

## Roadmap (GPU)

1. Fix device hash160 on CUDA → re-enable on-GPU bloom.
2. Safe multi-key CUDA batches (avoid TDR) for higher GPU throughput.
3. OpenCL secp256k1 (AMD + NVIDIA) mirroring the CUDA EC path.
4. Optional large-page allocation for huge filters/tables.
5. Wire vanity onto the GPU secp helper.
6. Real Solana / ed25519 path if demanded.

---

## Docs

- [docs/BUILD.md](docs/BUILD.md) — platforms & CMake options  
- [gpu/README.md](gpu/README.md) — CUDA / OpenCL details  
- [docs/BENCHMARK.md](docs/BENCHMARK.md) — benchmarking notes  
- [docs/FILTER_VERIFICATION.md](docs/FILTER_VERIFICATION.md) — filter checks  

---

## License

See [LICENSE](LICENSE).

## Donations

- BTC: `1HmztBLDnwwaKAGbtALsYvCNBuoJYEic3h`  
- Tips: `bc1q39meky2mn5qjq704zz0nnkl0v7kj4uz6r529at`
