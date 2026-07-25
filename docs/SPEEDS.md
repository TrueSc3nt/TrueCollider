# Measured speeds (TrueCollider / KeyCollider)

Real rates captured from live binaries on the maintainer benchmark host.  
Re-run: `powershell -ExecutionPolicy Bypass -File .\run_benchmarks.ps1`

## Hardware

| Component | Spec |
|-----------|------|
| CPU | Intel Core i7-920 @ 2.67 GHz · 8 logical threads · SSSE3 (SSE hash160 path) |
| GPU | NVIDIA GeForce RTX 3060 Ti 8 GB |
| Builds | `keyhunt.exe` (MinGW) · `keyhunt_cuda.exe` (VS2022 + CUDA 12.8) |
| Date | 2026-07-13 |

## Method

- Stats interval `-s 5`, sample **15 s** sustained (and **5 s** peak for CUDA).
- Random walk in a large hex range so the run does not finish early.
- Rates taken from the last `[+] Total ... (N keys/s)` line printed by the tool.

## CPU results

| Mode | Example command | 15 s rate |
|------|-----------------|----------:|
| address BTC | `keyhunt -m address -f targets.txt -l compress -t 8 -A sse -q -s 5` | 6,561,245 keys/s |
| address BTC + endo | add `-e` | 8,163,328 keys/s |
| address ETH | `-c eth -t 8` | 3,240,413 keys/s |
| address SOL | `-c sol -t 8` | 70,724 keys/s |
| rmd160 | `-m rmd160 -t 8` | 7,325,559 keys/s |
| rmd160 + endo | add `-e` | 8,102,707 keys/s |
| xpoint | `-m xpoint -t 8` | 11,329,467 keys/s |
| vanity | `-m vanity -v 1Love -e -t 8` | 8,067,481 keys/s |
| pubkey2addr | `-m pubkey2addr -t 8` | 489,009,152 keys/s* |
| minikeys | `-m minikeys -t 8` | 48,537 keys/s |
| mnemonic | `-m mnemonic -t 8` | 247,057 mnemonics/s |
| poetry | `-m poetry -t 4` | 57,862,758 mnemonics/s |
| brainwallet | `-m brainwallet -t 4` | 94,648,934 mnemonics/s |
| bsgs | `-m bsgs -b 40 -n 1048576 -t 8` | 26,442,255,564 keys/s† |

\* As reported by the binary; not necessarily comparable 1:1 with `address`.  
† Effective BSGS coverage rate with a 1M baby table.

## CUDA results (RTX 3060 Ti)

| Mode | Command | 15 s sustained |
|------|---------|---------------:|
| address BTC | `-U cuda -M auto -t 1` | ~15.8 Mkeys/s |
| rmd160 | `-U cuda -M auto -t 1` | ~15–16 Mkeys/s (same EC path) |
| address ETH | `-c eth -U cuda -M auto -t 1` | lower (Keccak host/device mix) |

Path: **GPU secp256k1 EC** + **device** hash160/bloom when self-test passes (else host hash/keccak + host bloom). Prefer `-M auto`. Host packs up to 8× launch chunk (default chunk 65536); search kernels are chunked to avoid WDDM TDR.

Note: older logs showing ~29 Mkeys/s were inflated by a CUDA key-counter bug (fixed). Measured rates above are post-fix.

## GPU EC wired vs still CPU-only

| Mode | GPU? |
|------|------|
| address / rmd160 / ETH / troot / vanity / xpoint / pubkey2addr / minikeys | **Yes** — GPU EC; BTC-family uses device hash160+bloom when ready |
| mnemonic / poetry / brainwallet | **Yes** — GPU EC after derive |
| bsgs | **Yes** — GPU baby-table + device GRP (host bloom; serial cycles) |
| address `-c sol` | **Yes** — full device ed25519 ge |
| Full on-device hash160+bloom | **Yes** when self-test passes |
| kangaroo | **Yes** — CUDA batch EC scan (≤2²⁴) + multi-walker DP; CPU fallback |

See hub [README.md](../README.md) for commands and BSGS `-n`/`-k` tables.

## Notes

- AVX2/AVX-512 CPUs should beat these SSE-only CPU figures substantially for hash160 modes.
- On this SSE host, CPU `-e` often beats the current CUDA EC+host-hash path for BTC address/rmd160.
- Prefer `-t 1` with CUDA to avoid GPU lock contention.
- Roadmap leftovers (Kangaroo GPU, BSGS GRP throughput): [ROADMAP.md](ROADMAP.md).
