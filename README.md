# TrueCollider / KeyCollider

[![CI](https://github.com/TrueSc3nt/TrueCollider/actions/workflows/ci.yml/badge.svg)](https://github.com/TrueSc3nt/TrueCollider/actions/workflows/ci.yml)
[![Release](https://github.com/TrueSc3nt/TrueCollider/actions/workflows/release.yml/badge.svg)](https://github.com/TrueSc3nt/TrueCollider/releases)

<p align="center">
  <b>TrueCollider</b> (also <b>KeyCollider</b>) — high-performance multi-currency<br/>
  private-key, seed, vanity, and pubkey discrete-log hunter.
</p>

<p align="center">
  <i>CPU SIMD · optional NVIDIA CUDA / OpenCL · puzzles · research · education</i>
</p>

Based on [Keyhunt](https://github.com/albertobsd/keyhunt) by Alberto · Developed & modified by **TrueScent**  
Repo: **[github.com/TrueSc3nt/TrueCollider](https://github.com/TrueSc3nt/TrueCollider)**

> [Getting started](docs/GETTING_STARTED.md) · [Command cookbook](docs/COMMANDS.md) · [Speeds](docs/SPEEDS.md) · [GPU](gpu/README.md) · [Roadmap](docs/ROADMAP.md)

---

## What it is

TrueCollider walks private keys (or seeds / mnemonics) as fast as your hardware allows and checks them against target addresses, hash160s, x-coordinates, or pubkeys. Hits append to **`FOUND_BTC.txt` / `FOUND_ETH.txt` / `FOUND_SOL.txt`** and legacy **`KEYFOUNDKEYFOUND.txt`**.

Built for Bitcoin puzzle ranges, vanity prefixes, multi-coin lists, BSGS/kangaroo when you have a public key, and research on secp256k1 / ed25519. It is **not** a “find any wallet” tool — full 256-bit search is not practical. Only search keys/ranges you are authorized to.

---

## Supported coins (`-c`)

| Flag | Curve / encode | Notes |
|------|----------------|-------|
| `btc` (default) | secp256k1 · hash160 | `1…` `3…` `bc1q…` |
| `eth` / `etc` | secp256k1 · keccak256 | `0x…` |
| `ltc` `doge` `xrp` `bch` `btg` | secp256k1 · hash160 | Coin-specific Base58 / CashAddr on hit |
| `troot` | secp256k1 · taproot tweak | `bc1p…` / 32-byte x-only |
| `sol` | **ed25519** | Seed → base58 pubkey; **CUDA:** GPU SHA512 + host ge (`-U cuda`) |
| `all` | multi secp | Do not mix Solana |
| `auto` | detect from file | |

---

## Supported modes (`-m`) & file formats

| Mode | Target file | Purpose |
|------|-------------|---------|
| `address` | addresses (one per line) | Default key → address search |
| `rmd160` | 40-char hex hash160 | Raw RIPEMD-160 match |
| `xpoint` | pubkeys / x-only hex | Match public-key X |
| `bsgs` | compressed/uncompressed pubkeys | Baby-step giant-step DL in a range |
| `kangaroo` | one pubkey | Pollard's kangaroo (CPU; tiny ranges sequential) |
| `vanity` | via `-v prefix` | Address prefix match |
| `minikeys` | address list | Bitcoin minikey (`S…`) search |
| `mnemonic` | address list | BIP39 → BIP32 → address |
| `poetry` / `brainwallet` | address list | Wordlist / passphrase → key |
| `pubkey2addr` | address list | Random key → address (defaults `-x random`) |
| `pub2rmd` | — | **Removed** — use `-m rmd160` |

**Fixtures:** `tests/66.txt` (puzzle 66 address), `tests/125.txt` (puzzle 125 pubkey), `tests/sol_sample.txt`, `tests/66.rmd`, `tests/1to32.*`, `tests/poetry.txt`, `tests/brainwalletwords.txt`.

---

## Status checklist (matches current code)

| Item | Status |
|------|--------|
| All modes above on **CPU** | **Done** |
| BSGS CPU + `-n`/`-k` validation, power-of-2 warn, `-k auto` / RAM guide | **Done** |
| Kangaroo CPU | **Done** |
| Dry-run `-y`, `FOUND_*.txt`, fuse hang guards | **Done** |
| `-M MB\|auto` memory budget (CUDA VRAM + BSGS blooms) | **Done** |
| CUDA GPU EC: address, rmd160, ETH/troot, vanity, xpoint, pubkey2addr, minikeys | **Done** (host hash/filter) |
| CUDA GPU EC after derive: mnemonic / poetry / brainwallet | **Done** (PBKDF2/words stay CPU) |
| CUDA BSGS baby-table + giant-step `ComputePublicKey` | **Done** (GRP giant loop still CPU) |
| CUDA Solana (`-c sol`) | **Done** (GPU SHA512 + host ed25519 ge) |
| OpenCL hash160 | **Done** (build with `ENABLE_OPENCL`; not default CUDA exe) |
| Device CUDA hash160 bloom (full on-GPU filter) | **Not shipped** (self-test fails; host filter used) |
| Full device Edwards ge / full GPU GRP BSGS | **Not shipped** |

---

# 1. CPU

Prefer CPU when you have AVX2/AVX-512, want `-e` endomorphism, Solana, or heavy BSGS tables in host RAM.

### Common flags

| Flag | Meaning |
|------|---------|
| `-t N` | Threads |
| `-e` | GLV endomorphism (secp address/rmd160/vanity) |
| `-A auto\|sse\|avx2\|avx512` | Hash160 vector width |
| `-b bits` / `-r start:end` | Bit or hex range |
| `-l compress\|uncompress\|both` | Pubkey form |
| `-x sequential\|random\|chaos\|…` | Key-picking pattern |
| `-q` `-s SEC` | Quiet / stats interval |
| `-y` | Dry-run: print config (incl. BSGS/RAM tips) and exit |

### Examples

```bash
# Puzzle 66 (CPU)
./keyhunt -m address -f tests/66.txt -b 66 -l compress -e -A auto -t 8 -q -s 10

# Ethereum
./keyhunt -m address -c eth -f eth.txt -t 8 -q -s 10

# Solana (CPU only)
./keyhunt -m address -c sol -f tests/sol_sample.txt -t 8 -q -s 10

# Vanity
./keyhunt -m vanity -v 1Cool -e -t 8

# RMD160 / xpoint
./keyhunt -m rmd160 -f tests/66.rmd -l compress -e -t 8
./keyhunt -m xpoint -f tests/_xpoint_g.txt -t 8

# Kangaroo (tiny range demo)
./keyhunt -m kangaroo -f tests/125.txt -r 1:100000
```

### Measured CPU speeds (this bench host)

Intel i7-920 @ 2.67 GHz (SSE hash160), Windows MinGW `keyhunt.exe`, ~15 s window:

| Mode | Sustained | Notes |
|------|----------:|-------|
| address BTC | **6.56 M**/s | `-t 8 -l compress` |
| address BTC + `-e` | **8.16 M**/s | |
| address ETH | **3.24 M**/s | |
| address SOL | **70.7 K**/s | ed25519 |
| rmd160 | **7.33 M**/s | |
| xpoint | **11.3 M**/s | |
| vanity + `-e` | **8.07 M**/s | |
| minikeys | **48.5 K**/s | |
| mnemonic | **247 K**/s | BIP39 |
| bsgs | **~26 G**/s† | `-b 40 -n 1048576 -t 8` |

† Effective giant-step coverage, not raw EC. Full tables: [docs/SPEEDS.md](docs/SPEEDS.md).

---

## BSGS on CPU (`-m bsgs`) — `-n` / `-k` tuning

Rules (classic [Keyhunt](https://github.com/albertobsd/keyhunt)):

1. **`-n` cannot be &lt; 1048576 (2²⁰)** — TrueCollider errors if smaller.
2. **Optimal `-k` are powers of 2:** `1,2,4,8,16,32,64,128,…` — non–power-of-2 gets a warning.
3. Exceeding **k max** for a given N can cause suboptimal speed or missed hits.
4. **`-k auto`** (or `-M auto` with default k) picks recommended k (and larger N when RAM is high) from host RAM / `-M`.

```bash
./keyhunt -m bsgs -f tests/125.txt -b 125 -R -k 512 -t 8 -S -q -s 10
./keyhunt -m bsgs -f tests/125.txt -b 125 -k auto -y          # dry-run shows recommend
./keyhunt -m bsgs -f tests/125.txt -b 125 -M 8192 -k auto -t 4
```

### Valid N and maximum K (by bit class)

| bits | `-n` (hex) | k max |
|-----:|------------|------:|
| 20 | `0x100000` | 1 |
| 22 | `0x400000` | 2 |
| 24 | `0x1000000` | 4 |
| 26 | `0x4000000` | 8 |
| 28 | `0x10000000` | 16 |
| 30 | `0x40000000` | 32 |
| 32 | `0x100000000` | 64 |
| 34 | `0x400000000` | 128 |
| 36 | `0x1000000000` | 256 |
| 38 | `0x4000000000` | 512 |
| 40 | `0x10000000000` | 1024 |
| 42 | `0x40000000000` | 2048 |
| 44 | `0x100000000000` | 4096 |
| 46 | `0x400000000000` | 8192 |
| 48 | `0x1000000000000` | 16384 |
| 50 | `0x4000000000000` | 32768 |
| 52 | `0x10000000000000` | 65536 |
| 54 | `0x40000000000000` | 131072 |
| 56 | `0x100000000000000` | 262144 |
| 58 | `0x400000000000000` | 524288 |
| 60 | `0x1000000000000000` | 1048576 |
| 62 | `0x4000000000000000` | 2097152 |
| 64 | `0x10000000000000000` | 4194304 |

Default N is `0x100000000000` (44-bit class, k max 4096).

### RAM → recommended `-k` / `-n`

| RAM | Suggested flags |
|-----|-----------------|
| 2 GB | `-k 128` |
| 4 GB | `-k 256` |
| 8 GB | `-k 512` |
| 16 GB | `-k 1024` |
| 32 GB | `-k 2048` |
| 64 GB | `-n 0x100000000000 -k 4096` |
| 128 GB | `-n 0x400000000000 -k 4096` |
| 256 GB | `-n 0x400000000000 -k 8192` |
| 512 GB | `-n 0x1000000000000 -k 8192` |
| 1 TB | `-n 0x1000000000000 -k 16384` |
| 2 TB | `-n 0x4000000000000 -k 16384` |
| 4 TB | `-n 0x4000000000000 -k 32768` |
| 8 TB | `-n 0x10000000000000 -k 32768` |

Do **not** rely on swap for BSGS tables. Use `-S` to save blooms/tables to disk after first build.

---

# 2. GPU

Enable with `-U cuda` (NVIDIA, `keyhunt_cuda.exe`) or `-U opencl` (hash160 offload; EC on CPU).

### Flags

| Flag | Meaning |
|------|---------|
| `-U cuda\|opencl\|none` | Backend (default none) |
| `-G N` | Host batch size hint (keys); clamped by `-M` / free VRAM |
| `-M MB\|auto\|2G` | Memory budget — CUDA batch/VRAM; also BSGS bloom budget |
| `-t 1` | Prefer low thread count with CUDA (GPU lock) |

Inspired by [Collider-bsgs](https://github.com/pp717/Collider-bsgs) (`-w`/`-htsz` → our `-k`/`-M`) and [Rotor-Cuda](https://github.com/Mehdi256/Rotor-Cuda) (`--gpux` → internal 256-thread grids + chunked launches).

### Commands by mode

```bash
# Address / rmd160 / ETH / taproot
keyhunt_cuda.exe -m address -f tests/66.txt -b 66 -l compress -U cuda -M auto -t 1 -q -s 5
keyhunt_cuda.exe -m rmd160 -f tests/66.rmd -l compress -U cuda -M 2048 -t 1 -q -s 5
keyhunt_cuda.exe -m address -c eth -f eth.txt -U cuda -M auto -t 1 -q -s 5
keyhunt_cuda.exe -m address -c troot -f troot.txt -U cuda -M auto -t 1 -q -s 5

# Vanity / xpoint / pubkey2addr / minikeys (GPU EC + host filter)
keyhunt_cuda.exe -m vanity -v 1Love -U cuda -M auto -t 1 -q -s 5
keyhunt_cuda.exe -m xpoint -f tests/_xpoint_g.txt -U cuda -M auto -t 1
keyhunt_cuda.exe -m pubkey2addr -f tests/66.txt -U cuda -M auto -t 1

# Mnemonic / poetry / brainwallet: GPU after privkey derived
keyhunt_cuda.exe -m mnemonic -f tests/66.txt -U cuda -M auto -t 1 -q -s 5

# BSGS: GPU assists baby-table build; search loop still CPU
keyhunt_cuda.exe -m bsgs -f tests/125.txt -b 125 -k auto -U cuda -M auto -t 4 -S

# Dry-run memory / batch plan
keyhunt_cuda.exe -m address -f tests/66.txt -U cuda -M auto -y

# Solana: CUDA SHA512 + host ed25519 ge
keyhunt_cuda.exe -m address -c sol -f tests/sol_sample.txt -r 1:8 -U cuda -M auto -t 1

# Dry-run memory / batch plan
keyhunt_cuda.exe -m address -f tests/66.txt -U cuda -M auto -y
```

### CUDA speed notes (RTX 3060 Ti, this host)

Earlier benches with small `-G 128`: ~**110–154 K** keys/s address (GPU EC + **host** hash). Larger `-M auto` batches improve occupancy; host hash remains the bottleneck until device hash160 is production-ready. On SSE-only CPUs, CPU + `-e` can still beat CUDA for address mode.

| Partial / host finish | Detail |
|------------------------|--------|
| Solana | GPU SHA512+clamp; host `ge_scalarmult_base` |
| BSGS GRP loop | CPU additions; GPU assists baby build + giant `ComputePublicKey` |
| Device bloom hit path | Host filter (device hash160 incorrect) |

Details: [gpu/README.md](gpu/README.md).

---

# 3. Memory flags (`-M`)

| Usage | Effect |
|-------|--------|
| `-M auto` | CUDA: ~60% free VRAM → batch size. BSGS: can drive `-k auto` sizing |
| `-M 512` / `-M 2048` / `-M 2G` | Cap budget in MB (or GB suffix) |
| `-M matrix` | Legacy matrix screen (not memory) |
| `-G N` | Explicit batch; clamped if over `-M` |

Prints a **memory plan** at CUDA / BSGS startup. Combine with `-y` to preview without searching.

---

# 4. Build

### Windows CPU (MinGW)

```bat
build_mingw_native.bat
REM → keyhunt.exe
```

### Windows CUDA (VS 2022 + CUDA 12.x)

```bat
build_cuda_vs2022.bat
REM → keyhunt_cuda.exe
```

### Linux / macOS

```bash
make -j$(nproc)
./keyhunt -h
# CUDA:
cmake -B build-cuda -DENABLE_CUDA=ON && cmake --build build-cuda -j
```

More: [docs/BUILD.md](docs/BUILD.md).

---

# 5. Output & dry-run

| Output | Content |
|--------|---------|
| `FOUND_BTC.txt` | BTC-family hits |
| `FOUND_ETH.txt` | ETH/ETC |
| `FOUND_SOL.txt` | Solana |
| `KEYFOUNDKEYFOUND.txt` | Legacy combined log |
| `VANITYKEYFOUND.txt` | Vanity hits |

```bash
./keyhunt -m address -f tests/66.txt -b 66 -U cuda -M auto -y
```

---

# 6. Docs map

| Doc | Audience |
|-----|----------|
| [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md) | First run |
| [docs/COMMANDS.md](docs/COMMANDS.md) | Copy-paste recipes |
| [docs/SPEEDS.md](docs/SPEEDS.md) | Measured rates |
| [docs/TEST_RESULTS.md](docs/TEST_RESULTS.md) | Latest smoke PASS/FAIL |
| [docs/BUILD.md](docs/BUILD.md) | Compilers |
| [gpu/README.md](gpu/README.md) | CUDA / OpenCL internals |
| [docs/ROADMAP.md](docs/ROADMAP.md) | Future work |

---

## Disclaimer

Education, puzzles, and research on keys/ranges you are authorized to search. Use at your own risk.

## License / credits

See repository license. Upstream: Alberto Keyhunt, Jean Luc Pons lineage (VanitySearch / BSGS / Kangaroo), Collider-bsgs / Rotor-Cuda conventions for GPU memory UX.
