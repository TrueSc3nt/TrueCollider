keys = {
    1: 0x1, 2: 0x3, 3: 0x7, 4: 0x8, 5: 0x15, 6: 0x31, 7: 0x4c, 8: 0xe0,
    9: 0x1d3, 10: 0x202, 11: 0x483, 12: 0xa7b, 13: 0x1460, 14: 0x2930,
    15: 0x68f3, 16: 0xc936, 17: 0x1764f, 18: 0x3080d, 19: 0x5749f,
    20: 0xd2c55, 21: 0x1ba534, 22: 0x2de40f, 23: 0x556e52, 24: 0xdc2a04,
    25: 0x1fa5ee5, 26: 0x340326e, 27: 0x6ac3875, 28: 0xd916ce8,
    29: 0x17e2551e, 30: 0x3d94cd64, 31: 0x7d4fe747, 32: 0xb862a62e,
    33: 0x1a96ca8d8, 34: 0x34a65911d, 35: 0x4aed21170, 36: 0x9de820a7c,
    37: 0x1757756a93, 38: 0x22382facd0, 39: 0x4831ba3048, 40: 0x118ea4e25e,
    41: 0x22642e890a, 42: 0x14714265ac, 43: 0x51b7342f35, 44: 0x214748363a,
    45: 0x6db34902ea, 46: 0xd2e8b8200e, 47: 0x19dba991a7, 48: 0x3a1405d9a4,
    49: 0x7f04e9306f, 50: 0x10b5e92e8a, 51: 0x23cade85ea, 52: 0x43f91e472f,
    53: 0x9cd0d874b8, 54: 0x143e13b2a8, 55: 0x3207d8c19b, 56: 0x6573b531ad,
    57: 0xb0d87a4c0e, 58: 0x1575c23a1c, 59: 0x2d37f0956a, 60: 0x5a56e04573,
    61: 0xb2e0f41c88, 62: 0x1234567890ab, 63: 0x21c3677c13, 64: 0xf7051f27b0,
}

# Check if keys follow any simple pattern
print("Puzzle | Key (hex)           | Bits | Position in range")
for p in sorted(keys.keys()):
    k = keys[p]
    bits = k.bit_length()
    range_start = 1 << (p - 1)
    range_end = (1 << p) - 1
    pos = (k - range_start) / max(1, range_end - range_start) * 100
    print(f"  {p:4}  | {k:016x}      | {bits:4} | {pos:.2f}%")

# Check ratios between consecutive keys
print("\nRatios between consecutive keys:")
for i in range(2, 20):
    if i in keys and (i-1) in keys:
        ratio = keys[i] / keys[i-1]
        print(f"  {i}/{i-1} = {ratio:.4f}")

# Check if keys follow a polynomial or exponential fit
print("\nKey values vs puzzle number:")
for p in [10, 20, 30, 40, 50, 60]:
    if p in keys:
        log_key = keys[p].bit_length()
        print(f"  Puzzle {p}: {log_key} bits, value = {keys[p]}")
