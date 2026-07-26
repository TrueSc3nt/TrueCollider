# Windows example scripts

For the **full** mode/settings library (100+ scripts), see [`../bats/`](../bats/README.md).

Double-click or run from **repo root** via `examples\name.bat`. Each script `cd`s to the repository root automatically.

| File | Mode / purpose |
|------|----------------|
| `build_cpu.bat` | Build `keyhunt.exe` |
| `build_cuda.bat` | Build `keyhunt_cuda.exe` |
| `search_btc_address.bat` | BTC address / puzzle 66 (CPU) |
| `search_rmd160.bat` | RMD160 |
| `search_eth.bat` | Ethereum |
| `search_sol.bat` | Solana (`USE_CUDA=1` optional) |
| `search_rs_random_sequential.bat` | `-rs` smoke (`USE_CUDA=1` optional) |
| `vanity_btc.bat` | Vanity prefix |
| `bsgs_example.bat` | BSGS + dry-run |
| `kangaroo_example.bat` | Kangaroo tiny range (CPU then CUDA if built) |
| `mnemonic_example.bat` | BIP-39 |
| `poetry_example.bat` | Poetry |
| `brainwallet_example.bat` | Brainwallet |
| `minikeys_example.bat` | Minikeys |
| `xpoint_example.bat` | X-point |
| `pubkey2addr_example.bat` | pubkey2addr |
| `balance_check.bat` | `-N` smoke (needs curl/network) |
| `dry_run.bat` | `-y` CPU (+ CUDA plan if built) |
| `gpu_cuda_address.bat` | CUDA address (`-U cuda`) |
| `gpu_hybrid_both.bat` | Hybrid CPU+CUDA (`-U both`) |
| `run_keyhunt.bat` | Generic launcher (CPU / `-U` CUDA / WSL fallback) |
| `run_puzzle66_example.bat` | Tiny address demo via `targets.txt` |
| `run_sol_sample.bat` | Guaranteed Sol hit (`-r 1:8`) |
| `run_gpu_cuda_example.bat` | CUDA puzzle-66 sample |

## Notes

- Scripts reject missing or **0-byte** binaries (common corrupt download).
- CUDA demos need `keyhunt_cuda.exe` from `examples/build_cuda.bat` / `bats/00_build/build_cuda_vs2022.bat`.
- Do **not** pass `-e` with `-U cuda` or `-U both` (endomorphism forces the CPU EC path).
- `-U both` is hybrid threading (even IDs → CUDA, odd → CPU). It is **not** the same as `-l both` (compress/uncompress).
- Quick Sol hit demo: `examples/run_sol_sample.bat` (`-r 1:8`).

Full flag docs: [../README.md](../README.md) · Short recipes: [../docs/COMMANDS.md](../docs/COMMANDS.md)
