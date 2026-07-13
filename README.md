# TrueCollider

[![CI](https://github.com/TrueSc3nt/TrueCollider/actions/workflows/ci.yml/badge.svg)](https://github.com/TrueSc3nt/TrueCollider/actions/workflows/ci.yml)
[![Release](https://github.com/TrueSc3nt/TrueCollider/actions/workflows/release.yml/badge.svg)](https://github.com/TrueSc3nt/TrueCollider/releases)

<p align="center">
  <b>TrueCollider</b> — also known as <b>KeyCollider</b> — is a high-performance multi-currency<br/>
  private-key &amp; seed hunter for puzzles, vanity, pubkey discrete-log, and research.
</p>

<p align="center">
  <i>One tool. Many curves. CPU SIMD + optional NVIDIA CUDA / OpenCL.</i><br/>
  Bitcoin · Ethereum · Litecoin · Dogecoin · XRP · BCH · BTG · Taproot · Solana · and more.
</p>

Based on [Keyhunt](https://github.com/albertobsd/keyhunt) by Alberto · Developed & modified by **TrueScent**

> **New here?** → [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md)  
> **Every recipe?** → [docs/COMMANDS.md](docs/COMMANDS.md)  
> **GPU details?** → [gpu/README.md](gpu/README.md)  
> **What’s next?** → [docs/ROADMAP.md](docs/ROADMAP.md)

---

## What is TrueCollider / KeyCollider?

TrueCollider is a **key-collision and range-search engine**: you give it target addresses (or pubkeys / hashes), a bit-range or hex range, and it walks private keys as fast as your hardware allows — writing hits to `KEYFOUNDKEYFOUND.txt`.

It is built for:

- **Bitcoin puzzle challenges** (known bit ranges)
- **Vanity** prefixes (`1Cool…`)
- **Multi-coin address lists** (BTC-family hash160, ETH keccak, Taproot x-only, Solana ed25519)
- **BSGS** when you already have a **public key** (discrete log in a bounded range)
- **Research / education** on secp256k1 and ed25519 search patterns

It is **not** a magic “find any wallet” product. Full 256-bit space search is not practical. Stay on puzzles and ranges you are authorized to search.

---

## Features

| Area | What you get |
|------|----------------|
| **Modes** | `address` · `rmd160` · `xpoint` · `bsgs` · `vanity` · `minikeys` · `mnemonic` · `poetry` · `brainwallet` · `pubkey2addr` |
| **Coins (`-c`)** | `btc` `eth` `etc` `ltc` `doge` `xrp` `bch` `btg` `troot` `sol` `all` `auto` |
| **CPU speed** | Threads · GLV endomorphism (`-e`) · SSE 4-wide · **AVX2 8-wide** · **AVX-512 16-wide** (`-A auto`) |
| **CUDA (`-U cuda`)** | NVIDIA **GPU secp256k1 EC** for address / rmd160 / eth / etc / taproot; host hash + bloom (stable) |
| **OpenCL (`-U opencl`)** | NVIDIA / AMD / Intel **hash160** offload; EC on CPU |
| **Filters** | Binary fuse → bloom · BSGS multi-level blooms + sorted baby table |
| **Patterns (`-x`)** | `sequential` `random` `chaos` `gravity` `spiral` `reverse` `auto` |
| **Other** | BIP32 paths · BIP39 languages · node mode · quiet stats |

---

## Measured speeds (real hardware bench)

Benchmarks run on this developer machine and pasted from live `keyhunt` / `keyhunt_cuda` stats (`-s 5`, 15 s window unless noted). **Your numbers will differ** with CPU generation, AVX level, GPU, threads, and target-file size.

**Hardware used for these numbers**

| | |
|--|--|
| CPU | Intel Core i7-920 @ 2.67 GHz (8 threads, **SSSE3 / SSE hash160** — no AVX2/512 on this chip) |
| GPU | NVIDIA GeForce **RTX 3060 Ti** 8 GB |
| OS / builds | Windows · `keyhunt.exe` (MinGW) · `keyhunt_cuda.exe` (VS2022 + CUDA 12.8) |

Re-run locally: `powershell -ExecutionPolicy Bypass -File .\run_benchmarks.ps1` · details: [docs/SPEEDS.md](docs/SPEEDS.md)

### CPU — every search mode

| Mode | Flags (summary) | Sustained rate (15 s) | Unit |
|------|-----------------|----------------------:|------|
| `address` BTC | `-t 8 -l compress -A sse` | **6.56 M** | keys/s |
| `address` BTC + `-e` | `-t 8 -e -l compress` | **8.16 M** | keys/s |
| `address` ETH | `-c eth -t 8` | **3.24 M** | keys/s |
| `address` SOL | `-c sol -t 8` | **70.7 K** | keys/s |
| `rmd160` | `-t 8 -l compress` | **7.33 M** | keys/s |
| `rmd160` + `-e` | `-t 8 -e` | **8.10 M** | keys/s |
| `xpoint` | `-t 8` | **11.3 M** | keys/s |
| `vanity` | `-v 1Love -t 8 -e` | **8.07 M** | keys/s |
| `pubkey2addr` | `-t 8` | **~489 M*** | keys/s* |
| `minikeys` | `-t 8` | **48.5 K** | keys/s |
| `mnemonic` (BIP39) | `-t 8` | **247 K** | mnemonics/s |
| `poetry` | `-t 4` | **57.9 M** | mnemonics/s |
| `brainwallet` | `-t 4` | **94.6 M** | mnemonics/s |
| `bsgs` | `-b 40 -n 1048576 -t 8` | **~26.4 G** | keys/s† |

\* `pubkey2addr` rate is what the binary prints; treat as relative, not apples-to-apples with `address`.  
† BSGS “keys/s” is the usual effective giant-step coverage rate (table-assisted), not raw EC ops.

### CUDA (RTX 3060 Ti) — modes that actually use the GPU

| Mode | Flags | Peak (5 s) | Sustained (15 s) | Notes |
|------|-------|----------:|-----------------:|-------|
| `address` BTC | `-U cuda -G 128 -t 1` | **154 K** keys/s | **110 K** keys/s | GPU EC + **host** hash160/bloom |
| `rmd160` | `-U cuda -G 128 -t 1` | **165 K** keys/s | **118 K** keys/s | Same path |
| `address` ETH | `-U cuda -G 128 -t 1` | **67 K** keys/s | **50 K** keys/s | GPU EC + host keccak |

### CUDA — not implemented (stay on CPU)

| Mode | GPU status | Use instead |
|------|------------|-------------|
| `mnemonic` / BIP39 | **No GPU path** | CPU: ~**247 K** mnemonics/s (`-t 8`) |
| `poetry` / `brainwallet` | **No GPU path** | CPU rates in table above |
| `vanity` | **No GPU path** | CPU: ~**8.1 M** keys/s with `-e` |
| `bsgs` | **No GPU path** | CPU: ~**26 G** keys/s (table size dependent) |
| `address -c sol` | **No GPU path** | CPU: ~**71 K** keys/s |
| `xpoint` / `minikeys` / `pubkey2addr` | **No GPU path** | CPU table above |

On this **SSE-only** CPU, AVX2/512 machines will often be much faster on CPU hash-heavy modes. CUDA here is still valuable for offloading EC; host hash is the bottleneck until device hash160 is production-ready. Prefer **CPU `-e -A auto`** when you have AVX2+.

---

## Quick start

### Build — Linux / macOS

```bash
git clone https://github.com/TrueSc3nt/TrueCollider.git
cd TrueCollider
make -j$(nproc)
./keyhunt -h
```

### Build — Windows CPU (MSYS2 MinGW)

```bat
build_mingw_native.bat
REM → keyhunt.exe
keyhunt.exe -h
```

### Build — Windows CUDA (VS 2022 + CUDA 12.x)

```bat
build_cuda_vs2022.bat
REM → keyhunt_cuda.exe
keyhunt_cuda.exe -h
```

### Build — OpenCL (CMake)

```bash
cmake -B build-opencl -DENABLE_OPENCL=ON
cmake --build build-opencl -j
```

More compilers & flags: **[docs/BUILD.md](docs/BUILD.md)**.

---

## Which mode should I use?

```text
Only have an ADDRESS?              →  -m address   (-c btc/eth/sol/troot/…)
Have RMD160 hex (40 chars)?        →  -m rmd160
Have a PUBLIC KEY (puzzle DL)?     →  -m bsgs
Want a custom prefix (1Cool…)?     →  -m vanity
BIP39 / wordlist / brainwallet?    →  -m mnemonic / poetry / brainwallet
```

Hits always append to **`KEYFOUNDKEYFOUND.txt`**.

---

## CPU examples

### Bitcoin puzzle-style (compressed + endomorphism + auto SIMD)

```bash
./keyhunt -m address -f tests/66.txt -b 66 -l compress -R -q -s 10 -t 8 -e -A auto
```

### Custom hex range

```bash
./keyhunt -m address -f targets.txt -r 100000:1fffff -l compress -t 8 -e
```

### Ethereum

```bash
./keyhunt -m address -c eth -f eth.txt -b 40 -t 8 -q -s 10
```

### Solana (ed25519 seed → base58)

```bash
./keyhunt -m address -c sol -f tests/sol_sample.txt -r 1:8 -t 1
./keyhunt -m address -c sol -f sol.txt -t 8 -q -s 10
```

Known sample hit: seed `0x1` → `6ASf5EcmmEHTgDJ4X4ZT5vT6iHVJBXPg5AN5YoTCpGWt`

### Taproot (`bc1p` / x-only)

```bash
./keyhunt -m address -c troot -f troot.txt -t 8
```

### Vanity

```bash
./keyhunt -m vanity -v 1Cool -l compress -R -b 256 -e -t 8 -A auto
```

### BSGS (you have the **pubkey**, not only the address)

```bash
./keyhunt -m bsgs -f tests/125.txt -b 125 -q -s 10 -R -t 8
```

Use `-k` to grow baby tables (more RAM → faster giant steps). Submodes: `-B sequential|random|backward|both|dance`.

### Spiral / chaos patterns

```bash
./keyhunt -m address -f targets.txt -b 66 -l compress -x spiral -e -t 8 -A auto
```

Windows helpers: `run_puzzle66_example.bat`, `run_sol_sample.bat`, `run_gpu_cuda_example.bat`.

---

## GPU examples (CUDA & OpenCL)

### Honest GPU matrix

| Mode / coin | CPU | `-U cuda` (NVIDIA) | `-U opencl` |
|-------------|---------|--------------------|-------------|
| `address` / `rmd160` — BTC, LTC, DOGE, XRP, BCH, BTG, `all` | Full | **GPU EC** + host hash160/bloom | Host EC + **GPU hash160** |
| `address` — ETH / ETC | Full | **GPU EC** + host keccak/bloom | CPU |
| `address` — Taproot (`troot`) | Full | **GPU EC** + host taproot tweak | CPU |
| `address` — Solana (`sol`) | Full | CPU (ed25519 GPU planned) | CPU |
| `vanity` / `bsgs` / `xpoint` / mnemonic / … | Full | CPU (BSGS stays on group-EC CPU) | CPU |

Notes:

- Device CUDA **hash160** is not trusted for production yet → address search hashes on the **host** after GPU EC.
- CUDA does **not** use `-e`. For endomorphism coverage use CPU `-e`.
- On strong CPUs, **AVX2/512 + `-e`** can beat a small GPU batch; tune with `-G` (default **128**, max **256**).
- Prefer **`-t 1`** (or few threads) with `-U cuda` so threads do not fight over the GPU lock.

### CUDA — Bitcoin address hunt

```bash
# Linux / unified binary built with ENABLE_CUDA
./keyhunt -m address -f targets.txt -l compress -U cuda -G 128 -t 1 -q -s 5

# Windows CUDA build
keyhunt_cuda.exe -m address -f targets.txt -l compress -U cuda -G 128 -t 1 -q -s 5
```

### CUDA — Ethereum

```bash
./keyhunt -m address -c eth -f eth.txt -U cuda -G 128 -t 1 -q -s 5
```

### CUDA — Taproot

```bash
./keyhunt -m address -c troot -f troot.txt -U cuda -G 128 -t 1 -q -s 5
```

### OpenCL — hash160 offload (AMD / NVIDIA / Intel)

```bash
./keyhunt -m address -f targets.txt -l compress -U opencl -t 8 -q -s 5
```

### GPU flags

| Flag | Meaning |
|------|---------|
| `-U cuda` | NVIDIA secp EC path |
| `-U opencl` | OpenCL hash160 |
| `-U none` | CPU only (default) |
| `-G N` | Batch size hint (`1..256` used on CUDA EC path; default `128`) |

Deep dive: **[gpu/README.md](gpu/README.md)**.

---

## Currencies (`-c`) in detail

| Flag | Curve / encode | Notes |
|------|----------------|-------|
| `btc` | secp256k1 · hash160 | Legacy / P2SH / bc1q (default) |
| `eth` / `etc` | secp256k1 · keccak256 | `0x` addresses |
| `ltc` `doge` `xrp` `bch` `btg` | secp256k1 · hash160 | Coin-specific Base58 / CashAddr on hit |
| `troot` | secp256k1 · taproot tweak | `bc1p` / 32-byte x-only targets |
| `sol` | **ed25519** | Seed → base58 pubkey; no `-e`, no BIP32 `-p`, no GPU yet |
| `all` | secp set | Multi secp coins in one run — do **not** mix Solana |
| `auto` | detect | From file contents |

---

## Performance tips

| Do | Don’t |
|----|--------|
| `-t` ≈ physical cores (CPU) | Expect pagefile to speed AVX |
| `-e` on CPU secp address/rmd160/vanity | Use `-e` with Solana or CUDA EC |
| `-A auto` / `avx2` / `avx512` | Assume GPU always wins |
| `-G 128`–`256` with CUDA | Slam `-t` high with CUDA (GPU lock) |
| Size **RAM** for BSGS blooms (`-k`) | Confuse “AVX selected” with “AVX hash” (AVX1 → SSE hash) |

---

## Documentation map

| Doc | Audience |
|-----|----------|
| [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md) | First-time install & first run |
| [docs/COMMANDS.md](docs/COMMANDS.md) | Copy-paste command cookbook |
| [docs/BUILD.md](docs/BUILD.md) | Compilers, CMake, CUDA, OpenCL |
| [docs/BENCHMARK.md](docs/BENCHMARK.md) | Measuring keys/s |
| [docs/SPEEDS.md](docs/SPEEDS.md) | **Real measured rates** (CPU + CUDA tables) |
| [docs/ROADMAP.md](docs/ROADMAP.md) | Kangaroo, GPU BSGS, SOL GPU, … |
| [gpu/README.md](gpu/README.md) | CUDA / OpenCL internals & status |

---

## Disclaimer

This software is for **education, puzzles, and research** on keys and ranges you are authorized to search. You are responsible for how you use it.

---

## License / credits

See repository license. Upstream ideas: Alberto Keyhunt, Jean Luc Pons (VanitySearch / BSGS / Kangaroo lineage), and the wider secp256k1 tooling ecosystem.
