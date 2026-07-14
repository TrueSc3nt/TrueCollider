# TrueCollider / KeyCollider

[![CI](https://github.com/TrueSc3nt/TrueCollider/actions/workflows/ci.yml/badge.svg)](https://github.com/TrueSc3nt/TrueCollider/actions/workflows/ci.yml)
[![Release](https://github.com/TrueSc3nt/TrueCollider/actions/workflows/release.yml/badge.svg)](https://github.com/TrueSc3nt/TrueCollider/actions/workflows/release.yml)

<p align="center">
  <b>TrueCollider</b> (also <b>KeyCollider</b>) — high-performance multi-currency<br/>
  private-key, seed, vanity, and pubkey discrete-log hunter.
</p>

<p align="center">
  <i>CPU SIMD · optional NVIDIA CUDA / OpenCL · puzzles · research · education</i>
</p>

Based on [Keyhunt](https://github.com/albertobsd/keyhunt) by Alberto · Developed & modified by **TrueScent**  
Repo: **[github.com/TrueSc3nt/TrueCollider](https://github.com/TrueSc3nt/TrueCollider)**

> [Getting started](docs/GETTING_STARTED.md) · [Command cookbook](docs/COMMANDS.md) · [Full `-h` dump](docs/HELP_DUMP.txt) · [Speeds](docs/SPEEDS.md) · [GPU honesty](gpu/README.md) · [Windows examples](examples/) · [Roadmap](docs/ROADMAP.md)

---

## A Little Intro

**KeyCollider / TrueCollider** by TrueScent is an open-source multi-coin key-search toolkit: Bitcoin-family addresses, Ethereum, Solana, vanity, brainwallets, BSGS, and Pollard's kangaroo — on CPU SIMD with optional NVIDIA CUDA. It searches ranges and patterns you supply; it is not a wallet scraper or a “find free BTC” oracle.

---

## Table of contents

1. [Brand / what it is](#brand--what-it-is)
2. [Quick start (Windows CPU + CUDA)](#quick-start-windows-cpu--cuda)
3. [Full modes table](#full-modes-table--m)
4. [Supported coins](#supported-coins--c)
5. [Complete flag reference](#complete-flag-reference)
6. [Per-mode command examples](#per-mode-command-examples)
7. [Online balance checking (`-N`)](#online-balance-checking--n)
8. [GPU / CUDA](#gpu--cuda)
9. [BSGS tuning (`-n` / `-k` / `-M`)](#bsgs-tuning--n----k----m)
10. [Output files](#output-files)
11. [Performance notes](#performance-notes)
12. [Example `.bat` index](#example-bat-index)
13. [Docs map](#docs-map)
14. [Disclaimer](#disclaimer)

---

## Brand / what it is

**TrueCollider / KeyCollider** walks private keys (or seeds / mnemonics / minikeys / brainwallets) as fast as your hardware allows and checks them against:

- addresses (BTC, ETH, LTC, DOGE, XRP, BCH, BTG, ETC, Taproot, Solana, …)
- raw RIPEMD-160 (hash160)
- public-key X coordinates
- full public keys (BSGS / kangaroo discrete log in a known range)
- vanity address prefixes

Hits append to **`FOUND_BTC.txt` / `FOUND_ETH.txt` / `FOUND_SOL.txt`** and legacy **`KEYFOUNDKEYFOUND.txt`**.

Built for Bitcoin puzzle ranges, vanity, multi-coin lists, BIP-39/BIP-32 experiments, and research on secp256k1 / ed25519. It is **not** a “find any wallet” tool — full 256-bit search is not practical. Only search keys/ranges you are authorized to.

Built-in help mirrors this document:

```bat
keyhunt.exe -h
keyhunt_cuda.exe -h
```

A machine dump of the current help text lives in [`docs/HELP_DUMP.txt`](docs/HELP_DUMP.txt).

---

## Quick start (Windows CPU + CUDA)

### Prerequisites

| Build | Need |
|-------|------|
| CPU (`keyhunt.exe`) | [MSYS2](https://www.msys2.org/) MinGW-w64 **or** an already-built release binary |
| CUDA (`keyhunt_cuda.exe`) | Visual Studio 2022 Build Tools + NVIDIA CUDA Toolkit 12.x + NVIDIA GPU |

### Build CPU

```bat
examples\build_cpu.bat
REM or: build_mingw_native.bat
REM → keyhunt.exe
```

### Build CUDA

```bat
examples\build_cuda.bat
REM or: build_cuda_vs2022.bat
REM → keyhunt_cuda.exe
```

### First tiny runs (fixtures under `tests/`)

```bat
REM BTC puzzle #66 address target, 66-bit range
keyhunt.exe -m address -f tests\66.txt -b 66 -l compress -e -A auto -t 8 -q -s 10

REM Dry-run (print config / memory plan, no search)
keyhunt.exe -m address -f tests\66.txt -b 66 -U cuda -M auto -y

REM Solana sample (CPU; CUDA optional with -U cuda)
keyhunt.exe -m address -c sol -f tests\sol_sample.txt -t 4 -q -s 5
```

Linux / macOS:

```bash
make -j$(nproc)
./keyhunt -h
cmake -B build-cuda -DENABLE_CUDA=ON && cmake --build build-cuda -j
```

More compilers: [docs/BUILD.md](docs/BUILD.md). Beginner path: [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md).

---

## Full modes table (`-m`)

| Mode | Target / input | Purpose |
|------|----------------|---------|
| `address` | Addresses, one per line | Default: privkey → pubkey → address vs filter |
| `rmd160` | 40-char hex hash160 | Match raw RIPEMD-160 (skip Base58) |
| `xpoint` | Pubkeys / x-only hex (64 / 66 / 130 chars) | Match public-key **X** |
| `bsgs` | Compressed / uncompressed pubkeys | Baby-step giant-step DL in a range |
| `kangaroo` | One pubkey + `-r` / `-b` | Pollard's kangaroo (**CPU only**) |
| `vanity` | Prefix via `-v` (no `-f` required for the prefix) | Address prefix search |
| `minikeys` | Address list (`-f`) | Bitcoin minikey (`S…`) grind |
| `mnemonic` | Address list | BIP-39 → BIP-32 → address |
| `poetry` | Address list | Poetry wordlist → hex key |
| `brainwallet` | Address list | Wordlist passphrase → SHA256 → key |
| `pubkey2addr` | Address list | Random key → address (defaults `-x random`) |
| `pub2rmd` | — | **Removed** — use `-m rmd160` |

### Sample fixtures (`tests/`)

| File | Use |
|------|-----|
| `tests/66.txt` | Puzzle 66 P2PKH address |
| `tests/66.rmd` | Puzzle 66 hash160 |
| `tests/125.txt` | Puzzle 125 pubkey (BSGS / kangaroo) |
| `tests/sol_sample.txt` | Solana sample address |
| `tests/_eth_1.txt` | Ethereum sample |
| `tests/_xpoint_g.txt` | X-point demos |
| `tests/_pubkey_g.txt` | Pubkey demos |
| `tests/poetry.txt` | Poetry wordlist |
| `tests/brainwalletwords.txt` | Brainwallet wordlist |
| `tests/unsolvedpuzzles.rmd` | Unsolved puzzle hash160s |
| `tests/1to32.rmd` / `1to32.eth` | Tiny sequential demos |

---

## Supported coins (`-c`)

| Flag | Curve / encode | Address forms | Notes |
|------|----------------|---------------|-------|
| `btc` (default) | secp256k1 · hash160 | `1…` `3…` `bc1q…` | P2SH `3…` script-hash path when present |
| `eth` | secp256k1 · keccak256 | `0x…` | |
| `etc` | secp256k1 · keccak256 | `0x…` | Ethereum Classic |
| `ltc` | secp256k1 · hash160 | `L…` | |
| `doge` | secp256k1 · hash160 | `D…` | |
| `xrp` | secp256k1 · hash160 | `r…` | |
| `bch` | secp256k1 · hash160 | CashAddr / legacy | |
| `btg` | secp256k1 · hash160 | `G…` | |
| `troot` | secp256k1 · taproot tweak | `bc1p…` / 32-byte x-only | Often with BIP-86 `-p` |
| `sol` | **ed25519** | Base58 pubkey | Address mode; CUDA prefers device ge, host fallback |
| `all` | multi secp | Mixed file | **Do not mix Solana** |
| `auto` | detect from file | — | Picks BTC/ETH/… from content |

Applies mainly to `address`, `rmd160`, `vanity`, `pubkey2addr` (currency-dependent).

---

## Complete flag reference

Parsed from `getopt` in `keyhunt.cpp` and `menu()` / `-h`. **Do not invent flags** — only what exists below.

### Required / primary

| Flag | Arg | Description |
|------|-----|-------------|
| `-m` | mode | Search mode (see table). Default: `address` |
| `-f` | file | Target file (addresses, hashes, pubkeys — depends on mode) |
| `-h` | — | Print full built-in help and exit successfully |

### Crypto / look

| Flag | Arg | Description |
|------|-----|-------------|
| `-c` | crypto | `btc` `eth` `ltc` `doge` `xrp` `sol` `bch` `btg` `etc` `troot` `all` `auto` |
| `-l` | look | `compress` `uncompress` `both` (default both). address / rmd160 / pubkey2addr |
| `-N` | `[url]` | **Online balance check flag** when a key is found (optional URL). See [Balance checking](#online-balance-checking--n). Requires `curl`. |
| `-p` | path | BIP-32 path (address / rmd160), e.g. `m/84'/0'/0'/0` |
| `-D` | count | Child indices `1..100` per path (mnemonic **or** with `-p`) |

### Range / stride / bits

| Flag | Arg | Description |
|------|-----|-------------|
| `-r` | `START` or `START:END` | Hex private-key range. One value → start..curve order |
| `-b` | bits | Bit-length window (`1..256`). Puzzle-style ranges |
| `-T` | unix_ts | Timestamp → ~4×10⁹ key window starting at that integer |
| `-Z` | bytes | Strip N leading zero bytes (**requires `-b` first**). Shrinks padded ranges |
| `-n` | number | Sequential keys per cycle **or** BSGS table span `N` (hex `0x…` or decimal). BSGS: ≥ `1048576`, exact square root |
| `-I` | stride | Stride for address / rmd160 / xpoint |
| `-R` | — | Random / BSGS random giant-step convenience (`FLAGRANDOM`, BSGS mode 3) |

### BSGS-specific

| Flag | Arg | Description |
|------|-----|-------------|
| `-B` | mode | Giant-step strategy: `sequential` `backward` `both` `random` `dance` |
| `-k` | value\|`auto` | K factor (prefer powers of 2). `auto` from RAM / `-M` |
| `-S` | — | Save/load BSGS bloom filters + baby-step table to disk |
| `-z` | value | Bloom size multiplier (≥ 1). Default 1 |

### Search pattern (`-x`) — Collider modes

| Value | Description |
|-------|-------------|
| `sequential` | Linear walk start→end |
| `random` | Random keys in range (also `-R`) |
| `chaos` | Logistic map `r=3.99999` |
| `gravity` | Bias toward previously found regions |
| `spiral` | Archimedean spiral from range midpoint |
| `reverse` | Inverted BSGS baby/giant roles |
| `auto` | Cycle spiral → chaos → gravity → reverse |

Works with essentially all modes including BSGS. See also [`COLLIDER_MODES_README.md`](COLLIDER_MODES_README.md).

### Performance / backends

| Flag | Arg | Description |
|------|-----|-------------|
| `-t` | N | CPU threads (default 1). Prefer `-t 1` with CUDA |
| `-e` | — | GLV endomorphism (~3× for CPU address / rmd160 / vanity). **Not** on GPU EC |
| `-A` | mode | CPU vector: `auto` `none` `sse` `avx` `avx2` `avx512` |
| `-U` | backend | GPU: `none` (default) `cuda` `opencl` |
| `-G` | N | GPU batch size hint `1..1048576` (clamped by `-M` / VRAM) |
| `-M` | budget | `auto` / `512` / `2048` / `2G` / … — CUDA VRAM + BSGS bloom budget. Legacy: `-M matrix` screen |
| `-y` | — | Dry-run: print resolved config (incl. memory plan) and exit |

### Mode-specific extras

| Flag | Mode | Description |
|------|------|-------------|
| `-v` | vanity | Vanity Base58 prefix, e.g. `1Cool` |
| `-C` | minikeys | Base minikey string (exactly 22 chars, starts with `S`) |
| `-8` | minikeys | Custom Base58 alphabet (exactly 58 chars) |
| `-w` | mnemonic / brainwallet | Word count. Mnemonic: `0`/`12`/`15`/`18`/`21`/`24`. Brainwallet: `0`=random or listed counts |
| `-L` | mnemonic | BIP-39 language or `all` (10 languages) |
| `-W` | mnemonic | Match ETH (keccak) instead of BTC |

### Output / misc

| Flag | Arg | Description |
|------|-----|-------------|
| `-s` | seconds | Stats interval. `0`=off. Default 30 |
| `-q` | — | Quiet (less per-thread spam) |
| `-V` | — | Verbose derivation / chain code |
| `-6` | — | Skip SHA-256 checksum validation on cached data files |
| `-d` | — | Debug logs (bech32 decode, BSGS internals, …) |

### Not implemented / partial (honest)

| Item | Reality |
|------|---------|
| `-E` | Present in the `getopt` option string but **no handler** — do not use |
| `-N` balance check | **Wired**: on each hit, `writekey` / `writekeyeth` / `writekeysol` call `node_check_balance` (curl → public APIs or BTC Core RPC). Needs `curl` on PATH; live RPC for `-Nhttp://...`. See [Balance checking](#online-balance-checking--n). |
| `pub2rmd` mode | Removed; use `-m rmd160` |
| Device CUDA hash160 + on-GPU bloom | **Shipped** — self-test must pass; otherwise host-filter fallback |
| Full GPU GRP BSGS loop | **Shipped** (device GRP + host bloom); not yet throughput-tuned |
| Kangaroo on GPU | **Shipped** (`-U cuda`): small ranges GPU batch EC scan; larger ranges multi-walker DP (device jumps, host DP table). CPU fallback always. |

---

## Per-mode command examples

On Windows use `keyhunt.exe` / `keyhunt_cuda.exe`. Paths below use `tests\` fixtures.

### address

```bat
keyhunt.exe -m address -f tests\66.txt -b 66 -l compress -e -A auto -t 8 -q -s 10
keyhunt.exe -m address -f tests\66.txt -r 20000000000000000:40000000000000000 -l compress -x chaos -t 8
keyhunt.exe -m address -f tests\66.txt -b 72 -T 1421345234 -t 8 -x auto
keyhunt.exe -m address -f tests\66.txt -b 72 -Z 6 -t 8
keyhunt.exe -m address -c eth -f tests\_eth_1.txt -t 8 -q -s 10
keyhunt.exe -m address -c sol -f tests\sol_sample.txt -t 4 -q -s 5
keyhunt.exe -m address -c troot -f troot_targets.txt -t 8
keyhunt.exe -m address -c all -f mixed_targets.txt -t 8
keyhunt.exe -m address -c auto -f mixed_targets.txt -t 8
keyhunt.exe -m address -p "m/84'/0'/0'/0" -D 20 -f tests\66.txt -V -t 8
keyhunt.exe -m address -c troot -p "m/86'/0'/0'/0" -D 10 -f troot_targets.txt -V -t 8
```

### rmd160

```bat
keyhunt.exe -m rmd160 -f tests\66.rmd -l compress -e -x gravity -t 8 -q -s 10
keyhunt.exe -m rmd160 -f tests\unsolvedpuzzles.rmd -b 66 -l compress -t 8
keyhunt.exe -m rmd160 -p "m/44'/0'/0'/0" -D 10 -f tests\66.rmd -t 8
keyhunt.exe -m rmd160 -c ltc -f ltc_hashes.rmd -t 8
```

### xpoint

```bat
keyhunt.exe -m xpoint -f tests\_xpoint_g.txt -t 8
keyhunt.exe -m xpoint -f tests\_pubkey_g.txt -b 40 -t 4 -x spiral
```

### bsgs

```bat
keyhunt.exe -m bsgs -f tests\125.txt -b 125 -B sequential -n 0x1000000 -t 4
keyhunt.exe -m bsgs -f tests\125.txt -b 125 -R -k 512 -t 8 -S -q -s 10
keyhunt.exe -m bsgs -f tests\125.txt -b 125 -k auto -y
keyhunt.exe -m bsgs -f tests\125.txt -b 125 -M 8192 -k auto -t 4
keyhunt.exe -m bsgs -f tests\125.txt -x auto -S -t 8
```

### kangaroo (CPU / CUDA)

```bat
keyhunt.exe -m kangaroo -f tests\_pubkey_g.txt -r 1:1000
keyhunt.exe -m kangaroo -f tests\125.txt -b 40 -t 4
keyhunt_cuda.exe -m kangaroo -f tests\_pubkey_g.txt -r 1:1000 -U cuda
```

Ranges ≤ 2²⁴: sequential EC walk (CUDA batch scan with `-U cuda`). Larger ranges: DP kangaroo (CUDA multi-walker jumps + host DP table).

### vanity

```bat
keyhunt.exe -m vanity -v 1Cool -e -t 8
keyhunt.exe -m vanity -v bc1qabc -t 8
```

### minikeys

```bat
keyhunt.exe -m minikeys -f tests\66.txt -t 4
keyhunt.exe -m minikeys -C SRPqx8QiwnW4WNWnTVa2W5 -f tests\66.txt
```

### mnemonic

```bat
keyhunt.exe -m mnemonic -w 24 -L english -f tests\66.txt -t 4
keyhunt.exe -m mnemonic -W -L all -f tests\_eth_1.txt -t 8
keyhunt.exe -m mnemonic -D 50 -f tests\66.txt -t 8
keyhunt.exe -m mnemonic -w 12 -L english -f tests\66.txt -t 4 -q -s 10
```

Paths checked: BIP-44 / BIP-49 / BIP-84 × `-D` indices.

### poetry / brainwallet

```bat
keyhunt.exe -m poetry -f tests\66.txt -t 4
keyhunt.exe -m brainwallet -f tests\66.txt -t 8
keyhunt.exe -m brainwallet -w 3 -f tests\66.txt -t 4
```

### pubkey2addr

```bat
keyhunt.exe -m pubkey2addr -f tests\66.txt -x auto -t 4
keyhunt.exe -m pubkey2addr -c eth -f tests\_eth_1.txt -t 8
keyhunt.exe -m pubkey2addr -f tests\66.txt -q -s 10 -t 4
```

### Dry-run / quiet / debug

```bat
keyhunt.exe -m address -f tests\66.txt -b 66 -y
keyhunt.exe -m address -f tests\66.txt -U cuda -M auto -y
keyhunt.exe -m address -f tests\66.txt -q -s 5 -d -t 2
```

### Puzzle batch style (72–160)

Funding timestamp often used: `1421345234` (2015-01-15). Example:

```bat
keyhunt.exe -m address -f tests\unsolvedpuzzles.rmd -b 72 -T 1421345234 -t 8 -x auto
```

Also see `run_puzzle66_example.bat` and [`PUZZLE_SEARCH_README.md`](PUZZLE_SEARCH_README.md).

---

## Online balance checking (`-N`)

### Status: **wired**

| Piece | Status |
|-------|--------|
| CLI `-N` / `-Nhttp://user:pass@host:port` | Parsed — sets `FLAGNODECHECK` and optional `NODE_RPC_URL` |
| `node_check_balance(address, crypto)` | Implemented — shells out to `curl` |
| Call from `writekey` / `writekeyeth` / `writekeysol` on hit | **Wired** — results printed and appended to `FOUND_*.txt` |

```bat
REM Public APIs (no URL) — BTC blockstream.info, ETH/ETC etherscan, LTC blockcypher
keyhunt.exe -m address -c btc -f tests\66.txt -N -t 8

REM Own Bitcoin Core RPC (scantxoutset start + addr(...))
keyhunt.exe -m address -c btc -f tests\66.txt -Nhttp://user:pass@127.0.0.1:8332 -t 8

REM Fixture hit + public API smoke (needs network + curl; expects ZERO on puzzle dust addr)
keyhunt.exe -m address -f tests\_btc_1to2.txt -r 1:2 -l compress -N -t 1
```

### Public API mapping (in `node_check_balance`)

| Crypto | Without custom URL |
|--------|--------------------|
| BTC | `https://blockstream.info/api/address/{addr}` |
| ETH / ETC | `https://api.etherscan.io/api?...&address={addr}` |
| LTC | `https://api.blockcypher.com/v1/ltc/main/addrs/{addr}/balance` |
| Other (`sol`, `doge`, …) | Printed as UNSUPPORTED |

With `NODE_RPC_URL` set, **BTC only** uses JSON-RPC `scantxoutset` via:

`http://user:pass@host:port` (default parse host `127.0.0.1:8332`). Real RPC needs a **live** Bitcoin Core (or compatible) node — dry-run alone does not query.

### Safety notes

- Requires **`curl`** on `PATH` (Windows: Win10+ built-in or install curl).
- Public APIs have **rate limits**; do not enable on high-rate vanity grinds expecting one HTTP call per hit.
- RPC URL embeds credentials in the command line (visible to other users / shell history) — prefer a local node ACL.
- APIs/third parties learn which addresses you asked about.
- Response parsing is heuristic (string-search for `"balance"` / `"funded_txo_sum"` / `"final_balance"` etc.) — not a production wallet audit tool.

Example helper: [`examples/balance_check.bat`](examples/balance_check.bat).

---

## GPU / CUDA

Runtime: `-U none` (default) · `-U cuda` · `-U opencl`.

| Path | Status |
|------|--------|
| BTC-family `address` / `rmd160` | GPU EC + **device** hash160 + **device** bloom (host fallback) |
| ETH / ETC `address` | GPU EC + host keccak + host bloom |
| Taproot `troot` | GPU EC + host tweak + filter |
| vanity / xpoint / pubkey2addr / minikeys | GPU EC + filter (device when hash160 ready) |
| mnemonic / poetry / brainwallet | Derive on CPU; GPU EC afterward |
| BSGS | GPU baby-table + **device GRP** giant-step (host bloom); serial cycles today |
| Solana `-c sol` | **Full device** ed25519 `ge_scalarmult_base` (host-ge fallback) |
| Kangaroo | **CUDA** (`-U cuda`): ≤2²⁴ GPU batch EC scan; larger multi-walker DP (device jumps, host table). CPU fallback |
| Device hash160 bloom search | **Shipped** when self-test passes |
| `-e` endomorphism on GPU EC | **No** |
| OpenCL | Host EC + GPU hash160 (`ENABLE_OPENCL` build; not default CUDA exe) |

### GPU examples

```bat
keyhunt_cuda.exe -m address -f tests\66.txt -b 66 -l compress -U cuda -M auto -t 1 -q -s 5
keyhunt_cuda.exe -m rmd160 -f tests\66.rmd -l compress -U cuda -M 2048 -t 1 -q -s 5
keyhunt_cuda.exe -m address -c eth -f tests\_eth_1.txt -U cuda -M auto -t 1
keyhunt_cuda.exe -m address -c troot -f troot.txt -U cuda -M auto -t 1
keyhunt_cuda.exe -m vanity -v 1Love -U cuda -M auto -t 1 -q -s 5
keyhunt_cuda.exe -m xpoint -f tests\_xpoint_g.txt -U cuda -M auto -t 1
keyhunt_cuda.exe -m pubkey2addr -f tests\66.txt -U cuda -M auto -t 1
keyhunt_cuda.exe -m mnemonic -f tests\66.txt -U cuda -M auto -t 1 -q -s 5
keyhunt_cuda.exe -m bsgs -f tests\125.txt -b 125 -k auto -U cuda -M auto -t 4 -S
keyhunt_cuda.exe -m address -c sol -f tests\sol_sample.txt -r 1:8 -U cuda -M auto -t 1
keyhunt_cuda.exe -m address -f tests\66.txt -U cuda -M auto -y
```

Details: [`gpu/README.md`](gpu/README.md). Quick sample: `run_gpu_cuda_example.bat`.

### Memory (`-M` / `-G`)

| Usage | Effect |
|-------|--------|
| `-M auto` | CUDA: ~60% free VRAM → batch. BSGS: can drive `-k auto` |
| `-M 512` / `-M 2048` / `-M 2G` | Cap budget (MB or GB suffix) |
| `-M matrix` | Legacy matrix screen (not memory) |
| `-G N` | Explicit batch; clamped if over `-M` |

Combine with `-y` to preview the memory plan without searching.

---

## BSGS tuning (`-n` / `-k` / `-M`)

Rules (classic Keyhunt):

1. **`-n` cannot be &lt; 1048576 (2²⁰)** — TrueCollider errors if smaller.
2. Prefer **`-k` powers of 2:** `1,2,4,8,16,…` — non–power-of-2 warns.
3. Exceeding **k max** for a given N can hurt speed or miss hits.
4. **`-k auto`** (or `-M auto` with default k) picks from host RAM / `-M`.

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

Do **not** rely on swap for BSGS tables. Use `-S` after the first build.

---

## Output files

| File | Content |
|------|---------|
| `FOUND_BTC.txt` | BTC-family hits |
| `FOUND_ETH.txt` | ETH / ETC |
| `FOUND_SOL.txt` | Solana |
| `KEYFOUNDKEYFOUND.txt` | Legacy combined log |
| `VANITYKEYFOUND.txt` | Vanity hits |

Hits are appended (not overwritten). Check the working directory where you launched `keyhunt`.

---

## Performance notes

Prefer CPU when you have AVX2/AVX-512, want `-e`, heavy BSGS tables in host RAM, or OpenCL-only boxes.

Prefer CUDA when GPU EC batches with `-M auto` beat your CPU (host hash still often dominates until device hash160 is trusted).

| Tip | Why |
|-----|-----|
| `-e` on CPU secp modes | Multiplies candidates per EC |
| `-A auto` | Picks AVX-512 → AVX2 → SSE |
| `-l compress` | One pubkey form when targets allow |
| `-t 1` with `-U cuda` | Avoid oversubscription vs GPU lock |
| `-q -s 5` | Readable speed line without spam |
| `-y` before long BSGS | Validate N/k/RAM plan |

Measured rates on one host: [`docs/SPEEDS.md`](docs/SPEEDS.md). Recent smoke: [`docs/TEST_RESULTS.md`](docs/TEST_RESULTS.md).

Example CPU bench snapshot (older SSE host): address ~6–8 Mkeys/s with `-e`; SOL ~70 K/s; BSGS effective coverage much higher (giant-step, not raw EC).

---

## Example `.bat` index

All under [`examples/`](examples/). Edit threads / targets; run from repo root or any cwd (scripts `cd` to repo root).

| Script | What it runs |
|--------|----------------|
| [`build_cpu.bat`](examples/build_cpu.bat) | Wrapper → `build_mingw_native.bat` |
| [`build_cuda.bat`](examples/build_cuda.bat) | Wrapper → `build_cuda_vs2022.bat` |
| [`search_btc_address.bat`](examples/search_btc_address.bat) | `-m address` puzzle 66 |
| [`search_rmd160.bat`](examples/search_rmd160.bat) | `-m rmd160` |
| [`search_eth.bat`](examples/search_eth.bat) | `-c eth` |
| [`search_sol.bat`](examples/search_sol.bat) | `-c sol` |
| [`vanity_btc.bat`](examples/vanity_btc.bat) | `-m vanity -v 1Cool` |
| [`bsgs_example.bat`](examples/bsgs_example.bat) | `-m bsgs` + dry-run tip |
| [`kangaroo_example.bat`](examples/kangaroo_example.bat) | Tiny kangaroo demo |
| [`mnemonic_example.bat`](examples/mnemonic_example.bat) | BIP-39 |
| [`poetry_example.bat`](examples/poetry_example.bat) | Poetry |
| [`brainwallet_example.bat`](examples/brainwallet_example.bat) | Brainwallet |
| [`minikeys_example.bat`](examples/minikeys_example.bat) | Minikeys |
| [`xpoint_example.bat`](examples/xpoint_example.bat) | X-point |
| [`pubkey2addr_example.bat`](examples/pubkey2addr_example.bat) | pubkey2addr |
| [`balance_check.bat`](examples/balance_check.bat) | Documents `-N` (partial) |
| [`dry_run.bat`](examples/dry_run.bat) | `-y` CPU + CUDA if present |
| [`gpu_cuda_address.bat`](examples/gpu_cuda_address.bat) | CUDA address + `-M auto` |

Root-level leftovers (still valid): `run_keyhunt.bat`, `run_puzzle66_example.bat`, `run_sol_sample.bat`, `run_gpu_cuda_example.bat`.

---

## Docs map

| Doc | Audience |
|-----|----------|
| [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md) | Absolute beginners |
| [docs/COMMANDS.md](docs/COMMANDS.md) | Short cookbook |
| [docs/HELP_DUMP.txt](docs/HELP_DUMP.txt) | Raw `keyhunt.exe -h` |
| [docs/SPEEDS.md](docs/SPEEDS.md) | Measured rates |
| [docs/TEST_RESULTS.md](docs/TEST_RESULTS.md) | Smoke PASS/FAIL |
| [docs/BUILD.md](docs/BUILD.md) | Compilers |
| [gpu/README.md](gpu/README.md) | CUDA / OpenCL internals |
| [docs/ROADMAP.md](docs/ROADMAP.md) | Future work |
| [COLLIDER_MODES_README.md](COLLIDER_MODES_README.md) | `-x` search patterns |
| [PUZZLE_SEARCH_README.md](PUZZLE_SEARCH_README.md) | Puzzle notes |

---

## Disclaimer

Education, puzzles, and research on keys/ranges you are authorized to search. Use at your own risk. Full random 256-bit search will not succeed in any practical time.

## License / credits

See repository license. Upstream: Alberto Keyhunt, Jean Luc Pons lineage (VanitySearch / BSGS / Kangaroo), Collider-bsgs / Rotor-Cuda conventions for GPU memory UX.

**Donations (project tip jars from built-in help):**

- BTC: `1HmztBLDnwwaKAGbtALsYvCNBuoJYEic3h`
- Tips to Iceland: `bc1q39meky2mn5qjq704zz0nnkl0v7kj4uz6r529at`
