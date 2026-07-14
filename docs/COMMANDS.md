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
| `-x` | `-x chaos` | sequential / random / chaos / gravity / spiral / reverse / auto |
| `-U` | `-U cuda` | GPU: none / cuda / opencl |
| `-G` | `-G 8192` | GPU batch hint (keys) |
| `-M` | `-M auto` / `-M 2048` / `-M 2G` | Memory budget (VRAM/RAM); `-M matrix` = screen |
| `-k` | `-k 512` / `-k auto` | BSGS K factor |
| `-n` | `-n 0x100000000000` | BSGS N (≥ 2^20, exact sqrt) / sequential cycle size |
| `-B` | `-B dance` | BSGS: sequential / backward / both / random / dance |
| `-S` | `-S` | Save/load BSGS tables |
| `-z` | `-z 2` | Bloom size multiplier |
| `-R` | `-R` | Random / BSGS random convenience |
| `-I` | `-I 2` | Stride (address / rmd160 / xpoint) |
| `-y` | `-y` | Dry-run config and exit |
| `-v` | `-v 1Cool` | Vanity prefix |
| `-C` / `-8` | minikeys | Base string / custom alphabet |
| `-w` / `-L` / `-W` / `-D` | mnemonic | Words / language / ETH / indices |
| `-p` / `-D` | BIP-32 | Path + child index count |
| `-N` / `-Nurl` | balance | **Partial** — see README (flag not wired to hits yet) |
| `-q` `-s` | `-q -s 10` | Quiet / stats seconds |
| `-V` | `-V` | Verbose derivation |
| `-6` | `-6` | Skip cache checksum |
| `-d` | `-d` | Debug |

---

## CPU recipes

```bash
./keyhunt -m address -f tests/66.txt -b 66 -l compress -e -A auto -t 8 -q -s 10
./keyhunt -m address -c eth -f tests/_eth_1.txt -t 8 -q -s 10
./keyhunt -m address -c sol -f tests/sol_sample.txt -t 8
./keyhunt -m vanity -v 1Cool -e -t 8
./keyhunt -m kangaroo -f tests/125.txt -r 1:100000
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
```

- `-n` ≥ `1048576` (2^20), exact square root required  
- Prefer `-k` power of 2; use `-k auto` for RAM-based pick  
- Full bits→N→kmax and RAM→k tables: **[README.md](../README.md)** (BSGS section)

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

**Solana:** prefers full device ed25519; falls back to CUDA SHA512 + host ge. BSGS GRP remains CPU; baby table + giant `ComputePublicKey` use GPU EC.

Hits → `FOUND_BTC.txt` / `FOUND_ETH.txt` / `FOUND_SOL.txt` + `KEYFOUNDKEYFOUND.txt`.

More: [README.md](../README.md) · [gpu/README.md](../gpu/README.md) · [SPEEDS.md](SPEEDS.md) · [examples/](../examples/).
