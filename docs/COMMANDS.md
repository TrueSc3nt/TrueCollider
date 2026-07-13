# Command cookbook

Copy, paste, and change file names / bit ranges. On Windows use `keyhunt.exe` / `keyhunt_cuda.exe`.

---

## Cheat sheet

| Flag | Example | Meaning |
|------|---------|---------|
| `-m` | `-m address` | Mode |
| `-f` | `-f tests/66.txt` | Target file |
| `-c` | `-c btc` / `eth` / `sol` / `troot` | Currency |
| `-t` | `-t 8` | CPU threads |
| `-b` / `-r` | `-b 66` / `-r 1:ffff` | Bit or hex range |
| `-l` | `-l compress` | compress / uncompress / both |
| `-e` | `-e` | GLV endomorphism (CPU secp) |
| `-A` | `-A auto` | Vector: auto / sse / avx2 / avx512 |
| `-U` | `-U cuda` | GPU: none / cuda / opencl |
| `-G` | `-G 8192` | GPU batch hint (keys) |
| `-M` | `-M auto` / `-M 2048` / `-M 2G` | Memory budget (VRAM/RAM) |
| `-k` | `-k 512` / `-k auto` | BSGS K factor |
| `-n` | `-n 0x100000000000` | BSGS N (≥ 2^20, exact sqrt) |
| `-y` | `-y` | Dry-run config and exit |
| `-v` | `-v 1Cool` | Vanity prefix |
| `-S` | `-S` | Save/load BSGS tables |
| `-q` `-s` | `-q -s 10` | Quiet / stats seconds |

---

## CPU recipes

```bash
./keyhunt -m address -f tests/66.txt -b 66 -l compress -e -A auto -t 8 -q -s 10
./keyhunt -m address -c eth -f eth.txt -t 8 -q -s 10
./keyhunt -m address -c sol -f tests/sol_sample.txt -t 8
./keyhunt -m vanity -v 1Cool -e -t 8
./keyhunt -m kangaroo -f tests/125.txt -r 1:100000
./keyhunt -m mnemonic -f tests/66.txt -w 12 -t 4
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
keyhunt_cuda.exe -m address -c eth -f eth.txt -U cuda -M auto -t 1
keyhunt_cuda.exe -m vanity -v 1Love -U cuda -M auto -t 1
keyhunt_cuda.exe -m bsgs -f tests/125.txt -b 125 -k auto -U cuda -M auto -t 4 -S
keyhunt_cuda.exe -m address -f tests/66.txt -U cuda -M auto -y
```

**Solana:** `-U cuda -c sol` uses GPU SHA512 + host ed25519. BSGS giant-step GRP remains CPU; baby table + giant `ComputePublicKey` use GPU EC.

Hits → `FOUND_BTC.txt` / `FOUND_ETH.txt` / `FOUND_SOL.txt` + `KEYFOUNDKEYFOUND.txt`.

More context: [README.md](../README.md) · [gpu/README.md](../gpu/README.md) · [SPEEDS.md](SPEEDS.md).
