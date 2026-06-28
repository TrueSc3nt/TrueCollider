# TrueCollider

**Multi-Currency Search + Binary Fuse Filters** — A high-performance cryptocurrency key search tool.

Developed & Modified by **TrueScent**

---

## Features

- **11 Search Modes** — address, pubkey2addr, rmd160, xpoint, bsgs, vanity, minikeys, mnemonic, poetry, brainwallet
- **10+ Cryptocurrencies** — BTC, ETH, LTC, DOGE, XRP, SOL, BCH, BTG, ETC, Taproot
- **Multi-Currency Simultaneous** — search all currencies at once with `-c all`
- **Binary Fuse Filters** — 30-40% faster lookups, 30-40% less memory than traditional bloom filters
- **7 Search Patterns** — sequential, random, chaos, gravity, spiral, reverse, auto
- **Multi-threaded** — full multi-thread support across all modes
- **BSGS Algorithm** — Baby-Step Giant-Step for solving crypto puzzles
- **BIP-39 Mnemonics** — 10 languages, BIP-44/49/84 derivation paths
- **Taproot (P2TR)** — search for Taproot bc1p... addresses with `-c troot`
- **Custom Derivation Paths** — use `-p` for BIP-32 derivation in address/rmd160 modes
- **Node Balance Check** — check balances via API when keys are found with `-N`
- **Endomorphism** — GLV endomorphism for 3x speedup in address/rmd160/vanity modes
- **Timestamp Search** — narrow search ranges by creation time

---

## Download and Build

This program was made in a linux environment. If you are a Windows user I strongly recommend using WSL with Ubuntu. It is available in the Microsoft Store.

Please install on your system:
- git
- build-essential

On Debian based systems, run these commands to update your environment and install the tools needed to compile:

```bash
sudo apt update && sudo apt upgrade
sudo apt install git -y
sudo apt install build-essential -y
```

To clone the repository:

```bash
git clone https://github.com/TrueSc3nt/TrueCollider.git
cd TrueCollider
make
```

Then execute with `-h` to see the help:

```bash
./keyhunt -h
```

---

## Disclaimer

This tool is made as a generic tool for puzzles. I recommend everyone to stay in puzzles.

---

## Quick Start

```bash
# Download and build
git clone https://github.com/TrueSc3nt/TrueCollider.git
cd TrueCollider
make

# Run help
./keyhunt -h

# Solve puzzle 66 (address mode)
./keyhunt -m address -f tests/66.rmd -b 66 -l compress -t 8

# Solve puzzle 125 (BSGS mode)
./keyhunt -m bsgs -f tests/125.txt -b 125 -q -s 10 -R
```

---

## Search Modes (`-m`)

### address

Search for private keys that produce addresses in the target file.

**Supported Cryptocurrencies:**
- BTC (Bitcoin) — 1.../3... addresses
- ETH (Ethereum) — 0x... addresses
- LTC (Litecoin) — L... addresses
- DOGE (Dogecoin) — D... addresses
- XRP (Ripple) — r... addresses
- BCH (Bitcoin Cash) — q.../p... addresses
- BTG (Bitcoin Gold) — G... addresses
- ETC (Ethereum Classic) — 0x... addresses
- TROOT (Taproot) — bc1p... addresses
- ALL — Search all currencies simultaneously

**Input file format:** One address per line (format depends on currency).

**How it works:**
1. Load target addresses into binary fuse filter + sorted array
2. Each thread generates candidate private keys
3. Derives public key → address (SHA256 + RIPEMD160 for BTC/LTC/DOGE/XRP, keccak256 for ETH/ETC)
4. Checks address against filter (fast rejection) then binary search
5. Found key is written to `KEYFOUNDKEYFOUND.txt`

**Custom derivation paths** (`-p`):
- Each random key is used as a BIP-32 master key
- Child keys are derived along the specified path for indices 0 to `-D-1`
- Supports hardened: `'` or `H` or `h` suffix (e.g. `44'`)
- Use `-V` for verbose output showing full derivation path and chain code

**Node balance checking** (`-N`):
- When a key is found, checks balance against blockchain API
- Requires curl to be installed
- Supports BTC, ETH, LTC, ETC

**Examples:**

```bash
# Search BTC addresses with 8 threads
./keyhunt -m address -f targets.txt -t 8

# Search ETH addresses
./keyhunt -m address -c eth -f eth_targets.txt -t 8

# Search Litecoin addresses
./keyhunt -m address -c ltc -f ltc_targets.txt -t 8

# Search Dogecoin addresses
./keyhunt -m address -c doge -f doge_targets.txt -t 8

# Search XRP addresses
./keyhunt -m address -c xrp -f xrp_targets.txt -t 8

# Search Bitcoin Cash addresses
./keyhunt -m address -c bch -f bch_targets.txt -t 8

# Search Bitcoin Gold addresses
./keyhunt -m address -c btg -f btg_targets.txt -t 8

# Search Ethereum Classic addresses
./keyhunt -m address -c etc -f etc_targets.txt -t 8

# Search Taproot (bc1p...) addresses
./keyhunt -m address -c troot -f troot_targets.txt -t 8

# Search ALL currencies simultaneously
./keyhunt -m address -c all -f all_targets.txt -t 8

# Search with custom derivation path (BIP-86 taproot)
./keyhunt -m address -c troot -p "m/86'/0'/0'/0" -D 10 -f troot_targets.txt -V -t 8

# Search with node balance checking
./keyhunt -m address -c btc -f targets.txt -N -t 8

# Search with specific range
./keyhunt -m address -f targets.txt -r 1:FFFFFFFF -t 8

# Search with custom derivation path (BIP-86 taproot)
./keyhunt -m address -c troot -p "m/86'/0'/0'/0" -D 10 -f troot_targets.txt -V -t 8

# Search with custom derivation path (BIP-84 native segwit)
./keyhunt -m address -p "m/84'/0'/0'/0" -D 20 -f targets.txt -V -t 8

# Search with custom derivation path (BIP-44 legacy)
./keyhunt -m address -p "m/44'/0'/0'/0" -D 20 -f targets.txt -V -t 8

# Search with specific range
./keyhunt -m address -f targets.txt -r 1:FFFFFFFF -t 8

# Search with bit range (puzzle 66)
./keyhunt -m address -f tests/66.rmd -b 66 -l compress -t 8

# Search with custom derivation path (BIP-86 taproot)
./keyhunt -m address -c troot -p "m/86'/0'/0'/0" -D 10 -f troot_targets.txt -t 8

# Search with custom derivation path (BIP-44)
./keyhunt -m address -p "m/44'/0'/0'/0" -D 20 -f targets.txt -t 8

# Search with chaos pattern
./keyhunt -m address -f targets.txt -x chaos -t 8

# Search with gravity pattern
./keyhunt -m address -f targets.txt -x gravity -t 8

# Search with spiral pattern
./keyhunt -m address -f targets.txt -x spiral -t 8

# Search with auto pattern (cycles through all)
./keyhunt -m address -f targets.txt -x auto -t 8

# Search with sequential pattern
./keyhunt -m address -f targets.txt -x sequential -t 8

# Search with random pattern
./keyhunt -m address -f targets.txt -x random -t 8

# Search with reverse pattern
./keyhunt -m address -f targets.txt -x reverse -t 8

# Quiet mode with stats every 10 seconds
./keyhunt -m address -f targets.txt -q -s 10 -t 8

# With endomorphism (3x speedup)
./keyhunt -m address -f targets.txt -e -t 8

# Compressed keys only
./keyhunt -m address -f targets.txt -l compress -t 8

# Uncompressed keys only
./keyhunt -m address -f targets.txt -l uncompress -t 8

# With timestamp range
./keyhunt -m address -f targets.txt -T 1609459200 -t 8
```

---

### pubkey2addr

Random key generation with address matching. Defaults to `-x random`.

**Supported Cryptocurrencies:** BTC, ETH, LTC, DOGE, XRP, BCH, BTG, ETC, ALL

**Input file format:** One address per line (format depends on currency).

**How it works:**
1. Load target addresses into binary fuse filter
2. Each thread picks random private keys across full key space
3. Derives public key → address
4. Checks address against filter + binary search on match
5. Found key is written to `KEYFOUNDKEYFOUND.txt`

**Examples:**

```bash
# Random key generation with BTC addresses
./keyhunt -m pubkey2addr -f targets.txt -t 8

# Random key generation with ETH addresses
./keyhunt -m pubkey2addr -c eth -f eth_targets.txt -t 8

# With auto search pattern
./keyhunt -m pubkey2addr -f targets.txt -x auto -t 4

# With chaos search pattern
./keyhunt -m pubkey2addr -f targets.txt -x chaos -t 4

# With gravity search pattern
./keyhunt -m pubkey2addr -f targets.txt -x gravity -t 4

# With spiral search pattern
./keyhunt -m pubkey2addr -f targets.txt -x spiral -t 4

# With reverse search pattern
./keyhunt -m pubkey2addr -f targets.txt -x reverse -t 4

# Quiet mode with stats every 10 seconds
./keyhunt -m pubkey2addr -f targets.txt -q -s 10 -t 4
```

---

### rmd160

Search for private keys matching target RIPEMD-160 hashes.

**Supported Cryptocurrencies:** BTC, ETH, LTC, DOGE, XRP, BCH, BTG, ETC, ALL

**Input file format:** One 40-char hex RIPEMD-160 hash per line.

**Custom derivation paths** (`-p`): Same as address mode — each random key is used as a BIP-32 master key and child keys are derived along the path. Use `-V` for verbose output.

**Examples:**

```bash
# Search RMD160 hashes
./keyhunt -m rmd160 -f tests/1to32.rmd -r 1:FFFFFFFF -l compress -s 5

# Search with custom derivation path
./keyhunt -m rmd160 -p "m/44'/0'/0'/0" -D 10 -f hashes.rmd -V -t 8

# Search with chaos pattern
./keyhunt -m rmd160 -f tests/1to32.rmd -x chaos -t 8

# Search with gravity pattern
./keyhunt -m rmd160 -f tests/1to32.rmd -x gravity -t 8

# Search with auto pattern
./keyhunt -m rmd160 -f tests/1to32.rmd -x auto -t 8

# Quiet mode with stats every 10 seconds
./keyhunt -m rmd160 -f tests/1to32.rmd -q -s 10 -t 8
```

---

### xpoint

Search by matching x-coordinate of public keys.

**Input file format:** One hex public key per line (64-char x-only, 66-char compressed, or 130-char uncompressed with 04 prefix).

**How it works:**
1. Parse public keys from file, extract x-coordinates
2. Generate candidate keys and compute public keys
3. Compare x-coordinate against the sorted target array
4. On match, prints the full private key

**Examples:**

```bash
# Search xpoint with range
./keyhunt -m xpoint -f tests/1to32.txt -r 1:FFFFFFFF -l compress -s 5

# Search with chaos pattern
./keyhunt -m xpoint -f tests/1to32.txt -x chaos -t 8

# Search with gravity pattern
./keyhunt -m xpoint -f tests/1to32.txt -x gravity -t 8

# Search with auto pattern
./keyhunt -m xpoint -f tests/1to32.txt -x auto -t 8

# Quiet mode with stats every 10 seconds
./keyhunt -m xpoint -f tests/1to32.txt -q -s 10 -t 8
```

---

### bsgs

Baby-Step Giant-Step mode for solving crypto puzzles.

**Input file format:** Compressed (66-char) or uncompressed (130-char) public keys.

**How it works:**
1. Build baby-step table from target public keys (x-coordinates)
2. Build 3-tier binary fuse/bloom filter from table
3. Walk giant steps through key space, checking each against table
4. On bloom hit, verify with second/third check for confirmation
5. Found key written to `KEYFOUNDKEYFOUND.txt`

**BSGS search modes (`-B`):**
- `sequential` — walk forward from start
- `backward` — walk backward from end
- `both` — alternate top/bottom of range
- `random` — random giant steps (default)
- `dance` — triple: top/bottom/random alternation

**Examples:**

```bash
# BSGS sequential
./keyhunt -m bsgs -f tests/125.txt -B sequential -n 0x1000000 -t 4

# BSGS backward
./keyhunt -m bsgs -f tests/125.txt -B backward -n 0x1000000 -t 4

# BSGS both
./keyhunt -m bsgs -f tests/125.txt -B both -n 0x1000000 -t 4

# BSGS random
./keyhunt -m bsgs -f tests/125.txt -B random -n 0x1000000 -t 4

# BSGS dance
./keyhunt -m bsgs -f tests/125.txt -B dance -n 0x1000000 -t 4

# BSGS with chaos pattern
./keyhunt -m bsgs -f tests/125.txt -x chaos -n 0x1000000 -t 4

# BSGS with gravity pattern
./keyhunt -m bsgs -f tests/125.txt -x gravity -n 0x1000000 -t 4

# BSGS with spiral pattern
./keyhunt -m bsgs -f tests/125.txt -x spiral -n 0x1000000 -t 4

# BSGS with auto pattern (cycles through all)
./keyhunt -m bsgs -f tests/125.txt -x auto -n 0x1000000 -t 4

# BSGS with reverse pattern
./keyhunt -m bsgs -f tests/125.txt -x reverse -n 0x1000000 -t 4

# BSGS quiet mode with stats every 10 seconds
./keyhunt -m bsgs -f tests/125.txt -q -s 10 -n 0x1000000 -t 4

# BSGS with higher K factor (more RAM, faster)
./keyhunt -m bsgs -f tests/125.txt -k 128 -n 0x10000000000 -t 4

# BSGS with bit range
./keyhunt -m bsgs -f tests/125.txt -b 125 -t 4

# BSGS save/load bloom filters
./keyhunt -m bsgs -f tests/125.txt -S -t 4
```

---

### vanity

Search for a specific vanity address prefix.

**Supported Cryptocurrencies:** BTC, ETH, LTC, DOGE, XRP, BCH, BTG, ETC, ALL

**Input file format:** Not applicable (use `-v` flag).

**How it works:**
1. Parse the target prefix and build matching rules
2. Generate candidate keys and derive addresses
3. Check if the address starts with the target prefix
4. On match, prints the private key and full address

**Examples:**

```bash
# Single vanity prefix
./keyhunt -m vanity -v 1Cool -t 8

# Multiple vanity prefixes
./keyhunt -m vanity -v 1Cool -v 1MyKey -t 8

# With chaos pattern
./keyhunt -m vanity -v 1Cool -x chaos -t 8

# With gravity pattern
./keyhunt -m vanity -v 1Cool -x gravity -t 8

# With auto pattern
./keyhunt -m vanity -v 1Cool -x auto -t 8

# Quiet mode with stats every 10 seconds
./keyhunt -m vanity -v 1Cool -q -s 10 -t 8

# With endomorphism (3x speedup)
./keyhunt -m vanity -v 1Cool -e -t 8

# Compressed keys only
./keyhunt -m vanity -v 1Cool -l compress -t 8
```

---

### minikeys

Generate and test minikeys (short Bitcoin private keys).

**Input file format:** One BTC address (base58) per line.

**How it works:**
1. Generate candidate minikey strings
2. Validate: starts with S, SHA256 first byte is 0x00
3. Derive private key → public key → address
4. Check against target address file

**Examples:**

```bash
# Basic minikeys search
./keyhunt -m minikeys -f targets.txt -t 4

# With custom base minikey
./keyhunt -m minikeys -C SG64GZqySYwBm9KxE1wJ28 -f targets.txt -t 4

# Random minikeys
./keyhunt -m minikeys -f targets.txt -n 0x10000 -q -R

# With chaos pattern
./keyhunt -m minikeys -f targets.txt -x chaos -t 4

# Quiet mode with stats every 10 seconds
./keyhunt -m minikeys -f targets.txt -q -s 10 -t 4
```

---

### mnemonic

Generate random BIP-39 mnemonics, derive keys via BIP-32/44/49/84.

**Input file format:** One BTC address (base58) or ETH address (0x...) per line.

**How it works:**
1. Generate random entropy (128-256 bits)
2. SHA-256 checksum, encode as 12-24 BIP-39 words
3. Validate mnemonic checksum (rejects invalid)
4. PBKDF2 → seed → BIP-32 master key
5. Derive child keys along each path
6. Compute address for each derived key
7. Check against binary fuse filter of target addresses

**Derivation paths checked:**
- BIP-44: `m/44'/0'/0'/0/N` — Legacy P2PKH (1...)
- BIP-49: `m/49'/0'/0'/0/N` — Wrapped SegWit (3...)
- BIP-84: `m/84'/0'/0'/0/N` — Native SegWit (bc1...)

**Examples:**

```bash
# 12-word English mnemonics
./keyhunt -m mnemonic -w 12 -L english -f targets.txt -t 4

# 24-word English mnemonics
./keyhunt -m mnemonic -w 24 -L english -f targets.txt -t 4

# 15-word mnemonics
./keyhunt -m mnemonic -w 15 -L english -f targets.txt -t 4

# 18-word mnemonics
./keyhunt -m mnemonic -w 18 -L english -f targets.txt -t 4

# 21-word mnemonics
./keyhunt -m mnemonic -w 21 -L english -f targets.txt -t 4

# Random word count (12, 15, 18, 21, or 24)
./keyhunt -m mnemonic -w 0 -L english -f targets.txt -t 4

# Japanese mnemonics
./keyhunt -m mnemonic -w 12 -L japanese -f targets.txt -t 4

# Spanish mnemonics
./keyhunt -m mnemonic -w 12 -L spanish -f targets.txt -t 4

# French mnemonics
./keyhunt -m mnemonic -w 12 -L french -f targets.txt -t 4

# Italian mnemonics
./keyhunt -m mnemonic -w 12 -L italian -f targets.txt -t 4

# Czech mnemonics
./keyhunt -m mnemonic -w 12 -L czech -f targets.txt -t 4

# Portuguese mnemonics
./keyhunt -m mnemonic -w 12 -L portuguese -f targets.txt -t 4

# Korean mnemonics
./keyhunt -m mnemonic -w 12 -L korean -f targets.txt -t 4

# Chinese Simplified mnemonics
./keyhunt -m mnemonic -w 12 -L chinese_simplified -f targets.txt -t 4

# Chinese Traditional mnemonics
./keyhunt -m mnemonic -w 12 -L chinese_traditional -f targets.txt -t 4

# All languages (cycles through all 10)
./keyhunt -m mnemonic -w 12 -L all -f targets.txt -t 4

# ETH addresses
./keyhunt -m mnemonic -w 12 -W -f eth_targets.txt -t 4

# Check 50 address indices per path (150 total per mnemonic)
./keyhunt -m mnemonic -D 50 -f targets.txt -t 8

# With chaos pattern
./keyhunt -m mnemonic -w 12 -L english -f targets.txt -x chaos -t 4

# With gravity pattern
./keyhunt -m mnemonic -w 12 -L english -f targets.txt -x gravity -t 4

# With auto pattern
./keyhunt -m mnemonic -w 12 -L english -f targets.txt -x auto -t 4

# Quiet mode with stats every 10 seconds
./keyhunt -m mnemonic -w 12 -L english -f targets.txt -q -s 10 -t 4
```

---

### poetry

Generate random poetry words, encode as hex private keys.

**Input file format:** One BTC address (base58) per line.

**How it works:**
1. Pick random poetry words (3 words per group)
2. Decode word indices into hex private key
3. Derive public key → address
4. Check against binary fuse filter of targets

**Examples:**

```bash
# Basic poetry search
./keyhunt -m poetry -f targets.txt -t 4

# With chaos pattern
./keyhunt -m poetry -f targets.txt -x chaos -t 4

# With gravity pattern
./keyhunt -m poetry -f targets.txt -x gravity -t 4

# With auto pattern
./keyhunt -m poetry -f targets.txt -x auto -t 4

# Quiet mode with stats every 10 seconds
./keyhunt -m poetry -f targets.txt -q -s 10 -t 4
```

---

### brainwallet

SHA256 hash passphrases from wordlist to derive private keys.

**Input file format:** One BTC address (base58) per line.

**How it works:**
1. Pick 1-24 random words from wordlist (or use `-w` count)
2. Apply SHA256 and double-SHA256 to the passphrase
3. Try number suffixes, leet speak, caps, separators
4. Derive public key → address for each variant
5. Check each against binary fuse filter of targets

**Mutations applied:**
- SHA256 hash (standard brainwallet)
- Double SHA256 hash (SHA256(SHA256()))
- Number suffix (1, 123, 0, 2024, 69, 420, 7, 13, 99)
- Leet speak (a→@, e→3, o→0, s→$, etc.)
- Capitalize first letter
- ALL CAPS
- Separators: space, dash, underscore, dot, none

**Examples:**

```bash
# Basic brainwallet search
./keyhunt -m brainwallet -f targets.txt -t 8

# 3-word phrases
./keyhunt -m brainwallet -w 3 -f targets.txt -t 8

# Random word count
./keyhunt -m brainwallet -w 0 -f targets.txt -t 8

# With chaos pattern
./keyhunt -m brainwallet -f targets.txt -x chaos -t 8

# With gravity pattern
./keyhunt -m brainwallet -f targets.txt -x gravity -t 8

# With auto pattern
./keyhunt -m brainwallet -f targets.txt -x auto -t 8

# Quiet mode with stats every 10 seconds
./keyhunt -m brainwallet -f targets.txt -q -s 10 -t 8
```

---

## Search Patterns (`-x`)

All search patterns work with **ALL search modes** including BSGS.

| Pattern | Description |
|---------|-------------|
| `sequential` | Linear walk from start to end of range |
| `random` | Random key selection across full range |
| `chaos` | Logistic map chaotic sequence (r=3.99999) |
| `gravity` | Adaptive: clusters around previously found keys |
| `spiral` | Archimedean spiral from range midpoint |
| `reverse` | Inverted BSGS baby/giant step roles |
| `auto` | Cycling: spiral→chaos→gravity→reverse |

**Examples:**

```bash
# Sequential search
./keyhunt -m address -f targets.txt -x sequential -t 8

# Random search
./keyhunt -m address -f targets.txt -x random -t 8

# Chaos search
./keyhunt -m address -f targets.txt -x chaos -t 8

# Gravity search
./keyhunt -m address -f targets.txt -x gravity -t 8

# Spiral search
./keyhunt -m address -f targets.txt -x spiral -t 8

# Reverse search
./keyhunt -m address -f targets.txt -x reverse -t 8

# Auto search (cycles through all patterns)
./keyhunt -m address -f targets.txt -x auto -t 8

# Works with BSGS too
./keyhunt -m bsgs -f pubkeys.txt -x chaos -n 0x1000000 -t 4

# Works with mnemonic
./keyhunt -m mnemonic -w 12 -f targets.txt -x auto -t 4

# Works with brainwallet
./keyhunt -m brainwallet -f targets.txt -x chaos -t 8
```

---

## Range Options

```bash
# Specific hex range
./keyhunt -m address -f targets.txt -r 1:FFFFFFFF -t 8

# Open-ended range (from start to curve order)
./keyhunt -m address -f targets.txt -r 1:FFFF -t 8

# Bit range (puzzle 66)
./keyhunt -m address -f targets.txt -b 66 -t 8

# Bit range (puzzle 125)
./keyhunt -m bsgs -f targets.txt -b 125 -t 8

# Timestamp range (search around Jan 1 2021)
./keyhunt -m address -f targets.txt -T 1609459200 -t 8

# Timestamp + bit range for tighter window
./keyhunt -m address -f targets.txt -T 1609459200 -b 32 -t 8
```

---

## Output Options

```bash
# Quiet mode (suppress thread output)
./keyhunt -m address -f targets.txt -q -t 8

# Stats every 10 seconds
./keyhunt -m address -f targets.txt -s 10 -t 8

# Stats every 5 seconds
./keyhunt -m address -f targets.txt -s 5 -t 8

# Turn off stats
./keyhunt -m address -f targets.txt -s 0 -t 8

# Matrix screen effect
./keyhunt -m address -f targets.txt -M -t 8

# Skip checksum on cached files
./keyhunt -m address -f targets.txt -6 -t 8
```

---

## Performance Options

```bash
# Endomorphism (3x speedup for address/rmd160/vanity)
./keyhunt -m address -f targets.txt -e -t 8

# Compressed keys only
./keyhunt -m address -f targets.txt -l compress -t 8

# Uncompressed keys only
./keyhunt -m address -f targets.txt -l uncompress -t 8

# Both compressed and uncompressed
./keyhunt -m address -f targets.txt -l both -t 8

# Stride value
./keyhunt -m address -f targets.txt -I 2 -t 8

# BSGS K factor (more RAM, faster)
./keyhunt -m bsgs -f targets.txt -k 128 -t 4

# Save/load BSGS bloom filters
./keyhunt -m bsgs -f targets.txt -S -t 4

# Bloom filter size multiplier
./keyhunt -m address -f targets.txt -z 2 -t 8
```

---

## File Formats

**Address targets:** One BTC address (base58) or ETH (0x...) per line.
```
1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
1CUNEBjYrCn2y1SdiUMohaKUi4wpP326Lb
```

**RMD160 targets:** One 40-char hex RIPEMD-160 hash per line.
```
751e76e8199196d454941c45d1b3a323f1433bd6
7dd65592d0ab2fe0d0257d571abf032cd9db93dc
```

**Public key targets:** One hex public key per line (64/66/130 chars).
```
0279be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
043ffa1cc011a8d23dec502c7656fb3f93dbe4c61f91fd443ba444b4ec2dd8e6f0406c36edf3d8a0dfaa7b8f309b8f1276a5c04131762c23594f130a023742bdde
```

**Vanity targets:** Via `-v` flag, not file.

---

## Test Files

The `tests/` directory contains:

- `1to32.eth` — Solved puzzle addresses (ETH)
- `1to32.rmd` — Solved puzzle addresses (RMD160)
- `63.pub` — Puzzle 63 public key
- `64.rmd` — Puzzle 64 RMD160
- `66.rmd` — Puzzle 66 RMD160
- `unsolvedpuzzles.rmd` — Unsolved puzzle RMD160 hashes
- `bip39/` — BIP-39 wordlists for all 10 languages
- `brainwalletwords.txt` — Brainwallet wordlist (352 words)
- `poetry.txt` — Poetry wordlist (547 words)
- `bip39_test.cpp` — BIP-39 test suite
- `poetry_test.cpp` — Poetry test suite

---

## Building

```bash
# Linux / WSL
make

# Clean
make clean
```

Requires: `g++`, `make`, `libm`, `libpthread`

---

## License

See [LICENSE](LICENSE) for details.

---

## Donations

- BTC: 1HmztBLDnwwaKAGbtALsYvCNBuoJYEic3h
- Tips to Iceland: bc1q39meky2mn5qjq704zz0nnkl0v7kj4uz6r529at

---

**TrueCollider** — Developed & Modified by TrueScent
