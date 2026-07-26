#!/usr/bin/env python3
"""Extract X coordinates from pubkeys to packed 32-byte .bin (MIT).

Accepts compressed (66 hex) or uncompressed (130 hex) pubkeys, one per line.

Usage:
  python scripts/pub_to_xpoint.py tests/63.pub -o tests/63.xpoint.bin
  keyhunt_cuda.exe -m xpoint -f tests/63.xpoint.bin -U cuda ...
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path


def line_to_x32(line: str) -> bytes | None:
    s = line.split("#", 1)[0].strip()
    if not s:
        return None
    if s.startswith("0x") or s.startswith("0X"):
        s = s[2:]
    if any(c not in "0123456789abcdefABCDEF" for c in s):
        return None
    if len(s) == 64:
        return bytes.fromhex(s)
    if len(s) == 66 and s[:2] in ("02", "03"):
        return bytes.fromhex(s[2:])
    if len(s) == 130 and s[:2] == "04":
        return bytes.fromhex(s[2:66])
    return None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("input", type=Path)
    ap.add_argument("-o", "--output", type=Path, required=True)
    ap.add_argument("--sort", action="store_true")
    args = ap.parse_args()

    records: list[bytes] = []
    skipped = 0
    for line in args.input.read_text(encoding="utf-8", errors="replace").splitlines():
        x = line_to_x32(line)
        if x is None or len(x) != 32:
            if line.strip() and not line.strip().startswith("#"):
                skipped += 1
            continue
        records.append(x)
    if args.sort:
        records.sort()
    args.output.write_bytes(b"".join(records))
    print(f"[+] Wrote {len(records)} x 32-byte xpoints -> {args.output} ({skipped} skipped)")
    return 0 if records else 1


if __name__ == "__main__":
    sys.exit(main())
