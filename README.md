# TrueCollider

[![CI](https://github.com/TrueSc3nt/TrueCollider/actions/workflows/ci.yml/badge.svg)](https://github.com/TrueSc3nt/TrueCollider/actions/workflows/ci.yml)
[![Release](https://github.com/TrueSc3nt/TrueCollider/actions/workflows/release.yml/badge.svg)](https://github.com/TrueSc3nt/TrueCollider/releases)

**Multi-currency private-key / seed search** for puzzles, vanity, and research.

Based on [Keyhunt](https://github.com/albertobsd/keyhunt) by Alberto · Developed & modified by **TrueScent**

> **New here?** Start with **[docs/GETTING_STARTED.md](docs/GETTING_STARTED.md)** (Windows & Linux, copy-paste).  
> **Want every recipe?** **[docs/COMMANDS.md](docs/COMMANDS.md)**  
> **What’s next?** **[docs/ROADMAP.md](docs/ROADMAP.md)**

---

## TL;DR

### 1) Build (Linux / macOS)

```bash
git clone https://github.com/TrueSc3nt/TrueCollider.git
cd TrueCollider
make -j$(nproc)
./keyhunt -h
```

### 2) Build (Windows CPU — MSYS2 MinGW)

```bat
build_mingw_native.bat
REM → keyhunt.exe
keyhunt.exe -h
```

### 3) Run a Bitcoin-style address search (puzzle 66 example)

```bash
./keyhunt -m address -f tests/66.txt -b 66 -l compress -R -q -s 10 -t 8 -e -A auto
```

### 4) Run Solana (ed25519)

```bash
./keyhunt -m address -c sol -f tests/sol_sample.txt -r 1:8 -t 1
```

### 5) BSGS (you have a **public key**, not only an address)

```bash
./keyhunt -m bsgs -f tests/125.txt -b 125 -q -s 10 -R -t 8
```

Hits are written to **`KEYFOUNDKEYFOUND.txt`**.

Add `-t` threads for speed. Use `-A auto` (default) for SSE / AVX2 / AVX-512 hash kernels.

---

## What TrueCollider is (and is not)

| It is | It is not |
|-------|-----------|
| A fast multi-mode **key hunter** (CPU + optional GPU helpers) | A guarantee you will find random wallets |
| Useful for **puzzles**, **known ranges**, **vanity**, **wordlists** | Practical for scanning all 2^256 keys |
| Honest about which modes use GPU | “AI finds private keys” marketing |

Stay on **puzzles / ranges you understand**. Full-space brute force is not realistic.

---

## Which mode should I use?

```text
Do you have only an ADDRESS?     →  -m address   (-c btc/eth/sol/…)
Do you have RMD160 hex?          →  -m rmd160
Do you have a PUBLIC KEY?        →  -m bsgs      (CPU; kangaroo planned)
Want a custom prefix (1Cool…)?   →  -m vanity
BIP39 words / poetry / brain?    →  -m mnemonic / poetry / brainwallet
```

| Mode | Input file | Typical use |
|------|------------|-------------|
| `address` | addresses | Default hunt |
| `rmd160` | 40-hex hashes | Faster than full address decode path |
| `xpoint` | X coords | Pubkey X matching |
| `bsgs` | pubkeys | Discrete log in a bit/range |
| `vanity` | optional file + `-v` | Prefix grind |
| `minikeys` | addresses | Casascius-style minikeys |
| `mnemonic` | addresses | BIP39 phrases |
| `poetry` / `brainwallet` | addresses | Wordlist → key |
| `pubkey2addr` | addresses | Pubkey conversion checks |

Full flag list: `./keyhunt -h` · recipes: [COMMANDS.md](docs/COMMANDS.md).

---

## Currencies (`-c`)

| Flag | Notes |
|------|--------|
| `btc` | Legacy / P2SH / bc1q (default) |
| `eth` / `etc` | `0x` Keccak |
| `ltc` / `doge` / `xrp` / `bch` / `btg` | Coin encodings |
| `troot` | Taproot `bc1p` / x-only |
| `sol` | **Solana** ed25519 seed → base58 pubkey (CPU `address`) |
| `all` | Multiple **secp** coins in one run |
| `auto` | Detect from file contents |

Solana is **not** secp256k1: no `-e`, no BIP32 `-p`, no CUDA/OpenCL path yet.

---

## Features at a glance

| Area | Details |
|------|---------|
| **CPU** | Threads · `-e` endomorphism · SSE 4-wide · **AVX2 8-wide** · **AVX-512 16-wide** (`-A`) |
| **CUDA** | NVIDIA secp EC for `address` / `rmd160` (hash often on host; batch may be clamped) |
| **OpenCL** | NVIDIA/AMD/Intel hash160 offload; EC on CPU |
| **Filters** | Binary fuse → bloom fallback · BSGS blooms |
| **Patterns** | `sequential` `random` `chaos` `gravity` `spiral` `reverse` `auto` (`-x`) |
| **Other** | BIP32 `-p`/`-D` · BIP39 languages · node `-N` · vanity `-v` |

---

## CPU vs GPU (read this)

| Mode | CPU | `-U cuda` | `-U opencl` |
|------|-----|-----------|-------------|
| `address` / `rmd160` | Full | GPU EC + host filter (stable) | Host EC + GPU hash160 |
| `vanity` / `bsgs` / `xpoint` | Full | CPU | CPU |
| `mnemonic` / `poetry` / `brainwallet` / `minikeys` / `pubkey2addr` | Full | CPU | CPU |
| `address` + `-c sol` | Full | CPU | CPU |

- Device CUDA **hash160** is not trusted yet → production uses **host** hash/bloom.
- CUDA does **not** use `-e`. For endomorphism coverage use CPU `-e`.
- On many CPUs, **AVX2/512 + `-e`** beats the current CUDA batch=1 path.

---

## Quick examples

```bash
# ETH
./keyhunt -m address -c eth -f eth.txt -t 8 -q -s 10

# Vanity
./keyhunt -m vanity -v 1Cool -l compress -R -b 256 -e -t 8

# OpenCL hash offload
./keyhunt -m address -f targets.txt -U opencl -t 8 -l compress

# CUDA EC (NVIDIA build)
./keyhunt -m address -f targets.txt -U cuda -t 1 -l compress -s 5

# Spiral pattern in a bit range
./keyhunt -m address -f targets.txt -b 66 -l compress -x spiral -e -t 8 -A auto
```

Windows Desktop binaries (when you build locally):

```text
keyhunt.exe          # MinGW CPU
keyhunt_cuda.exe     # MSVC + CUDA
```

---

## Build options

| Platform | How |
|----------|-----|
| Linux/macOS CPU | `make -j$(nproc)` or CMake |
| Windows CPU | `build_mingw_native.bat` / MSYS2 |
| Windows CUDA | `build_cuda_vs2022.bat` (VS 2022 + CUDA 12.x) |
| OpenCL | `cmake -B build -DENABLE_OPENCL=ON && cmake --build build -j` |

Details: **[docs/BUILD.md](docs/BUILD.md)** · GPU notes: **[gpu/README.md](gpu/README.md)**.

---

## Performance tips

| Do | Don’t |
|----|--------|
| `-t` ≈ physical cores | Expect pagefile to speed AVX |
| `-e` on CPU secp address/rmd160/vanity | Use `-e` with Solana / CUDA EC |
| `-A auto` / `avx2` / `avx512` | Assume GPU always wins |
| Size **RAM** for blooms/BSGS | Confuse “selected AVX” with “AVX hash kernel” (AVX1 → SSE hash) |

---

## Documentation map

| Doc | Audience |
|-----|----------|
| [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md) | First-time users |
| [docs/COMMANDS.md](docs/COMMANDS.md) | Copy-paste recipes |
| [docs/ROADMAP.md](docs/ROADMAP.md) | Plan & future features (Kangaroo, SOL GPU vanity, …) |
| [docs/BUILD.md](docs/BUILD.md) | Compilers & CMake |
| [docs/BENCHMARK.md](docs/BENCHMARK.md) | Measuring speed |
| [gpu/README.md](gpu/README.md) | CUDA / OpenCL |

---

## Disclaimer

This software is for **education, puzzles, and research** on keys/ranges you are authorized to search. You are responsible for how you use it.

---

## License / credits

See repository license. Upstream ideas: Alberto Keyhunt, Jean Luc Pons (VanitySearch / BSGS / Kangaroo lineage), and the wider secp256k1 tooling ecosystem.
