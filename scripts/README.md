# Scripts

TrueCollider helpers that live outside the core C++/CUDA tree. Run from anywhere; most tools `cd` to the repo root.

## Target conversion (custom CUDA edition, MIT)

KeyHunt-Cuda–style packed inputs, written from scratch (no GPL code).

| Script | Input | Output | Use with |
|--------|-------|--------|----------|
| `addr_to_hash160.py` | BTC addresses / 40-hex | `.hash160` / `.bin` (20 B) | `-m address` / `-m rmd160` |
| `eth_to_bin.py` | `0x` ETH addresses | `.keccak160` / `.bin` (20 B) | `-m address -c eth` |
| `pub_to_xpoint.py` | compressed/uncompressed pubs | `.xpoint.bin` (32 B) | `-m xpoint` (text still works) |
| `binsort.py` | packed bin | sorted packed bin | optional |

```bat
python scripts\addr_to_hash160.py tests\66.txt -o tests\66.hash160 --sort
keyhunt_cuda.exe -m address -f tests\66.hash160 -b 66 -l compress -U cuda -M auto -t 1 -x sequential -q
```

Text `-f` files still work (with `#` comments skipped). Binary packs skip the slow text decode step for huge lists.

## Build wrappers (Unix / cross)

| Script | Purpose |
|--------|---------|
| `build_linux.sh` | Native Linux `make` |
| `build_windows.sh` | MinGW cross → `keyhunt.exe` |
| `build_windows_cmake.sh` | CMake MinGW cross |
| `build_mingw_native.sh` | Native MinGW via make |
| `build_termux.sh` | Termux / Android |

Windows native builds: `bats/00_build/`.

## Audits / benches

| Script | Purpose |
|--------|---------|
| `run_smoke_tests.ps1` | Quick smoke |
| `run_deep_audit.ps1` | Full known-hit matrix → `docs/DEEP_AUDIT.md` |
| `run_final_audit.ps1` | Final audit pass |
| `run_mode_tests.ps1` | Mode matrix |
| `run_benchmarks.ps1` | Speed bench → `docs/SPEEDS.md` |

## Puzzle research helpers

| Script | Purpose |
|--------|---------|
| `puzzle_scan.py` | Blockchain funding scan |
| `wallet_search.py` | Solved-key pattern analysis |
| `bip32_search.py` | BIP-32 path tests |
