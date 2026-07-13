# TrueCollider based of Keyhunt By Alberto

[![CI](https://github.com/TrueSc3nt/TrueCollider/actions/workflows/ci.yml/badge.svg)](https://github.com/TrueSc3nt/TrueCollider/actions/workflows/ci.yml)
[![Release](https://github.com/TrueSc3nt/TrueCollider/actions/workflows/release.yml/badge.svg)](https://github.com/TrueSc3nt/TrueCollider/releases)

**Multi-Currency Cryptocurrency Key Search Tool** — High-performance key search with Binary Fuse Filters, supporting 10+ cryptocurrencies, custom derivation paths, node balance checking, and multi-currency simultaneous search.

Developed & Modified by **TrueScent**

---

## Table of Contents

- [Features](#features)
- [Download and Build](#download-and-build)
- [Quick Start](#quick-start)
- [Search Modes](#search-modes)
  - [Address Mode](#address-mode)
  - [RMD160 Mode](#rmd160-mode)
  - [Vanity Mode](#vanity-mode)
  - [BSGS Mode](#bsgs-mode)
  - [Mnemonic Mode](#mnemonic-mode)
  - [Poetry Mode](#poetry-mode)
  - [Brainwallet Mode](#brainwallet-mode)
  - [Minikeys Mode](#minikeys-mode)
  - [XPoint Mode](#xpoint-mode)
  - [Pubkey2Addr Mode](#pubkey2addr-mode)
- [Supported Cryptocurrencies](#supported-cryptocurrencies)
- [Address Formats](#address-formats)
- [Derivation Paths](#derivation-paths)
- [Node Balance Checking](#node-balance-checking)
- [Search Patterns](#search-patterns)
- [Performance Options](#performance-options)
- [Output Options](#output-options)
- [File Formats](#file-formats)
- [Examples](#examples)

---

## Features

- **10+ Search Modes** — address, pubkey2addr, rmd160, xpoint, bsgs, vanity, minikeys, mnemonic, poetry, brainwallet
- **11 Cryptocurrencies** — BTC, ETH, LTC, DOGE, XRP, SOL, BCH, BTG, ETC, Taproot, ALL
- **Multi-Currency Simultaneous** — search all currencies at once with `-c all`
- **Auto-Detect Currency** — use `-c auto` to detect address types from file content
- **All BTC Address Formats** — Legacy (1...), P2SH (3...), SegWit (bc1q...), Taproot (bc1p...)
- **Binary Fuse Filters** — 30-40% faster lookups, 30-40% less memory than traditional bloom filters
- **7 Search Patterns** — sequential, random, chaos, gravity, spiral, reverse, auto
- **Multi-threaded** — full multi-thread support across all modes
- **BSGS Algorithm** — Baby-Step Giant-Step for solving crypto puzzles
- **BIP-39 Mnemonics** — 10 languages, BIP-44/49/84 derivation paths
- **Custom Derivation Paths** — use `-p` for BIP-32 derivation in address/rmd160 modes
- **Node Balance Check** — check balances via API or own Bitcoin Core node
- **Endomorphism** — GLV endomorphism for 3x speedup in address/rmd160/vanity modes
- **Timestamp Search** — narrow search ranges by creation time
- **Verbose Output** — show full derivation path and chain code with `-V`

---

## Download and Build

### Requirements

- Linux, macOS, or Windows (WSL recommended)
- `git`
- `build-essential` (g++, make)
- `curl` (optional, for node balance checking)

### Install Dependencies (Debian/Ubuntu)

```bash
sudo apt update && sudo apt upgrade
sudo apt install git build-essential curl -y
```

### Clone and Build (Makefile)

```bash
git clone https://github.com/TrueSc3nt/TrueCollider.git
cd TrueCollider
make
```

### Build with CMake (recommended)

CMake gives a unified build on Linux, Windows (via MinGW), and Termux/Android.

```bash
cmake -B build
cmake --build build -j$(nproc)
# Binary: build/keyhunt
```

### Windows Users

Use WSL with Ubuntu:
```bash
wsl --install -d Ubuntu
# Then open Ubuntu terminal and run the build commands above
# Or, to build a standalone Windows .exe directly:
bash build_windows.sh
```

Or build the `.exe` with CMake and MinGW:
```bash
sudo apt install mingw-w64 cmake
cmake -B build-win \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x86_64.cmake \
  -DSTATIC_BUILD=ON
cmake --build build-win -j$(nproc)
# Binary: build-win/keyhunt.exe
```

### Termux (Android / aarch64)

```bash
pkg install cmake make clang
cmake -B build \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/termux-aarch64.cmake
cmake --build build -j$(nproc)
# Binary: build/keyhunt
```

---

## Quick Start

```bash
# Run help
./keyhunt -h

# Search BTC addresses with 8 threads
./keyhunt -m address -f targets.txt -t 8

# Search ETH addresses
./keyhunt -m address -c eth -f eth_targets.txt -t 8

# Search ALL currencies simultaneously
./keyhunt -m address -c all -f all_targets.txt -t 8

# Auto-detect currencies from file
./keyhunt -m address -c auto -f mixed_targets.txt -t 8

# Solve puzzle 66 (address mode)
./keyhunt -m address -f tests/66.rmd -b 66 -l compress -t 8

# Solve puzzle 125 (BSGS mode)
./keyhunt -m bsgs -f tests/125.txt -b 125 -q -s 10 -R
```

---

## Search Modes (`-m`)

### Address Mode

Search for private keys that produce addresses in the target file.

**Supported Cryptocurrencies:**
| Currency | Flag | Address Format |
|----------|------|----------------|
| Bitcoin | btc | 1..., 3..., bc1q... |
| Ethereum | eth | 0x... |
| Litecoin | ltc | L... |
| Dogecoin | doge | D... |
| XRP | xrp | r... |
| Bitcoin Cash | bch | q..., p... |
| Bitcoin Gold | btg | G... |
| Ethereum Classic | etc | 0x... |
| Taproot | troot | bc1p... |
| All Currencies | all | All of the above |
| Auto-Detect | auto | Detects from file |

**Input file format:** One address per line (format depends on currency)

**How it works:**
1. Load target addresses into binary fuse filter + sorted array
2. Each thread generates candidate private keys
3. Derives public key → address
4. Checks address against filter (fast rejection) then binary search
5. Found key is written to `KEYFOUNDKEYFOUND.txt`

**Examples:**
```bash
# Search BTC addresses (legacy, P2SH, or bech32)
./keyhunt -m address -f targets.txt -t 8

# Search ETH addresses
./keyhunt -m address -c eth -f eth_targets.txt -t 8

# Search Litecoin addresses
./keyhunt -m address -c ltc -f ltc_targets.txt -t 8

# Search Dogecoin addresses
./keyhunt -m address -c doge -f doge_targets.txt -t 8

# Search XRP addresses
./keyhunt -m address -c xrp -f xrp_targets.txt -t 8

# Search Taproot addresses
./keyhunt -m address -c troot -f troot_targets.txt -t 8

# Search ALL currencies simultaneously
./keyhunt -m address -c all -f all_targets.txt -t 8

# Auto-detect currencies from mixed file
./keyhunt -m address -c auto -f mixed_targets.txt -t 8

# Search with specific range
./keyhunt -m address -f targets.txt -r 1:FFFFFFFF -t 8

# Search with bit range (puzzle 66)
./keyhunt -m address -f tests/66.rmd -b 66 -l compress -t 8

# Search with chaos pattern
./keyhunt -m address -f targets.txt -x chaos -t 8

# Search with gravity pattern
./keyhunt -m address -f targets.txt -x gravity -t 8

# Search with spiral pattern
./keyhunt -m address -f targets.txt -x spiral -t 8

# Search with auto pattern (cycles through all)
./keyhunt -m address -f targets.txt -x auto -t 8

# Search with endomorphism (3x speedup)
./keyhunt -m address -f targets.txt -e -t 8

# Quiet mode with stats every 10 seconds
./keyhunt -m address -f targets.txt -q -s 10 -t 8

# With timestamp range (Jan 1, 2021)
./keyhunt -m address -f targets.txt -T 1609459200 -t 8
```

---

### RMD160 Mode

Search for private keys matching target RIPEMD-160 hashes directly (faster than address mode for raw hash comparisons).

**Supported Cryptocurrencies:** btc, eth, ltc, doge, xrp, bch, btg, etc, all

**Input file format:** One 40-char hex RIPEMD-160 hash per line

**Examples:**
```bash
# Search RMD160 hashes
./keyhunt -m rmd160 -f tests/1to32.rmd -l compress -s 5

# Search with all currencies
./keyhunt -m rmd160 -c all -f hashes.rmd -t 8

# Search with custom derivation path
./keyhunt -m rmd160 -p "m/44'/0'/0'/0" -D 10 -f hashes.rmd -V -t 8

# Search with chaos pattern
./keyhunt -m rmd160 -f tests/1to32.rmd -x chaos -t 8

# Search with gravity pattern
./keyhunt -m rmd160 -f tests/1to32.rmd -x gravity -t 8
```

---

### Vanity Mode

Search for a specific vanity address prefix.

**Input file format:** Not applicable (use `-v` flag)

**Examples:**
```bash
# Single vanity prefix
./keyhunt -m vanity -v 1Cool -t 8

# Multiple vanity prefixes
./keyhunt -m vanity -v 1Cool -v 1MyKey -t 8

# With endomorphism (3x speedup)
./keyhunt -m vanity -v 1Cool -e -t 8

# With chaos pattern
./keyhunt -m vanity -v 1Cool -x chaos -t 8

# Quiet mode
./keyhunt -m vanity -v 1Cool -q -s 10 -t 8
```

---

### BSGS Mode

Baby-Step Giant-Step algorithm for solving crypto puzzles when a public key is known.

**Input file format:** Compressed (66-char) or uncompressed (130-char) public keys

**BSGS search modes (`-B`):**
| Mode | Description |
|------|-------------|
| sequential | Walk forward from start |
| backward | Walk backward from end |
| both | Alternate top/bottom of range |
| random | Random giant steps (default) |
| dance | Triple: top/bottom/random alternation |

**Examples:**
```bash
# BSGS sequential
./keyhunt -m bsgs -f pubkeys.txt -B sequential -n 0x1000000 -t 4

# BSGS backward
./keyhunt -m bsgs -f pubkeys.txt -B backward -n 0x1000000 -t 4

# BSGS random (default)
./keyhunt -m bsgs -f pubkeys.txt -B random -n 0x1000000 -t 4

# BSGS with auto search cycling
./keyhunt -m bsgs -f pubkeys.txt -x auto -S -t 8

# BSGS with bit range (puzzle 125)
./keyhunt -m bsgs -f pubkeys.txt -b 125 -t 4

# BSGS with higher K factor (more RAM, faster)
./keyhunt -m bsgs -f pubkeys.txt -k 128 -n 0x10000000000 -t 4
```

---

### Mnemonic Mode

Generate random BIP-39 mnemonics, derive keys via BIP-32/44/49/84.

**Input file format:** One BTC/ETH address per line

**Derivation paths checked:**
- BIP-44: `m/44'/0'/0'/0/N` — Legacy P2PKH (1...)
- BIP-49: `m/49'/0'/0'/0/N` — Wrapped SegWit (3...)
- BIP-84: `m/84'/0'/0'/0/N` — Native SegWit (bc1q...)

**Examples:**
```bash
# 12-word English mnemonics
./keyhunt -m mnemonic -w 12 -L english -f targets.txt -t 4

# 24-word English mnemonics
./keyhunt -m mnemonic -w 24 -L english -f targets.txt -t 4

# Random word count (12, 15, 18, 21, or 24)
./keyhunt -m mnemonic -w 0 -L english -f targets.txt -t 4

# Japanese mnemonics
./keyhunt -m mnemonic -w 12 -L japanese -f targets.txt -t 4

# All languages (cycles through all 10)
./keyhunt -m mnemonic -w 12 -L all -f targets.txt -t 4

# ETH addresses
./keyhunt -m mnemonic -w 12 -W -f eth_targets.txt -t 4

# Check 50 address indices per path (150 total per mnemonic)
./keyhunt -m mnemonic -D 50 -f targets.txt -t 8
```

---

### Poetry Mode

Generate random poetry words, encode as hex private keys.

**Input file format:** One BTC address per line

**Examples:**
```bash
# Basic poetry search
./keyhunt -m poetry -f targets.txt -t 4

# With chaos pattern
./keyhunt -m poetry -f targets.txt -x chaos -t 4

# Quiet mode
./keyhunt -m poetry -f targets.txt -q -s 10 -t 4
```

---

### Brainwallet Mode

SHA256 hash passphrases from wordlist to derive private keys.

**Input file format:** One BTC address per line

**Mutations applied:**
- SHA256 hash (standard brainwallet)
- Double SHA256 hash
- Number suffix (1, 123, 0, 2024, 69, 420)
- Leet speak (a→@, e→3, o→0, s→$)
- Capitalize first letter
- ALL CAPS
- Separators: space, dash, underscore, dot, none

**Examples:**
```bash
# Basic brainwallet search
./keyhunt -m brainwallet -f targets.txt -t 8

# 3-word phrases
./keyhunt -m brainwallet -w 3 -f targets.txt -t 8

# Random word count
./keyhunt -m brainwallet -w 0 -f targets.txt -t 8
```

---

### Minikeys Mode

Generate and test Bitcoin minikeys (short private keys starting with 'S').

**Input file format:** One BTC address per line

**Examples:**
```bash
# Basic minikeys search
./keyhunt -m minikeys -f targets.txt -t 4

# With custom base minikey
./keyhunt -m minikeys -C SRPqx8QiwnW4WNWnTVa2W5 -f targets.txt -t 4

# Random minikeys
./keyhunt -m minikeys -f targets.txt -n 0x10000 -q -R
```

---

### XPoint Mode

Search by matching the x-coordinate of public keys.

**Input file format:** One hex public key per line (64-char x-only, 66-char compressed, or 130-char uncompressed)

**Examples:**
```bash
# Search xpoint with range
./keyhunt -m xpoint -f pubkeys.txt -r 1:FFFFFFFF -t 8

# With chaos pattern
./keyhunt -m xpoint -f pubkeys.txt -x chaos -t 8
```

---

### Pubkey2Addr Mode

Random key generation with address matching. Defaults to `-x random`.

**Supported Cryptocurrencies:** btc, eth, ltc, doge, xrp, bch, btg, etc, troot, all

**Examples:**
```bash
# Random key generation, BTC addresses
./keyhunt -m pubkey2addr -f targets.txt -t 8

# Random key generation, ETH addresses
./keyhunt -m pubkey2addr -c eth -f eth_targets.txt -t 8

# Auto search pattern
./keyhunt -m pubkey2addr -f targets.txt -x auto -t 4
```

---

## Supported Cryptocurrencies

| Currency | Flag | Address Format | Curve |
|----------|------|----------------|-------|
| Bitcoin | btc | 1..., 3..., bc1q... | secp256k1 |
| Ethereum | eth | 0x... | secp256k1 |
| Litecoin | ltc | L... | secp256k1 |
| Dogecoin | doge | D... | secp256k1 |
| XRP | xrp | r... | secp256k1 |
| Solana | sol | base58 | ed25519 |
| Bitcoin Cash | bch | q..., p... | secp256k1 |
| Bitcoin Gold | btg | G... | secp256k1 |
| Ethereum Classic | etc | 0x... | secp256k1 |
| Taproot | troot | bc1p... | secp256k1 |
| All Currencies | all | All of the above | All |

---

## Address Formats

### Bitcoin (BTC)

| Format | Prefix | Example | BIP |
|--------|--------|---------|-----|
| Legacy | 1... | `1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH` | P2PKH |
| P2SH | 3... | `3J98t1WpEZ73CNmQviecrnyiWrnqRhWNLy` | P2SH-P2WPKH |
| SegWit | bc1q... | `bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4` | P2WPKH |
| Taproot | bc1p... | `bc1p...` | P2TR |

### Ethereum (ETH/ETC)

| Format | Prefix | Example |
|--------|--------|---------|
| Standard | 0x... | `0x742d35Cc6634C0532925a3b844Bc9e7595f2bD3e` |

### Other Cryptocurrencies

| Currency | Format | Example |
|----------|--------|---------|
| Litecoin | L... | `LQTpS3VaYTjCr4cz9h3RjVTnBfS4Vf9c3V` |
| Dogecoin | D... | `D...` |
| XRP | r... | `rP5lX6w7nQnXn7g4m5k3j2h1g0f9e8d7c` |
| Bitcoin Gold | G... | `G...` |

---

## Derivation Paths

Use `-p` to specify custom BIP-32 derivation paths in address/rmd160 modes.

**How it works:**
- Each random key is used as a BIP-32 master key
- Child keys are derived along the specified path for indices 0 to `-D-1`
- Supports hardened: `'` or `H` or `h` suffix (e.g. `44'`)

**Examples:**
```bash
# BIP-84 native segwit derivation (20 indices)
./keyhunt -m address -p "m/84'/0'/0'/0" -D 20 -f targets.txt -V -t 8

# BIP-86 taproot derivation (10 indices)
./keyhunt -m address -c troot -p "m/86'/0'/0'/0" -D 10 -f targets.txt -V -t 8

# BIP-44 legacy derivation (50 indices)
./keyhunt -m address -p "m/44'/0'/0'/0" -D 50 -f targets.txt -V -t 8

# Custom derivation path
./keyhunt -m address -p "m/44'/0'/0'" -D 10 -f targets.txt -V -t 8

# RMD160 with derivation path
./keyhunt -m rmd160 -p "m/44'/0'/0'/0" -D 10 -f hashes.rmd -V -t 8
```

---

## Node Balance Checking

Use `-N` to check balances when keys are found.

### Public APIs (no setup required)
```bash
# Check balance via public APIs
./keyhunt -m address -c btc -f targets.txt -N -t 8
```

### Own Bitcoin Core Node
```bash
# Connect to your own Bitcoin Core node
./keyhunt -m address -c btc -f targets.txt -N http://user:pass@127.0.0.1:8332 -t 8
```

### Bitcoin Core Configuration

Add to your `bitcoin.conf`:
```
server=1
rpcuser=myuser
rpcpassword=mypass
rpcport=8332
```

### Supported APIs

| Currency | Public API | Own Node |
|----------|------------|----------|
| BTC | blockstream.info | Bitcoin Core RPC |
| ETH | etherscan.io | — |
| LTC | blockcypher.com | — |
| ETC | etherscan.io | — |

---

## Search Patterns (`-x`)

| Pattern | Description |
|---------|-------------|
| `sequential` | Linear walk from start to end of range |
| `random` | Random key selection across full range |
| `chaos` | Logistic map chaotic sequence (r=3.99999) |
| `gravity` | Adaptive: clusters around previously found keys |
| `spiral` | Archimedean spiral from range midpoint |
| `reverse` | Inverted BSGS baby/giant step roles |
| `auto` | Cycling: spiral→chaos→gravity→reverse |

**Examples:**
```bash
# Sequential search
./keyhunt -m address -f targets.txt -x sequential -t 8

# Chaos search
./keyhunt -m address -f targets.txt -x chaos -t 8

# Gravity search
./keyhunt -m address -f targets.txt -x gravity -t 8

# Spiral search
./keyhunt -m address -f targets.txt -x spiral -t 8

# Auto search (cycles through all)
./keyhunt -m address -f targets.txt -x auto -t 8

# Works with all modes
./keyhunt -m bsgs -f pubkeys.txt -x chaos -n 0x1000000 -t 4
./keyhunt -m mnemonic -w 12 -f targets.txt -x auto -t 4
```

---

## Performance Options

```bash
# Endomorphism (3x speedup for address/rmd160/vanity; auto-disabled in BSGS)
./keyhunt -m address -f targets.txt -e -t 8

# Compressed keys only
./keyhunt -m address -f targets.txt -l compress -t 8

# Uncompressed keys only
./keyhunt -m address -f targets.txt -l uncompress -t 8

# Both compressed and uncompressed
./keyhunt -m address -f targets.txt -l both -t 8

# Stride value
./keyhunt -m address -f targets.txt -I 2 -t 8

# BSGS K factor (more RAM, faster)
./keyhunt -m bsgs -f targets.txt -k 128 -t 4

# Bloom filter size multiplier
./keyhunt -m address -f targets.txt -z 2 -t 8

# Save/load BSGS bloom filters
./keyhunt -m bsgs -f targets.txt -S -t 4
```

### Batch Point Generation & `CPU_GRP_SIZE` Tuning

Address/rmd160/vanity mode generate public keys in batches of `CPU_GRP_SIZE` (default 1024). Increasing it can reduce per-key overhead on CPU-friendly ranges, but it also raises memory usage and thread contention. It is configurable at build time:

```bash
# Makefile: larger batch (e.g. 4096)
make CPU_GRP_SIZE=4096

# CMake: larger batch
cmake -B build -DCPU_GRP_SIZE=4096
cmake --build build -j$(nproc)
```

Suggested values:
| CPU / RAM | `CPU_GRP_SIZE` | Notes |
|-----------|------------------|-------|
| Low-end / 8 GB | 256 - 1024 | Lower memory, less contention |
| Mid-range / 16 GB | 1024 - 4096 | Default sweet spot |
| High-end / 32+ GB | 4096 - 16384 | Best throughput if L3/cache allows |

A future GPU backend will use a separate `GPU_BATCH_SIZE` knob and keep `CPU_GRP_SIZE` for CPU-only work.

### Binary Fuse Filter Verified Performance

The binary fuse filter is used in address mode for fast membership testing. Verification results show no false negatives and a measured false-positive rate very close to the theoretical 1/256 (~0.39%) for the 8-bit fingerprint version. See [docs/FILTER_VERIFICATION.md](docs/FILTER_VERIFICATION.md) for details.

### GPU Backend (Experimental)

A CUDA/OpenCL backend is planned for future high-end rigs. CMake options are already wired as scaffolding:

```bash
cmake -B build -DENABLE_CUDA=ON    # requires CUDA toolkit
cmake -B build -DENABLE_OPENCL=ON    # requires OpenCL headers/runtime
```

The current build only compiles device-detection stubs; the full EC math kernels will be added in a follow-up release.

---

## Output Options

```bash
# Quiet mode (suppress per-thread output)
./keyhunt -m address -f targets.txt -q -t 8

# Stats every 10 seconds
./keyhunt -m address -f targets.txt -s 10 -t 8

# Stats every 5 seconds
./keyhunt -m address -f targets.txt -s 5 -t 8

# Turn off stats
./keyhunt -m address -f targets.txt -s 0 -t 8

# Verbose derivation path output
./keyhunt -m address -p "m/84'/0'/0'/0" -D 10 -V -f targets.txt -t 8

# Matrix screen effect
./keyhunt -m address -f targets.txt -M -t 8

# Skip checksum on cached data files
./keyhunt -m address -f targets.txt -6 -t 8

# Debug mode
./keyhunt -m address -f targets.txt -d -t 8
```

---

## Range Options

```bash
# Specific hex range
./keyhunt -m address -f targets.txt -r 1:FFFFFFFF -t 8

# Open-ended range (from start to curve order)
./keyhunt -m address -f targets.txt -r 1:FFFF -t 8

# Bit range (puzzle 66)
./keyhunt -m address -f targets.txt -b 66 -t 8

# Bit range (puzzle 125)
./keyhunt -m bsgs -f targets.txt -b 125 -t 8

# Timestamp range (search around Jan 1 2021)
./keyhunt -m address -f targets.txt -T 1609459200 -t 8

# Timestamp + bit range for tighter window
./keyhunt -m address -f targets.txt -T 1609459200 -b 32 -t 8
```

---

## File Formats

### Address Targets
One address per line. The tool auto-detects format:
```
1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
3J98t1WpEZ73CNmQviecrnyiWrnqRhWNLy
bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4
0x742d35Cc6634C0532925a3b844Bc9e7595f2bD3e
LQTpS3VaYTjCr4cz9h3RjVTnBfS4Vf9c3V
```

### RMD160 Targets
One 40-char hex RIPEMD-160 hash per line:
```
751e76e8199196d454941c45d1b3a323f1433bd6
7dd65592d0ab2fe0d0257d571abf032cd9db93dc
```

### Public Key Targets
One hex public key per line (64/66/130 chars):
```
0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
043ffa1cc011a8d23dec502c7656fb3f93dbe4c61f91fd443ba444b4ec2dd8e6f...
```

### Taproot Targets
One 64-char hex x-only taproot output key per line:
```
a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2c3d4e5f6a1b2
```

---

## All Command-Line Options

### Required
| Flag | Description |
|------|-------------|
| `-m mode` | Search mode: address, pubkey2addr, rmd160, xpoint, bsgs, vanity, minikeys, mnemonic, poetry, brainwallet |
| `-f file` | Input file with target data |

### Cryptocurrency (`-c`)
| Flag | Description |
|------|-------------|
| `-c btc` | Bitcoin (default) |
| `-c eth` | Ethereum |
| `-c ltc` | Litecoin |
| `-c doge` | Dogecoin |
| `-c xrp` | XRP (Ripple) |
| `-c sol` | Solana |
| `-c bch` | Bitcoin Cash |
| `-c btg` | Bitcoin Gold |
| `-c etc` | Ethereum Classic |
| `-c troot` | Taproot (P2TR) |
| `-c all` | All currencies simultaneously |
| `-c auto` | Auto-detect from file content |

### Range
| Flag | Description |
|------|-------------|
| `-r SR:EN` | Hex range StartRange:EndRange |
| `-b bits` | Bit range - only test keys with this many bits |
| `-T timestamp` | Unix timestamp - search around creation time |
| `-Z bytes` | Strip N leading zero bytes from key |
| `-n number` | Sequential keys per cycle / BSGS table size |
| `-I stride` | Stride value for address/rmd160/xpoint modes |

### Derivation Paths
| Flag | Description |
|------|-------------|
| `-p path` | BIP-32 derivation path (e.g. `m/44'/0'/0'/0`) |
| `-D count` | Child key indices per base key (1-100) |
| `-V` | Verbose output - show full derivation path and chain code |

### BSGS Options
| Flag | Description |
|------|-------------|
| `-B mode` | BSGS strategy: sequential, backward, both, random, dance |
| `-k value` | K factor multiplies M (baby-step count) |
| `-S` | Save/load BSGS bloom filters to disk |
| `-n value` | Baby-step table size |

### Mnemonic Options
| Flag | Description |
|------|-------------|
| `-w count` | Word count: 0=random, 12, 15, 18, 21, 24 |
| `-W` | Check ETH addresses (keccak256) |
| `-L lang` | BIP39 language: english, spanish, french, italian, czech, portuguese, japanese, korean, chinese_simplified, chinese_traditional, all |
| `-D count` | Address indices per derivation path |

### Vanity Options
| Flag | Description |
|------|-------------|
| `-v value` | Target vanity prefix (e.g. "1Cool", "bc1qabc") |

### Minikey Options
| Flag | Description |
|------|-------------|
| `-C string` | Base minikey string (22 char). Starts with S |
| `-8 alpha` | Custom base58 alphabet (58 chars) |

### Address Filter
| Flag | Description |
|------|-------------|
| `-l look` | compress, uncompress, both (default: both) |

### Node Balance Check
| Flag | Description |
|------|-------------|
| `-N[url]` | Check balance when key found (requires curl) |
| | Without URL: public APIs (blockstream.info, etherscan) |
| | With URL: own Bitcoin Core node via RPC |
| | Format: `http://user:pass@host:port` |

### Performance
| Flag | Description |
|------|-------------|
| `-e` | Enable GLV endomorphism (3x speedup) |
| `-k value` | BSGS K factor (more RAM, faster) |
| `-z value` | Bloom filter size multiplier |
| `-e` | Enable endomorphism |

### Output
| Flag | Description |
|------|-------------|
| `-s seconds` | Stats output interval (0=off, default: 30) |
| `-q` | Quiet mode - suppress per-thread output |
| `-V` | Verbose mode - show full derivation path |
| `-M` | Matrix screen effect |
| `-d` | Debug mode |
| `-6` | Skip checksum on cached data files |

### Threading
| Flag | Description |
|------|-------------|
| `-t threads` | Number of threads (default: 1) |

### Search Pattern
| Flag | Description |
|------|-------------|
| `-x mode` | sequential, random, chaos, gravity, spiral, reverse, auto |

### Misc
| Flag | Description |
|------|-------------|
| `-R` | Random mode (alias for BSGS random) |
| `-S` | Save/load bloom filters |
| `-G value` | CPU group/batch size (compile-time via `CPU_GRP_SIZE` CMake/Make option) |

---

## Performance Benchmarks

| Addresses | Memory | Filter Size | Speed (8 threads) |
|-----------|--------|-------------|-------------------|
| 1 | 0.00 MB | 0.00 MB | ~2 Mkeys/s |
| 100 | 0.00 MB | 0.00 MB | ~3 Mkeys/s |
| 500 | 0.01 MB | 0.00 MB | ~3.5 Mkeys/s |
| 1,000 | 0.02 MB | 0.00 MB | ~4 Mkeys/s |
| 10,000 | 0.19 MB | 0.01 MB | ~4.2 Mkeys/s |

---

## License

See [LICENSE](LICENSE) for details.

---

## Donations

- BTC: 1HmztBLDnwwaKAGbtALsYvCNBuoJYEic3h
- 
**TrueCollider** — Modified by TrueScent
