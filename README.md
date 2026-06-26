# TrueCollider

**Search Modes + Binary Fuse Filters** — A high-performance Bitcoin/ETH key search tool.

Developed & Modified by **TrueScent**

---

## Features

- **11 Search Modes** — address, pubkey2addr, rmd160, xpoint, bsgs, vanity, minikeys, mnemonic, poetry, brainwallet, pubkey2addr
- **Binary Fuse Filters** — 30-40% faster lookups, 30-40% less memory than traditional bloom filters
- **7 Search Patterns** — sequential, random, chaos, gravity, spiral, reverse, auto
- **Multi-threaded** — full multi-thread support across all modes
- **BSGS Algorithm** — Baby-Step Giant-Step for solving crypto puzzles
- **BIP-39 Mnemonics** — 10 languages, BIP-44/49/84 derivation paths
- **Endomorphism** — GLV endomorphism for 3x speedup in address/rmd160 modes
- **Timestamp Search** — narrow search ranges by creation time

## Quick Start

```bash
# Build
make

# Search for BTC address matches
./keyhunt -m address -f targets.txt -t 8

# Random key generation against address list
./keyhunt -m pubkey2addr -f btc.txt -t 8

# BSGS puzzle solver with auto search pattern
./keyhunt -m bsgs -f pubkeys.txt -x auto -t 8

# 24-word English mnemonics
./keyhunt -m mnemonic -w 24 -L english -f targets.txt -t 4
```

## Search Modes

| Mode | Description | Input File |
|------|-------------|------------|
| `address` | Match BTC/ETH addresses | Base58 or 0x addresses |
| `pubkey2addr` | Random keys → address matching | Base58 or 0x addresses |
| `rmd160` | Match RIPEMD-160 hashes | 40-char hex hashes |
| `xpoint` | Match public key x-coordinates | 64/66/130-char hex keys |
| `bsgs` | Baby-Step Giant-Step puzzle solver | Compressed/uncompressed pubkeys |
| `vanity` | Find vanity address prefixes | Via `-v` flag |
| `minikeys` | Generate short Bitcoin minikeys | Target addresses |
| `mnemonic` | BIP-39 mnemonic generation | Target addresses |
| `poetry` | Poetry word key encoding | Target addresses |
| `brainwallet` | SHA256 passphrase dictionary | Target addresses |

## Search Patterns (`-x`)

| Pattern | Description |
|---------|-------------|
| `sequential` | Linear walk from start to end |
| `random` | Random key selection |
| `chaos` | Logistic map chaotic sequence |
| `gravity` | Adaptive search around found keys |
| `spiral` | Archimedean spiral from midpoint |
| `reverse` | Inverted BSGS baby/giant step roles |
| `auto` | Cycling: spiral→chaos→gravity→reverse |

All `-x` modes work with **all search modes** including BSGS.

## Options

```
-m mode      Search mode (default: address)
-f file      Input target file
-c crypto    btc, eth (default: btc)
-t threads   Number of threads (default: 1)
-x pattern   Search pattern (default: random)
-r SR:EN     Hex range StartRange:EndRange
-T timestamp Unix timestamp for range centering
-b bits      Bit range filter
-l look      compress, uncompress, both
-e           Enable endomorphism (3x speedup)
-q           Quiet mode
-s seconds   Stats interval (default: 30)
```

## Building

```bash
# Linux / WSL
make

# Clean
make clean
```

Requires: `g++`, `make`, `libm`, `libpthread`

## Performance

- **Binary Fuse Filters**: ~8-10M lookups/sec per core
- **Endomorphism**: 3x speedup for address/rmd160/vanity modes
- **Multi-threading**: scales linearly with core count

## License

See [LICENSE](LICENSE) for details.

---

**TrueCollider** — Developed & Modified by TrueScent
