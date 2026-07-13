# Roadmap & research-backed feature plan

This document is the product plan for TrueCollider: what exists, what competitors do, and what we should build next — ordered by impact for real users (puzzles, vanity, multi-coin, beginners).

---

## Landscape (what “serious” key search looks like in 2025–2026)

| Approach | Best for | Notes |
|----------|----------|--------|
| **Sequential / random scan + bloom/fuse** | Address lists, vanity, puzzles with no pubkey | Classic Keyhunt / VanitySearch path. TrueCollider already has this + endomorphism + AVX2/512. |
| **BSGS** | Known **pubkey**, mid-size ranges | Memory–time tradeoff. TrueCollider has CPU BSGS. |
| **Pollard's Kangaroo / Lambda** | Known pubkey, **narrow–medium** ranges (Bitcoin puzzles) | State of the art on GPU (RCKangaroo / PSCKangaroo / JeanLucPons lineage). Better memory scaling than full BSGS for large ranges. |
| **GLV endomorphism** | secp256k1 address/rmd160/vanity | ~√3–3× effective coverage. TrueCollider: CPU `-e`. |
| **SIMD hash160** | Bottleneck after EC | SSE 4 / AVX2 8 / AVX-512 16. Done. |
| **GPU EC + host filter** | Address hunt on NVIDIA | TrueCollider CUDA path (batch still conservative). |
| **GPU hash only (OpenCL)** | AMD/Intel/NVIDIA hash offload | Done for compress address/rmd160. |
| **ed25519 grind (Solana)** | SOL vanity / target lists | CPU path done; GPU grinders elsewhere hit tens of M keys/s with comb + fe10 + base58 prefilter. |
| **CreateAccountWithSeed (Solana)** | Fast “vanity” that is **not** a standalone keypair | SHA256 only — different product; optional later mode. |
| **Hybrid patterns** | Exploration | spiral/chaos/gravity already in TrueCollider (ordering heuristics, not crypto breakthroughs). |

Sources of practice in the wild: [albertobsd/keyhunt](https://github.com/albertobsd/keyhunt), JeanLucPons Kangaroo/VanitySearch, RCKangaroo / PSCKangaroo forks, Solana CUDA vanity grinders (e.g. suffix GPU tools).

---

## TrueCollider today (honest matrix)

| Capability | Status |
|------------|--------|
| Multi-coin address (+ SOL ed25519, troot) | Done |
| Vanity / mnemonic / poetry / brainwallet / minikeys | Done (CPU) |
| BSGS CPU | Done |
| AVX2 + AVX-512 hash160 | Done |
| CUDA secp EC | Partial (stable small batches; device hash160 not production) |
| OpenCL hash160 | Done (EC on CPU) |
| Kangaroo | **Not yet** |
| Beginner docs | **This docs pass** |
| One-click Windows installer | Not yet |

---

## Suggested features (priority order)

### P0 — users who “don’t have a clue”

1. **Getting Started + Commands** (this folder) linked from README TL;DR.
2. **Example target files** for every mode (`tests/…`) with comments.
3. **`--dry-run` / config dump**: print resolved mode, coin, range, vector level, GPU backend before search.
4. **Windows “double-click” helpers**: `.bat` launchers (`run_puzzle66.bat`, `run_sol_sample.bat`) that call `keyhunt.exe` with safe defaults.
5. **Clearer hit files**: one format per coin (`FOUND_BTC.txt`, `FOUND_SOL.txt`) plus keep legacy `KEYFOUNDKEYFOUND.txt`.

### P1 — speed & puzzles (secp256k1)

6. **Pollard's Kangaroo mode** (`-m kangaroo`): pubkey + bit/range; CPU first, then CUDA/Vulkan.
7. **Safer CUDA batches**: fix device hash160 or keep host hash but raise batch without TDR (stream sync, pinned memory).
8. **OpenCL secp256k1**: parity with CUDA EC for AMD.
9. **BSGS UX**: auto `-k` suggestion from free RAM; progress % for random/dance; documented bloom save/load.
10. **Checkpoint / resume** for long random searches (save last key + RNG state).

### P2 — Solana & vanity

11. **`-m vanity -c sol`**: prefix/suffix on base58 ed25519 addresses (CPU), then CUDA ed25519 grind.
12. **Base58 mod-58^K prefilter** (as in modern SOL GPU grinders) before full encode.
13. Optional **CreateAccountWithSeed** grind mode (document that it is *not* a normal wallet keypair).

### P3 — product polish

14. **HTML/Markdown report** after run (keys tried, rate, mode, hardware).
15. **JSONL stats** for scripts / dashboards.
16. **Unified binary** CPU+CUDA+OpenCL with runtime `-U`.
17. **Signed releases** + GitHub Release assets (`keyhunt-windows-x64.zip`, `keyhunt-linux-x64.tar.gz`).
18. **Large pages / huge bloom** optional allocator for BSGS.

### Explicit non-goals (for now)

- Claiming “full 256-bit BTC search is practical.”
- Shipping closed-source “AI finds keys” nonsense.
- Mixing Solana into `-c all` (different curve).

---

## Implementation sketch: Kangaroo (next big crypto feature)

1. Input: compressed/uncompressed pubkey + `-b` / `-r`.
2. CPU prototype: distinguished points, tame/wild herds, endomorphism-aware walks (constants verified vs bitcoin-core).
3. Persist DP table; resume.
4. Port hot loop to CUDA (learn from RCKangaroo design; re-verify endo constants).
5. Document: “use Kangaroo when you have a **pubkey** and a **range**; use `address` when you only have an **address**.”

---

## Doc / GitHub shape (vs Alberto)

Alberto’s Keyhunt README is a long **mode tutorial**. TrueCollider keeps:

| Alberto style | TrueCollider style |
|---------------|--------------------|
| One giant README | **Hub README** + `docs/GETTING_STARTED` + `docs/COMMANDS` + `docs/ROADMAP` |
| Linux-first / WSL | **Windows + Linux** first-class |
| BTC/ETH focus | Multi-coin + SOL + troot, with honesty about GPU |
| Puzzle TL;DR | Same idea, plus “which mode do I pick?” table |

---

## Success metrics

- New user finds a hit on `tests/sol_sample.txt` or a tiny BTC range without asking for help.
- Puzzle runners know when to use `address` vs `bsgs` vs (future) `kangaroo`.
- README never claims GPU modes that are CPU-only.
