# TrueCollider - full mode bat library

Double-click any script, or run from repo root:

```bat
bats\01_address\btc_puzzle66.bat
```

Every script cds to the repository root via `bats\_common.bat`.

## Folders

| Folder | Covers |
|--------|--------|
| `00_build` | CPU / CUDA builds |
| `01_address` | `-m address` ranges, compress, endo, BIP paths, ETH/SOL, `-N`, `-T`, `-Z` |
| `02_rmd160` | `-m rmd160` |
| `03_xpoint` | `-m xpoint` |
| `04_bsgs` | `-m bsgs` + `-B` modes |
| `05_kangaroo` | `-m kangaroo` CPU/CUDA |
| `06_vanity` | `-m vanity` |
| `07_minikeys` | `-m minikeys` |
| `08_mnemonic` | BIP-39 random / lastword / mask / ETH / all langs |
| `09_poetry` | `-m poetry` |
| `10_brainwallet` | `-m brainwallet` |
| `11_pubkey2addr` | `-m pubkey2addr` |
| `12_patterns` | All `-x` patterns (incl. keyhole/pocket/afterimage/wave/…) + `-rs` / `--walk` |
| `13_crypto` | `-c` currencies (btc/eth/sol + placeholders) |
| `14_gpu` | `-U cuda` / `both`, batch, mnemonic CUDA |
| `15_research` | shadow160, weakrng, hex-mask, wif-mask, kangaroo-mod, hybrid-dl, gaudry |
| `16_options` | `-y`, `-A`, `-q`, `-V`, `-I`, `-z`, `-R -q` |
| `17_puzzles` | Puzzle bit-range recipes + new walk-mode known-hit smokes |

Smoke everything quick: `bats\RUN_ALL_SMOKE.bat` (runs `bats\_run_smoke.ps1`)

Regenerate (after editing `_gen_all.ps1`):

```bat
powershell -ExecutionPolicy Bypass -File bats\_gen_all.ps1
```

## Important rules

- Build first: `bats\00_build\build_cpu.bat` (and `build_cuda.bat` for GPU).
- Do **not** use `-e` with `-U cuda` / `-U both`.
- `-e` does **not** help small puzzle `-b` ranges; omit it for puzzles.
- Bare `-R` = random search. Research uses `--submode NAME` (or `-R NAME` only if NAME is a real submode).
- `-U both` = hybrid CPU+CUDA threads. `-l both` = compress+uncompress.
- Coins without fixtures (`ltc`, `doge`, `xrp`, …): create `targets_*.txt` then run the bat.

Full flag docs: [../README.md](../README.md) / [../docs/COMMANDS.md](../docs/COMMANDS.md)
