print("Puzzle | ZeroBytes | Full Range          | Stripped Range      | Reduction")
print("-------|-----------|---------------------|---------------------|----------")
for puzzle in [71, 72, 73, 74, 76, 80, 90, 100, 120, 135]:
    full_start = 1 << (puzzle - 1)
    full_end = (1 << puzzle) - 1
    full = full_end - full_start

    # Max zero bytes we can strip (must leave at least 1 bit)
    max_strip = (puzzle - 1) // 8
    if max_strip > 0:
        remaining = puzzle - (max_strip * 8)
        strip_start = 1 << (remaining - 1)
        strip_end = 1 << remaining
        stripped = strip_end - strip_start
        reduction = full / stripped
        print(f"  {puzzle:4} | {max_strip:9} | {full:>18} | {stripped:>18} | {reduction:>8.1f}x")
    else:
        print(f"  {puzzle:4} | {0:9} | {full:>18} | {full:>18} | {'1.0':>8}x")
