# Getting started (absolute beginners)

TrueCollider is a program that tries private keys / seeds until it matches addresses (or related targets) you put in a text file.

You do **not** need to be a programmer. You need:

1. The `keyhunt` program (built or downloaded)
2. A **target file** (one address per line)
3. A **command** that says *what* to search and *how*

---

## Windows (easiest path)

### Option A — you already have `keyhunt.exe`

1. Put `keyhunt.exe` in a folder (example: `Desktop\TrueCollider`).
2. Create a text file `targets.txt` in the **same folder**.
3. Put one Bitcoin address per line, for example:

```text
1BgGZ9tcN4rm9KBzDn7KprQz87DX02FijI
```

4. Open **Command Prompt** or **PowerShell**:
   - Press `Win + R`, type `cmd`, press Enter
   - Or right-click the folder → “Open in Terminal”
5. Go to that folder:

```bat
cd Desktop\TrueCollider
```

6. Run a small test (bit range 16 = tiny range, for learning only):

```bat
keyhunt.exe -m address -f targets.txt -b 16 -l compress -t 4 -q -s 5
```

What that means:

| Piece | Meaning |
|-------|---------|
| `-m address` | Search by address |
| `-f targets.txt` | Read targets from this file |
| `-b 16` | Only search keys in a 16-bit range (demo) |
| `-l compress` | Compressed BTC addresses |
| `-t 4` | Use 4 CPU threads |
| `-q` | Quieter output |
| `-s 5` | Print speed every 5 seconds |

If a key is found, it is printed and saved to `KEYFOUNDKEYFOUND.txt` in the same folder.

### Option B — build from source (Windows CPU)

1. Install [MSYS2](https://www.msys2.org/).
2. Open **MSYS2 MinGW 64-bit**.
3. Install tools:

```bash
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-make git
```

4. Clone and build:

```bash
cd /c/Users/YOURNAME/Desktop
git clone https://github.com/TrueSc3nt/TrueCollider.git
cd TrueCollider
mingw32-make -j4
```

5. Run:

```bash
./keyhunt.exe -h
```

CUDA build (NVIDIA) is separate — see [BUILD.md](BUILD.md) / `build_cuda_vs2022.bat`.

---

## Linux

```bash
sudo apt update
sudo apt install -y git build-essential
git clone https://github.com/TrueSc3nt/TrueCollider.git
cd TrueCollider
make -j$(nproc)
./keyhunt -h
```

Example:

```bash
./keyhunt -m address -f tests/66.txt -b 66 -l compress -R -q -s 10 -t 8
```

---

## Pick the right mode (simple)

| I have… | Use |
|---------|-----|
| Bitcoin / Litecoin / etc. **addresses** | `-m address -c btc` (or `ltc`, `doge`, …) |
| Ethereum `0x…` addresses | `-m address -c eth` |
| Solana base58 addresses | `-m address -c sol` |
| Taproot `bc1p…` / x-only hex | `-m address -c troot` |
| Raw RIPEMD-160 hex (40 chars) | `-m rmd160` |
| Public keys (want private key in a bit range) | `-m bsgs` |
| Want a custom prefix like `1Cool…` | `-m vanity -v 1Cool` |
| BIP39 seed phrases | `-m mnemonic` |

Full recipes: [COMMANDS.md](COMMANDS.md).

---

## Important honesty

- Searching the **full** 256-bit keyspace is not practical. Use this for **puzzles**, **known ranges** (`-b` / `-r`), **vanity**, or research.
- Hits are written to `KEYFOUNDKEYFOUND.txt` (vanity may also use vanity output files).
- GPU (`-U cuda` / `-U opencl`) only helps some modes — see the main [README](../README.md).

---

## Get help from the program

```bash
./keyhunt -h
```

That prints every flag. If a command fails, copy the full error text — it usually says what is missing (`-f` file, wrong `-c`, etc.).
