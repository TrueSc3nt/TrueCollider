#!/usr/bin/env python3
"""Convert Ethereum 0x addresses to packed 20-byte keccak160 .bin (MIT).

Usage:
  python scripts/eth_to_bin.py tests/eth.txt -o tests/eth.keccak160
  keyhunt_cuda.exe -m address -c eth -f tests/eth.keccak160 -U cuda -M auto -t 1
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path


def line_to_eth20(line: str) -> bytes | None:
    s = line.split("#", 1)[0].strip()
    if not s:
        return None
    if s.startswith("0x") or s.startswith("0X"):
        s = s[2:]
    if len(s) != 40 or any(c not in "0123456789abcdefABCDEF" for c in s):
        return None
    return bytes.fromhex(s)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("input", type=Path)
    ap.add_argument("-o", "--output", type=Path, required=True)
    ap.add_argument("--sort", action="store_true")
    args = ap.parse_args()

    records: list[bytes] = []
    skipped = 0
    for line in args.input.read_text(encoding="utf-8", errors="replace").splitlines():
        h = line_to_eth20(line)
        if h is None:
            if line.strip() and not line.strip().startswith("#"):
                skipped += 1
            continue
        records.append(h)
    if args.sort:
        records.sort()
    args.output.write_bytes(b"".join(records))
    print(f"[+] Wrote {len(records)} x 20-byte ETH records -> {args.output} ({skipped} skipped)")
    return 0 if records else 1


if __name__ == "__main__":
    sys.exit(main())
