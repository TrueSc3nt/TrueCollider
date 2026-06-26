# Keyhunt with Collider Search Modes

## Overview

This version of keyhunt includes the search modes from Collider v2.0.0, adapted for CPU-only usage. These modes provide different search patterns that can improve key discovery rates depending on the target key distribution.

## New Search Modes

### Available Modes (`-x` flag)

| Mode | Description |
|------|-------------|
| `sequential` | Traditional sequential search (default keyhunt behavior) |
| `random` | Random search within range (default for -R flag) |
| `chaos` | Logistic map chaotic sequence for ergodic coverage |
| `gravity` | Adaptive search focusing around found keys |
| `spiral` | Archimedean spiral from range midpoint outward |
| `reverse` | Inverted BSGS baby/giant step roles |
| `auto` | Intelligent cycling through all modes |

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
Combines all four novel modes in a smart rotation. Cycles through phases every 200-300 iterations:
- Phase 1: SPIRAL (200 cycles) - Center-out coverage
- Phase 2: CHAOS (300 cycles) - Ergodic coverage across full range
- Phase 3: GRAVITY (200 cycles) - Adaptive focusing
- Phase 4: REVERSE (300 cycles) - Different collision patterns

## Usage Examples

### Basic Usage with Search Mode

```bash
# Chaos mode with address search
./keyhunt -m address -f tests/66.txt -b 66 -l compress -x chaos -q -t 8

# Gravity mode with rmd160 search
./keyhunt -m rmd160 -f tests/unsolvedpuzzles.rmd -b 66 -l compress -x gravity -q -t 8

# Spiral mode with xpoint search
./keyhunt -m xpoint -f tests/substracted40.txt -n 65536 -t 4 -b 40 -x spiral

# Auto mode with BSGS search
./keyhunt -m bsgs -f tests/135.txt -b 135 -x auto -q -s 10 -R

TrueCollider Search Modes + Binary Fuse Filters, developed & Modified by TrueScent
```

### Combined with Other Flags

```bash
# Chaos mode with endomorphism
./keyhunt -m address -f tests/puzzleswithout.txt -b  -l compress -x chaos -e -q -t 8

# Auto mode with saving bloom filters
./keyhunt -m bsgs -f tests/135.txt -b 135 -x auto -s 10 -R -S -k 128 -n 0x1000000000000

# Gravity mode with specific range
./keyhunt -m address -f tests/66.txt -r 20000000000000000:40000000000000000 -l compress -x gravity -q -t 8
```

## How It Works

### Search Pattern Implementation

The search modes work by modifying how the next starting key is selected for each batch of keys processed:

1. **Sequential**: Linear progression through the range
2. **Random**: Cryptographically random selection within range
3. **Chaos**: Logistic map generates deterministic but aperiodic sequence
4. **Gravity**: Biased random selection around previously found keys
5. **Spiral**: Mathematical spiral from center outward
6. **Reverse**: Uses different BSGS step ordering
7. **Auto**: Cycles through spiral → chaos → gravity → reverse

### Gravity Mode Adaptation

When gravity mode finds a key:
1. The search center shifts to the found key location
2. 70% of subsequent searches focus within ±1024 of the center
3. 30% of searches remain random for exploration
4. This creates a feedback loop that intensifies search around promising areas

## Compilation

### Windows (WSL - Recommended)

The binary is pre-compiled for WSL. To run:

```bash
# From WSL terminal:
cd "/mnt/c/Users/adam/OneDrive/Desktop/New folder/New folder/keyhunt-main"
./keyhunt -m address -f tests/66.txt -b 66 -l compress -x chaos -q -t 8
```

Or use the provided batch file:
```cmd
run_keyhunt.bat -m address -f tests/66.txt -b 66 -l compress -x chaos -q -t 8
```

### Recompile from source

```bash
# In WSL:
cd "/mnt/c/Users/adam/OneDrive/Desktop/New folder/New folder/keyhunt-main"
make
```

The binary is named `keyhunt` (Linux) and can be run from WSL on Windows.

## Technical Details

### Search Mode Constants

```c
#define SEARCHMODE_SEQUENTIAL 0
#define SEARCHMODE_RANDOM 1
#define SEARCHMODE_CHAOS 2
#define SEARCHMODE_GRAVITY 3
#define SEARCHMODE_SPIRAL 4
#define SEARCHMODE_REVERSE 5
#define SEARCHMODE_AUTO 6
```

### Global Variables

```c
int FLAGSEARCHMODE = SEARCHMODE_RANDOM;
double chaos_x = 0.1;
const double chaos_r = 3.99999;
Int gravity_center;
int gravity_found_count = 0;
Int spiral_center;
double spiral_angle = 0.0;
const double spiral_step = 0.1;
int auto_phase = 0;
int auto_cycles = 0;
const int AUTO_PHASE_CYCLES[4] = {200, 300, 200, 300};
```

### Key Functions

- `init_search_mode()` - Initializes search mode parameters
- `get_next_key_chaos()` - Generates next key using logistic map
- `get_next_key_gravity()` - Generates next key biased around found keys
- `get_next_key_spiral()` - Generates next key using spiral pattern
- `get_next_key_auto()` - Cycles through all modes
- `get_next_search_key()` - Main dispatcher for search modes
- `notify_key_found()` - Updates gravity mode when key is found

## Performance Notes

- **Chaos mode**: Minimal overhead, similar to random mode performance
- **Gravity mode**: Slight overhead for bias calculation, but can significantly speed up searches when keys cluster
- **Spiral mode**: Minimal overhead, good for keys near range midpoints
- **Auto mode**: Best overall choice for unknown key distributions

## Differences from Collider GPU Version

This CPU implementation differs from the GPU-based Collider:

1. **No GPU acceleration**: All computations are CPU-based
2. **Simplified BSGS**: Uses keyhunt's existing BSGS implementation
3. **Single-threaded search patterns**: Each thread independently follows the search pattern
4. **Compatible with all modes**: Works with address, rmd160, xpoint, bsgs, vanity, minikeys

## Credits

- Original keyhunt by AlbertoBSD (albertobsd@gmail.com)
- Search modes inspired by Collider v2.0.0
- Adapted for CPU-only usage

## License

See LICENSE file for details.
