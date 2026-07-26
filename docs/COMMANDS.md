# Command cookbook

Copy, paste, and change file names / bit ranges. On Windows use `keyhunt.exe` / `keyhunt_cuda.exe`, or the helpers under [`examples/`](../examples/).

Full exhaustive reference: **[README.md](../README.md)**. Raw built-in help: [`HELP_DUMP.txt`](HELP_DUMP.txt) (`keyhunt.exe -h`).

---

## Cheat sheet (every common flag)

| Flag | Example | Meaning |
|------|---------|---------|
| `-m` | `-m address` | Mode: address, rmd160, xpoint, bsgs, kangaroo, vanity, minikeys, mnemonic, poetry, brainwallet, pubkey2addr |
| `-f` | `-f tests/66.txt` | Target file |
| `-c` | `-c btc` / `eth` / `sol` / `troot` / `all` / `auto` | Currency |
| `-t` | `-t 8` | CPU threads |
| `-b` / `-r` | `-b 66` / `-r 1:ffff` | Bit or hex range |
| `-T` | `-T 1421345234` | Timestamp → ~4B key window |
| `-Z` | `-b 72 -Z 6` | Strip leading zero bytes (needs `-b`) |
| `-l` | `-l compress` | compress / uncompress / both |
| `-e` | `-e` | GLV endomorphism (CPU secp) |
| `-A` | `-A auto` | Vector: auto / none / sse / avx / avx2 / avx512 |
| `-x` | `-x chaos` / `-x keyhole` | sequential / random / rseq / chaos / gravity / spiral / reverse / auto / hilbert / sobol / halton / density-map / **keyhole** / **pocket** / **afterimage** / **driftcompass** / **twinflame** / **breadcrumb** / **clockwork** / **lottery** / **wave** (alias `waveroulette`) |
| `-rs` | `-rs` | Random-sequential (alias `-x rseq`): random start, walk N, reseed. Default N=1M |
| `--window-bits` | `--window-bits 40` | Keyhole window width (bits) |
| `--pocket-bits` | `--pocket-bits 16` | PocketRadar buckets = 2^P |
| `--antiloop-dist` | `--antiloop-dist 12` | Afterimage min XOR-popcount distance |
| `--shadow-mod` | `--shadow-mod 16` | ShadowLedger fiber modulus (`-B shadowledger`) |
| `--mod-step` / `--mod-rem` | `--mod-step 16 --mod-rem 3` | Residue snap for BSGS / gaudry / kangaroo-mod |
| `-U` | `-U cuda` | GPU: none / cuda / opencl |
| `-G` | `-G 8192` | GPU batch hint (keys) |
| `-M` | `-M auto` / `-M 2048` / `-M 2G` | Memory budget (VRAM/RAM); `-M matrix` = screen |
| `-k` | `-k 512` / `-k auto` | BSGS K factor |
| `-n` | `-n 0x100000000000` | BSGS N (≥ 2^20, exact sqrt) / sequential cycle size |
| `-B` | `-B modfan` | BSGS: sequential / backward / both / random / dance / rseq / residue / **modfan** / **shadowledger** / **hybrid** / freeze-table / compact-dp / dual-range (+ research names) |
| `-S` | `-S` | Save/load BSGS tables |
| `-z` | `-z 2` | Bloom size multiplier |
| `-R` | `-R` | Random / BSGS random convenience |
| `-rs` | `-rs` | Random-sequential (Mivvvy-style chunk walk) |
| `-I` | `-I 2` | Stride (address / rmd160 / xpoint) |
| `-y` | `-y` | Dry-run config and exit |
| `-v` | `-v 1Cool` | Vanity prefix |
| `-C` / `-8` | minikeys | Base string / custom alphabet |
| `-w` / `-L` / `-W` / `-D` | mnemonic | Words / language / ETH / indices |
| `-p` / `-D` | BIP-32 | Path + child index count |
| `-N` / `-Nurl` | balance | **Wired** — curl balance check on hits (see README) |
| `-q` `-s` | `-q -s 10` | Quiet / stats seconds |
| `-V` | `-V` | Verbose derivation |
| `-6` | `-6` | Skip cache checksum |
| `-d` | `-d` | Debug |

---

## CPU recipes

```bash
./keyhunt -m address -f tests/66.txt -b 66 -l compress -e -A auto -t 8 -q -s 10
./keyhunt -m address -f tests/_btc_1to2.txt -r 1:1000 -rs -n 0x400 -t 2 -s 5
./keyhunt_cuda -m address -f tests/_btc_1to2.txt -r 1:1000 -rs -n 0x400 -U cuda -M auto -t 1 -s 5
./keyhunt -m address -c eth -f tests/_eth_1.txt -t 8 -q -s 10
./keyhunt -m address -c sol -f tests/sol_sample.txt -t 8
./keyhunt -m vanity -v 1Cool -e -t 8
./keyhunt -m kangaroo -f tests/_pubkey_g.txt -r 1:1000
./keyhunt_cuda -m kangaroo -f tests/_pubkey_g.txt -r 1:1000 -U cuda
./keyhunt -m mnemonic -f tests/66.txt -w 12 -t 4
./keyhunt -m poetry -f tests/66.txt -t 4
./keyhunt -m brainwallet -w 3 -f tests/66.txt -t 4
./keyhunt -m minikeys -f tests/66.txt -t 4
./keyhunt -m xpoint -f tests/_xpoint_g.txt -t 8
./keyhunt -m pubkey2addr -f tests/66.txt -x auto -t 4
./keyhunt -m rmd160 -f tests/66.rmd -l compress -e -t 8
```

### BSGS (`-n` / `-k`)

```bash
./keyhunt -m bsgs -f tests/125.txt -b 125 -R -k 512 -t 8 -S -q -s 10
./keyhunt -m bsgs -f tests/125.txt -b 125 -k auto -y
./keyhunt -m bsgs -f pubkey.txt -b 135 -B modfan --mod-step 16 -t 8 -S
./keyhunt -m bsgs -f pubkey.txt -b 135 -B hybrid -t 8
./keyhunt -m bsgs -f pubkey.txt -B shadowledger --shadow-mod 32 -t 8
```

- `-n` ≥ `1048576` (2^20), exact square root required  
- Prefer `-k` power of 2; use `-k auto` for RAM-based pick  
- Full bits→N→kmax and RAM→k tables: **[README.md](../README.md)** (BSGS section)
- **Honesty:** BSGS is discrete-log with a baby table — **not** Pollard kangaroo √N. `-B freeze-table` / `compact-dp` print hygiene tips (`-S`, `-M`/`-k`); they do not invent a new algorithm.
- **Residue / ModFan:** `--mod-step M` + `--mod-rem R` (or `-B modfan` → rem = tid % M) snaps giant starts onto a residue class. Useful for sharding herds, not a complexity miracle.

### Puzzle walks (address / rmd160, bits 72–160)

```bash
./keyhunt -m address -f targets.txt -b 72 -l compress -t 8 -x keyhole --window-bits 40 -q
./keyhunt -m rmd160 -f hashes.rmd -b 80 -l compress -t 8 -x pocket --pocket-bits 16 -q
./keyhunt -m address -f targets.txt -b 66 -x afterimage --antiloop-dist 12 -t 8
./keyhunt -m address -f targets.txt -b 75 -x sobol -t 8
./keyhunt -m address -f targets.txt -b 90 -x wave -t 8
```

- Address/rmd160 search is **hash160 matching** in a bit range — **not** ECDLP kangaroo. Walk modes only change *where* bases are drawn.
- Do **not** use `-e` on small puzzle `-b` windows; endomorphism rarely helps there.
- Pubkey-known puzzles → use `-m bsgs` / `-m kangaroo`, not address walk modes.

### Attribution (ideas vs code)

Walk / BSGS helper **names and behaviors** (Keyhole, PocketRadar, Afterimage, DriftCompass, TwinFlame, Breadcrumb, Clockwork, LotteryHerd, WaveRoulette, ModFan, ShadowLedger, HybridBSGS, …) are inspired by public write-ups and the RCKangaroo-Puzzle135 **MODES_GUIDE / echomodes** documentation by RetiredCoder and contributors.

TrueCollider ships **independent MIT-licensed reimplementations** of those *behaviors*. **No GPLv3 source was copied** from RCKangaroo. If you need the original kangaroo engine, use that project under its license; do not mix the trees.

Bats: [`bats/12_patterns/`](../bats/12_patterns/), [`bats/04_bsgs/`](../bats/04_bsgs/), [`bats/17_puzzles/`](../bats/17_puzzles/).

---

## GPU recipes

```bash
keyhunt_cuda.exe -m address -f tests/66.txt -b 66 -l compress -U cuda -M auto -t 1 -q -s 5
keyhunt_cuda.exe -m rmd160 -f tests/66.rmd -U cuda -M 2048 -t 1 -q -s 5
keyhunt_cuda.exe -m address -c eth -f tests/_eth_1.txt -U cuda -M auto -t 1
keyhunt_cuda.exe -m vanity -v 1Love -U cuda -M auto -t 1
keyhunt_cuda.exe -m bsgs -f tests/125.txt -b 125 -k auto -U cuda -M auto -t 4 -S
keyhunt_cuda.exe -m address -c sol -f tests/sol_sample.txt -r 1:8 -U cuda -M auto -t 1
keyhunt_cuda.exe -m address -f tests/66.txt -U cuda -M auto -y
```

**Solana:** full device ed25519 `ge_scalarmult_base` when self-test passes (else CUDA SHA512 + host ge). BSGS: device GRP giant-step + host bloom (serial cycles today); baby table still GPU-assisted.

Hits → `FOUND_BTC.txt` / `FOUND_ETH.txt` / `FOUND_SOL.txt` + `KEYFOUNDKEYFOUND.txt`.

More: [README.md](../README.md) · [gpu/README.md](../gpu/README.md) · [SPEEDS.md](SPEEDS.md) · [examples/](../examples/).
