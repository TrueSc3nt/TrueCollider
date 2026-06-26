# TrueCollider

**Search Modes + Binary Fuse Filters** — A high-performance Bitcoin/ETH key search tool.

Developed & Modified by **TrueScent**

---

## Features

- **11 Search Modes** — address, pubkey2addr, rmd160, xpoint, bsgs, vanity, minikeys, mnemonic, poetry, brainwallet
- **Binary Fuse Filters** — 30-40% faster lookups, 30-40% less memory than traditional bloom filters
- **7 Search Patterns** — sequential, random, chaos, gravity, spiral, reverse, auto
- **Multi-threaded** — full multi-thread support across all modes
- **BSGS Algorithm** — Baby-Step Giant-Step for solving crypto puzzles
- **BIP-39 Mnemonics** — 10 languages, BIP-44/49/84 derivation paths
- **Endomorphism** — GLV endomorphism for 3x speedup in address/rmd160/vanity modes
- **Timestamp Search** — narrow search ranges by creation time

## TL;DR

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

## Disclaimer

This tool is made as a generic tool for puzzles. I recommend everyone to stay in puzzles.

## Modes

### address

Search for private keys that produce addresses in the target file.

Example input file `tests/1to32.txt`:
```
1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
1CUNEBjYrCn2y1SdiUMohaKUi4wpP326Lb
...
```

Command:
```bash
./keyhunt -m address -f tests/1to32.txt -r 1:FFFFFFFF
```

Test with puzzle 66:
```bash
./keyhunt -m address -f tests/66.rmd -b 66 -l compress -R -q -s 10
```

### pubkey2addr

Random key generation with address matching. Defaults to `-x random`.

```bash
./keyhunt -m pubkey2addr -f btc.txt -t 8
```

### rmd160

Search for private keys matching target RIPEMD-160 hashes.

```bash
./keyhunt -m rmd160 -f tests/1to32.rmd -r 1:FFFFFFFF -l compress -s 5
```

### xpoint

Search by matching x-coordinate of public keys.

```bash
./keyhunt -m xpoint -f tests/1to32.txt -r 1:FFFFFFFF -l compress -s 5
```

### bsgs

Baby-Step Giant-Step mode for solving crypto puzzles.

```bash
./keyhunt -m bsgs -f tests/125.txt -b 125 -q -s 10 -R
```

### vanity

Search for a specific vanity address prefix.

```bash
./keyhunt -m vanity -l compress -R -b 256 -v 1Good1 -v 1MyKey
```

### minikeys

Generate and test minikeys (short Bitcoin private keys).

```bash
./keyhunt -m minikeys -f tests/1to32.txt -n 0x10000 -q -R
```

### mnemonic

Generate random BIP-39 mnemonics, derive keys, and check against target addresses.

```bash
./keyhunt -m mnemonic -w 24 -L english -f targets.txt -t 4
```

### poetry

Generate random poetry words, encode as hex private keys.

```bash
./keyhunt -m poetry -f targets.txt -t 4
```

### brainwallet

SHA256 hash passphrases from wordlist to derive private keys.

```bash
./keyhunt -m brainwallet -f targets.txt -t 8
```

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

## License

See [LICENSE](LICENSE) for details.

## Donations

- BTC: 1HmztBLDnwwaKAGbtALsYvCNBuoJYEic3h
- Tips to Iceland: bc1q39meky2mn5qjq704zz0nnkl0v7kj4uz6r529at

---

**TrueCollider** — Developed & Modified by TrueScent
