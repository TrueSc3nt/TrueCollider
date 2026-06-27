# Bitcoin Puzzle Deterministic Wallet Search
## Puzzles 72-160

### Overview

This toolkit implements a search strategy for Bitcoin puzzles 72-160 based on analyzing the deterministic wallet pattern used by the puzzle creator.

### Key Findings

**Blockchain Analysis:**
- **Funding Transaction:** `08389f34c98c606322740c0be6a7125d9860bb8d5cb182c02f98461e5fa6cd15`
- **Block Height:** 339085
- **Timestamp:** 1421345234 (January 15, 2015 18:07:14 UTC)
- **256 puzzle outputs** funded in a single transaction

**Solved Puzzles (as of 2025):**
- Puzzles 1-39: All solved
- Puzzle 66: Solved September 2024 (6.6 BTC prize)
- Puzzle 67: Solved February 2025 (6.7 BTC prize)
- Puzzle 68: Solved April 2025 (6.8 BTC prize)
- Puzzle 69: Solved April 2025 (6.9 BTC prize)

**Deterministic Wallet Pattern:**
The puzzle creator used a BIP-32 deterministic wallet to generate consecutive keys from a single master seed. Key observations:

1. Keys are consecutive BIP-32 derivations
2. No simple mathematical formula between keys
3. If you find ONE key, you can derive all others
4. Puzzles 40-65, 70, and 71-160 remain unsolved (with puzzles 75, 80, 85, 90, 95, 100, 105, 110, 115, 120, 125, 130 having prizes)

### Tools Created

1. **`puzzle_scan.py`** - Fetches blockchain data and analyzes puzzle funding
2. **`wallet_search.py`** - Analyzes solved puzzle keys and generates search commands
3. **`bip32_search.py`** - BIP-32 derivation path testing and pattern analysis

### Quick Start

```bash
# Run blockchain scan
python puzzle_scan.py

# Analyze key patterns
python wallet_search.py

# Generate and run batch search
python bip32_search.py
bash run_puzzle_search.sh
```

### Search Strategies

**1. Direct Address Search (fastest for puzzles < 80 bits):**
```bash
keyhunt -m address -f tests/unsolvedpuzzles.txt -b 72 -t 8 -x auto
```

**2. Timestamp-Constrained Search:**
```bash
keyhunt -m address -f targets.txt -T 1421345234 -b 72 -t 8
```
Uses the puzzle funding timestamp (Jan 15 2015) to narrow search window.

**3. BSGS Mode (for puzzles with public keys):**
```bash
keyhunt -m bsgs -f tests/PUZZLE_NUM.txt -B sequential -n 0x1000000 -t 4
```

**4. Combined Strategy:**
```bash
keyhunt -m address -f tests/unsolvedpuzzles.txt -b 72 -T 1421345234 -t 8 -x auto
```

### Puzzle Difficulty Guide

| Bit Range | Estimated Time | Strategy |
|-----------|----------------|----------|
| 72-80 | Minutes to hours | Address mode, direct search |
| 81-96 | Hours to days | Address + timestamp constraint |
| 97-112 | Days to weeks | BSGS if public key available |
| 113-128 | Weeks to months | BSGS + endomorphism |
| 129-160 | Months+ | Advanced techniques needed |

### Derivation Path Testing

If you find one key, test these paths to find the master seed:

```
m/0'/0'/0'
m/44'/0'/0'/0/0
m/0/0/0
```

Then derive all puzzle keys from the same master seed.

### Important Notes

1. **Start with puzzle 72** - fastest to search (72-bit range)
2. **Use timestamp** - narrows search to keys generated around Jan 15 2015
3. **Find one key** - if you solve ANY puzzle, you can potentially derive all others
4. **Check public keys** - some puzzles have public keys available for BSGS mode

### Files

- `puzzle_40_160_data.json` - Puzzle addresses and metadata (puzzles 40-160)
- `tests/unsolved_puzzles_71_160.txt` - Addresses for puzzles 71-160
- `run_puzzle_search.sh` - Batch search script
- `tests/derivation-paths/puzzle_paths.txt` - BIP-32 paths to test
- `tests/unsolvedpuzzles.txt` - All unsolved puzzle addresses
