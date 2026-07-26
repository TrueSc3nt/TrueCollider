# Keyhunt / TrueCollider Search Modes (`-x`)

## Overview

TrueCollider includes Collider-style search patterns plus newer host-planned walk modes (Keyhole, Pocket, Afterimage, Wave, …). These modes change **where** private-key bases are drawn inside `-r` / `-b`. They do **not** turn address/hash160 search into Pollard kangaroo √N.

For BSGS giant-step helpers (`-B modfan`, `shadowledger`, `hybrid`, …) see [`docs/COMMANDS.md`](docs/COMMANDS.md) and [`README.md`](README.md#whats-new-walk--bsgs-helpers).

Full bats: [`bats/12_patterns/`](bats/12_patterns/), [`bats/17_puzzles/`](bats/17_puzzles/).

## Available Modes (`-x` flag)

### Classic Collider patterns

| Mode | Description |
|------|-------------|
| `sequential` | Traditional sequential search (default keyhunt behavior) |
| `random` | Random search within range (also `-R`) |
| `rseq` | Random-sequential chunk walk (alias `-rs`; default `-n` = 1M) |
| `chaos` | Logistic map chaotic sequence for ergodic coverage |
| `gravity` | Adaptive search focusing around found keys |
| `spiral` | Archimedean spiral from range midpoint outward |
| `reverse` | Inverted BSGS baby/giant step roles |
| `auto` | Cycle spiral → chaos → gravity → reverse |
| `hilbert` / `sobol` / `halton` / `density-map` | Low-discrepancy / density planners |

### Walk planners (host-planned)

| Mode | Description | Flags |
|------|-------------|-------|
| `keyhole` | Time-share random 2^W windows | `--window-bits W` (default 40) |
| `pocket` | Coarse 2^P pockets + Sobol visit | `--pocket-bits P` (default 16) |
| `afterimage` | Antiloop reseeds (XOR-popcount distance) | `--antiloop-dist D` (default 12) |
| `driftcompass` | Drift + compass-style base planning | — |
| `twinflame` | Paired / mirrored walk planning | — |
| `breadcrumb` | Leave-and-return style reseeds | — |
| `clockwork` | Periodic / structured stride planning | — |
| `lottery` | Short random waves then reseed | often with `-n` |
| `wave` | Alternate low/high halves | alias: `waveroulette` |

**CPU vs CUDA:** walk planners are host-planned. Prefer CPU for new `-x` modes. Do **not** use `-e` with `-U cuda` / `-U both`.

**Honesty:** address/rmd160 is hash160 matching in a bit range — walk modes only replan bases. Pubkey-known puzzles → `-m bsgs` / `-m kangaroo`.

### Mode Descriptions

#### Chaos Mode
Uses the logistic map equation `x(n+1) = r * x(n) * (1 - x(n))` with `r = 3.99999` (fully chaotic regime) to generate search starting points. Unlike random mode which has clustering bias, chaotic sequences are deterministic but aperiodic - they provably fill the entire key space with mathematically optimal ergodic coverage.

#### Gravity Mode
When a key is found, the search center "gravitates" toward that region. New keys are generated as a 70/30 mix of random values biased toward the last successful region. This creates an adaptive system that naturally concentrates compute resources where partial information exists.

#### Spiral Mode
Searches outward from the midpoint of the range in an Archimedean spiral pattern (`r = a * theta`). Statistically, keys generated from common entropy sources (timestamps, counters, weak RNGs) tend to cluster near "round" numbers or midpoints. Spiral search covers these high-probability areas first while still guaranteeing full coverage.

#### Reverse Mode
Instead of the standard BSGS approach (baby steps from G, giant steps from target P), this reverses the roles. Mathematically equivalent but practically different - it creates entirely different hash table collision patterns.

#### Auto Mode
Combines spiral / chaos / gravity / reverse in a smart rotation. Cycles through phases every 200-300 iterations:
- Phase 1: SPIRAL (200 cycles) - Center-out coverage
- Phase 2: CHAOS (300 cycles) - Ergodic coverage across full range
- Phase 3: GRAVITY (200 cycles) - Adaptive focusing
- Phase 4: REVERSE (300 cycles) - Different collision patterns

#### Keyhole / Pocket / Afterimage / Wave
- **Keyhole** — pick a random window of width `2^W` inside the bit range and grind it before jumping.
- **Pocket** — partition into `2^P` pockets; visit with a Sobol-like order.
- **Afterimage** — reject reseeds that are too close in XOR-popcount distance (`--antiloop-dist`).
- **Wave / WaveRoulette** — alternate searching the low and high halves of the active range.

## Usage Examples

```bash
# Chaos mode with address search
./keyhunt -m address -f tests/66.txt -b 66 -l compress -x chaos -q -t 8

# Gravity mode with rmd160 search
./keyhunt -m rmd160 -f tests/unsolvedpuzzles.rmd -b 66 -l compress -x gravity -q -t 8

# Spiral mode with xpoint search
./keyhunt -m xpoint -f tests/_xpoint_g.txt -n 65536 -t 4 -b 40 -x spiral

# Auto mode with BSGS search
./keyhunt -m bsgs -f tests/125.txt -b 125 -x auto -q -s 10 -R

# New walk modes (puzzle-style)
./keyhunt -m address -f tests/66.txt -b 72 -l compress -t 8 -x keyhole --window-bits 40 -q
./keyhunt -m rmd160 -f tests/66.rmd -b 66 -l compress -t 8 -x pocket --pocket-bits 16 -q
./keyhunt -m address -f tests/66.txt -b 66 -x afterimage --antiloop-dist 12 -t 8
./keyhunt -m address -f tests/66.txt -b 75 -x wave -t 8

# Windows bats
bats\12_patterns\x_keyhole.bat
bats\17_puzzles\b20_pocket.bat
```

### Combined with Other Flags

```bash
# Chaos mode with endomorphism (CPU secp only — not with -U cuda)
./keyhunt -m address -f tests/66.txt -b 66 -l compress -x chaos -e -q -t 8

# Auto mode with saving bloom filters (BSGS)
./keyhunt -m bsgs -f tests/125.txt -b 125 -x auto -s 10 -R -S -k 128

# Gravity mode with specific range
./keyhunt -m address -f tests/66.txt -r 20000000000000000:40000000000000000 -l compress -x gravity -q -t 8
```

## How It Works

The search modes work by modifying how the next starting key is selected for each batch of keys processed:

1. **Sequential**: Linear progression through the range
2. **Random**: Cryptographically random selection within range
3. **Chaos**: Logistic map generates deterministic but aperiodic sequence
4. **Gravity**: Biased random selection around previously found keys
5. **Spiral**: Mathematical spiral from center outward
6. **Reverse**: Uses different BSGS step ordering
7. **Auto**: Cycles through spiral → chaos → gravity → reverse
8. **Walk planners**: Host picks windows / pockets / halves / antiloop reseeds before the EC+hash batch

### Gravity Mode Adaptation

When gravity mode finds a key:
1. The search center shifts to the found key location
2. 70% of subsequent searches focus within ±1024 of the center
3. 30% of searches remain random for exploration
4. This creates a feedback loop that intensifies search around promising areas

## Compilation

### Windows

```bat
bats\00_build\build_cpu.bat
REM or: bats\00_build\build_mingw_native.bat
```

### Linux / WSL

```bash
make -j$(nproc)
./keyhunt -m address -f tests/66.txt -b 66 -l compress -x chaos -q -t 8
```

## Attribution

Walk / BSGS helper **names and behaviors** (Keyhole, PocketRadar, Afterimage, DriftCompass, TwinFlame, Breadcrumb, Clockwork, LotteryHerd, WaveRoulette, ModFan, ShadowLedger, HybridBSGS, …) are inspired by public write-ups and the RCKangaroo-Puzzle135 **MODES_GUIDE / echomodes** documentation by RetiredCoder and contributors.

TrueCollider ships **independent MIT-licensed reimplementations** of those *behaviors*. **No GPLv3 source was copied** from RCKangaroo.

## Credits

- Original keyhunt by AlbertoBSD
- Classic search modes inspired by Collider v2.0.0
- Walk / BSGS helper behaviors inspired by RCKangaroo public docs (independent reimplementation)
- TrueCollider / KeyCollider by TrueScent

## License

See LICENSE file for details.
