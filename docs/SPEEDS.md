# Measured speeds (TrueCollider / KeyCollider)

Real rates captured from live binaries on the maintainer benchmark host.  
Re-run: `powershell -ExecutionPolicy Bypass -File .\scripts\run_benchmarks.ps1`

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

## CUDA results (RTX 3060 Ti) — custom CUDA edition

| Mode | Command | Sustained / peak |
|------|---------|-----------------:|
| address BTC (sequential GRP=1024) | `-U cuda -M auto -t 1 -x sequential` | **~90–96 Mkeys/s** peak (large auto batch) |
| address BTC (prior GRP=256) | same | ~165–166 Mkeys/s (more centers / occupancy) |
| rmd160 (sequential GRP) | same | ~same EC+hash path |
| xpoint (sequential GRP) | `-m xpoint -U cuda …` | higher than address when GRP path active |
| address ETH | `-c eth -U cuda -M auto -t 1` | lower (Keccak often host-side) |
| mnemonic | `-m mnemonic -U cuda` | mnemonics/s (PBKDF2-bound; **not** EC Mk/s) |

Path: **device GRP keystream** (G-table + Montgomery batch inv, **GRP=1024**, PreferL1, auto host batch up to ~8–14M keys so thousands of centers stay in flight). Falls back to per-key `scalar_mul_g` for non-consecutive keys, ETH, endomorphism, or host-filter mode. Prefer `-M auto` / `-t 1`.

KeyHunt-Cuda README ~1013 Mk/s is **XPOINT** on a GTX 1650 with GRP=2048 — not 1:1 with BTC address+hash160 on a 3060 Ti. GRP=1024 trades some occupancy vs GRP=256 on this card; further gain needs less host packing overhead / dual streams.

Measured 2026-07-26: GRP=256 ≈165–166 Mkeys/s; GRP=1024 + larger batches ≈90–96 Mkeys/s peak (was ~15–16 with per-key scalar×G; ~24 with GRP=1024 undersubscribed).

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
- Sequential CUDA GRP (~165 Mkeys/s) now beats this host's SSE CPU address path; sparse/non-stride-1 modes still use the older per-key GPU EC path.
- Prefer `-t 1` with CUDA to avoid GPU lock contention.
- Roadmap leftovers (Kangaroo GPU, BSGS GRP throughput): [ROADMAP.md](ROADMAP.md).
