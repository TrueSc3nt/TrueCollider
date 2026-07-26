#!/usr/bin/env python3
"""Sort fixed-length binary records in-place or to a new file (MIT).

Usage:
  python scripts/binsort.py 20 tests/in.hash160 tests/out.hash160
  python scripts/binsort.py 32 tests/in.xpoint.bin tests/out.xpoint.bin
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("length", type=int, choices=(20, 32), help="Record length in bytes")
    ap.add_argument("input", type=Path)
    ap.add_argument("output", type=Path)
    args = ap.parse_args()

    data = args.input.read_bytes()
    if len(data) % args.length != 0:
        print(f"[E] file size {len(data)} not divisible by {args.length}", file=sys.stderr)
        return 1
    recs = [data[i : i + args.length] for i in range(0, len(data), args.length)]
    recs.sort()
    args.output.write_bytes(b"".join(recs))
    print(f"[+] Sorted {len(recs)} records -> {args.output}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
