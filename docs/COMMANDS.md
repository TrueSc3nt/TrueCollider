# Command cookbook

Copy, paste, and change file names / bit ranges. On Windows use `keyhunt.exe` instead of `./keyhunt`.

---

## Cheat sheet (most used flags)

| Flag | Example | Meaning |
|------|---------|---------|
| `-m` | `-m address` | Mode |
| `-f` | `-f targets.txt` | Target file |
| `-c` | `-c btc` / `eth` / `sol` / `troot` / `auto` | Currency |
| `-t` | `-t 8` | CPU threads |
| `-b` | `-b 66` | Bit range (puzzle style) |
| `-r` | `-r 1:ffff` | Hex range start:end |
| `-l` | `-l compress` | compress / uncompress / both |
| `-e` | `-e` | GLV endomorphism (CPU secp only) |
| `-A` | `-A auto` | Vector: auto / sse / avx2 / avx512 |
| `-U` | `-U cuda` | GPU: none / cuda / opencl |
| `-R` | `-R` | Random walk in range |
| `-q` | `-q` | Quiet |
| `-s` | `-s 10` | Stats every N seconds |
| `-x` | `-x spiral` | Key-picking pattern |
| `-v` | `-v 1Cool` | Vanity prefix |
| `-B` | `-B sequential` | BSGS submode |

---

## Address mode (default)

### Bitcoin puzzle-style (compressed)

```bash
./keyhunt -m address -f tests/66.txt -b 66 -l compress -R -q -s 10 -t 8 -e -A auto
```

### Custom hex range

```bash
./keyhunt -m address -f targets.txt -r 100000:1fffff -l compress -t 8 -e
```

### Ethereum

```bash
./keyhunt -m address -c eth -f eth.txt -b 40 -t 8 -q -s 10
```

### Solana (ed25519 seed → base58 address)

```bash
./keyhunt -m address -c sol -f sol.txt -t 8 -q -s 10
./keyhunt -m address -c sol -f tests/sol_sample.txt -r 1:8 -t 1
```

Target file: one Solana address per line (or 64-char hex pubkey).

### Taproot

```bash
./keyhunt -m address -c troot -f troot.txt -t 8
```

### Mixed file (auto-detect)

```bash
./keyhunt -m address -c auto -f mixed.txt -t 8
```

### Multi-currency secp set

```bash
./keyhunt -m address -c all -f mixed_secp.txt -t 8
```

(`all` is for secp coins — do **not** mix Solana into `all`.)

---

## RMD160

Targets are 40-character hex RIPEMD-160 digests (no Base58).

```bash
./keyhunt -m rmd160 -f hashes.rmd -l compress -t 8 -e -A auto
```

---

## Vanity

```bash
./keyhunt -m vanity -v 1Cool -l compress -R -b 256 -e -t 8 -s 10
./keyhunt -m vanity -f vanity_prefixes.txt -l compress -R -b 256 -e -t 8
```

---

## BSGS (known public key, bit/range search)

Input file: one public key per line (compressed or uncompressed).

```bash
./keyhunt -m bsgs -f pubkeys.txt -b 125 -B sequential -t 4 -q -s 10 -R
./keyhunt -m bsgs -f pubkeys.txt -B random -n 0x1000000 -t 8
./keyhunt -m bsgs -f pubkeys.txt -x auto -S -t 8
```

`-k` / bloom save flags: see `./keyhunt -h` and Alberto-style BSGS notes in older Keyhunt docs; TrueCollider keeps BSGS on **CPU**.

---

## X-point

Match X coordinates of public keys.

```bash
./keyhunt -m xpoint -f xpoints.txt -b 40 -t 8
```

---

## Minikeys / mnemonic / poetry / brainwallet / pubkey2addr

```bash
./keyhunt -m minikeys -f targets.txt -t 8
./keyhunt -m mnemonic -f targets.txt -t 8
./keyhunt -m poetry -f targets.txt -t 4
./keyhunt -m brainwallet -f targets.txt -t 4
./keyhunt -m pubkey2addr -f targets.txt -t 8
```

BIP39 language / word count flags: `./keyhunt -h`.

---

## Patterns (`-x`) — how keys are picked

These change **order**, not the math:

```bash
-x sequential
-x random
-x chaos
-x gravity
-x spiral
-x reverse
-x auto
```

Example:

```bash
./keyhunt -m address -f targets.txt -b 66 -l compress -x spiral -e -t 8
```

---

## GPU

```bash
# NVIDIA — GPU EC (address/rmd160); hash often still on host
./keyhunt -m address -f targets.txt -U cuda -t 1 -l compress -s 5

# OpenCL — GPU hash160, EC on CPU
./keyhunt -m address -f targets.txt -U opencl -t 8 -l compress
```

Vanity / BSGS / mnemonic / Solana stay **CPU**.

---

## Speed knobs

```bash
./keyhunt -m address -f targets.txt -b 66 -l compress -e -A avx2 -t 16 -q -s 5
./keyhunt -m address -f targets.txt -b 66 -l compress -e -A avx512 -t 16
```

| Knob | Effect |
|------|--------|
| `-t` | More CPU threads |
| `-e` | ~3× address checks per base key (secp) |
| `-A avx2` / `avx512` | Wider hash160 batches |
| More RAM | Larger filters / BSGS tables |

---

## Target file formats

| Mode / `-c` | Each line |
|-------------|-----------|
| BTC legacy | `1…` / `3…` |
| Bech32 | `bc1q…` |
| ETH | `0x` + 40 hex |
| SOL | base58 pubkey (~32–44 chars) or 64 hex |
| TROOT | `bc1p…` or 64 hex x-only |
| RMD160 | 40 hex |
| BSGS | 66/130 hex pubkey |

Empty lines are ignored. Keep one target per line.
