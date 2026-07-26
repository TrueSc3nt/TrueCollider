#!/usr/bin/env python3
"""Convert BTC-family addresses (Base58 / bech32 P2WPKH) to packed 20-byte hash160 .bin

MIT — TrueCollider custom CUDA edition. Idea inspired by KeyHunt-Cuda tooling;
this script is an independent reimplementation (no GPL code).

Usage:
  python scripts/addr_to_hash160.py tests/66.txt -o tests/66.hash160
  keyhunt_cuda.exe -m address -f tests/66.hash160 -U cuda -M auto -t 1
"""
from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path


ALPHABET = b"123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz"


def b58decode(s: str) -> bytes:
    n = 0
    for ch in s.encode("ascii"):
        n = n * 58 + ALPHABET.index(ch)
    # leading zeros
    pad = 0
    for ch in s:
        if ch == "1":
            pad += 1
        else:
            break
    full = n.to_bytes((n.bit_length() + 7) // 8 or 1, "big")
    return b"\x00" * pad + full


def hash160_from_base58(addr: str) -> bytes | None:
    try:
        raw = b58decode(addr.strip())
    except (ValueError, KeyError):
        return None
    if len(raw) < 25:
        return None
    payload, checksum = raw[:-4], raw[-4:]
    if hashlib.sha256(hashlib.sha256(payload).digest()).digest()[:4] != checksum:
        return None
    # version(1) + hash160(20)
    if len(payload) != 21:
        return None
    return payload[1:21]


def bech32_polymod(values):
    GEN = [0x3B6A57B2, 0x26508E6D, 0x1EA119FA, 0x3D4233DD, 0x2A1462B3]
    chk = 1
    for v in values:
        b = chk >> 25
        chk = ((chk & 0x1FFFFFF) << 5) ^ v
        for i in range(5):
            chk ^= GEN[i] if ((b >> i) & 1) else 0
    return chk


def bech32_hrp_expand(hrp: str):
    return [ord(x) >> 5 for x in hrp] + [0] + [ord(x) & 31 for x in hrp]


def bech32_decode(addr: str):
    addr = addr.strip()
    if any(ord(x) < 33 or ord(x) > 126 for x in addr):
        return None
    if addr.lower() != addr and addr.upper() != addr:
        return None
    addr = addr.lower()
    pos = addr.rfind("1")
    if pos < 1 or pos + 7 > len(addr):
        return None
    hrp, data = addr[:pos], addr[pos + 1 :]
    charset = "qpzry9x8gf2tvdw0s3jn54khce6mua7l"
    try:
        decoded = [charset.index(c) for c in data]
    except ValueError:
        return None
    if bech32_polymod(bech32_hrp_expand(hrp) + decoded) != 1:
        return None
    return hrp, decoded[:-6]


def convertbits(data, frombits, tobits, pad=True):
    acc = 0
    bits = 0
    ret = []
    maxv = (1 << tobits) - 1
    for value in data:
        acc = (acc << frombits) | value
        bits += frombits
        while bits >= tobits:
            bits -= tobits
            ret.append((acc >> bits) & maxv)
    if pad:
        if bits:
            ret.append((acc << (tobits - bits)) & maxv)
    elif bits >= frombits or ((acc << (tobits - bits)) & maxv):
        return None
    return ret


def hash160_from_bech32(addr: str) -> bytes | None:
    dec = bech32_decode(addr)
    if not dec:
        return None
    hrp, data = dec
    if hrp not in ("bc", "tb") or not data:
        return None
    witver = data[0]
    prog = convertbits(data[1:], 5, 8, False)
    if prog is None or witver != 0 or len(prog) != 20:
        return None
    return bytes(prog)


def line_to_hash160(line: str) -> bytes | None:
    s = line.split("#", 1)[0].strip()
    if not s or s.startswith(";"):
        return None
    if s.startswith("0x") or s.startswith("0X"):
        return None
    if len(s) == 40 and all(c in "0123456789abcdefABCDEF" for c in s):
        return bytes.fromhex(s)
    if s.lower().startswith("bc1") or s.lower().startswith("tb1"):
        return hash160_from_bech32(s)
    return hash160_from_base58(s)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("input", type=Path, help="Text file of addresses / 40-hex hash160")
    ap.add_argument("-o", "--output", type=Path, required=True, help="Output .bin / .hash160")
    ap.add_argument("--sort", action="store_true", help="Sort records (binary search friendly)")
    args = ap.parse_args()

    records: list[bytes] = []
    skipped = 0
    for line in args.input.read_text(encoding="utf-8", errors="replace").splitlines():
        h = line_to_hash160(line)
        if h is None or len(h) != 20:
            if line.strip() and not line.strip().startswith("#"):
                skipped += 1
            continue
        records.append(h)

    if args.sort:
        records.sort()

    args.output.write_bytes(b"".join(records))
    print(f"[+] Wrote {len(records)} x 20-byte records -> {args.output} ({skipped} skipped)")
    return 0 if records else 1


if __name__ == "__main__":
    sys.exit(main())
