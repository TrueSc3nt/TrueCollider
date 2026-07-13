# CPU vectorization benchmark

TrueCollider supports multiple CPU hash160 backends selected with `-A`:

| Flag | Behavior |
|------|----------|
| `-A auto` | Best level supported by CPU/OS (default) |
| `-A sse` | 4-wide SSE SHA-256 + RIPEMD-160 |
| `-A avx512` | 16-wide AVX-512 batch (falls back to SSE if unsupported) |
| `-A none` | Scalar reference hashes |

## Quick benchmark (Linux / WSL)

```bash
cmake -B build && cmake --build build -j$(nproc)
chmod +x scripts/bench_vector.sh
./scripts/bench_vector.sh ./build/keyhunt 20
```

The script runs address mode for ~20 seconds per vector level on a fixed key range and prints `time(1)` stats.

## Manual benchmark (Windows)

```powershell
# Build via WSL or MinGW cross-compile first
.\keyhunt.exe -m address -f targets.txt -r 1:1000000000000 -A sse -t 4 -q
.\keyhunt.exe -m address -f targets.txt -r 1:1000000000000 -A auto -t 4 -q
.\keyhunt.exe -m address -f targets.txt -r 1:1000000000000 -A avx512 -t 4 -q
```

Compare keys/sec reported in the status output (or wall-clock time for the same range).

## What to expect

- **AVX-512 CPU (Xeon, some i9/i7):** `-A avx512` or `-A auto` — up to ~4× faster hash160 vs SSE on the address/rmd160/vanity loops (no endomorphism).
- **Typical desktop/laptop:** `-A auto` selects SSE or AVX2; `-A avx512` warns and falls back to SSE.
- **Endomorphism enabled (`-e`):** stays on the 4-wide SSE path even with `-A avx512`.

## CUDA (experimental)

Build with CUDA toolkit:

```bash
cmake -B build-cuda -DENABLE_CUDA=ON
cmake --build build-cuda -j$(nproc)
./build-cuda/keyhunt -U cuda -h
```

On startup, CUDA runs a hash160 self-test. Full GPU search (EC + filter on device) is not yet wired into the main loop; phase 1 validates batch hash160 on the GPU.

## P2SH targets

If the target file contains `3…` addresses, TrueCollider sets an internal flag and also computes P2SH script hashes (small extra cost). Mixed `1…` / `3…` / `bc1q…` files work without extra flags.
