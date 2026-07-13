# Binary Fuse Filter Verification

This document records the expected behavior, measured false-positive rates, and memory footprint of the binary fuse filter used in TrueCollider address mode.

## What is the binary fuse filter?

A binary fuse filter is a compact, probabilistic membership data structure. It is used in address mode to quickly reject generated addresses that are **not** in the target set. Only candidates that pass the filter are checked against a sorted array of real targets (a binary search), so the filter eliminates the vast majority of expensive work.

TrueCollider uses the 8-bit fingerprint variant (`binary_fuse8`) by default.

## Expected properties

| Property | Value |
|----------|-------|
| False negatives | **0** — any inserted key is always reported as present |
| False positive rate (8-bit) | ~1/256 = **~0.39%** |
| Memory overhead | ~1 byte per stored key + small fixed overhead |
| Lookup speed | O(1), very small constant |

## Verification

Run the included test:

```bash
cmake -B build
cmake --build build -j$(nproc)
./build/tests_cmake/test_binaryfuse
```

Typical output on a 64-bit x86 machine:

```
Binary Fuse Filter Verification
RNG seed: 1752399041
---------------------------------------------------------------
Size       Memory(B)    FalseNeg     FalsePos     FP Rate
1000       2030         0            383          0.383000%
10000      16438        0            397          0.397000%
100000     145231       0            389          0.389000%
1000000    1259752      0            3926         0.392600%
---------------------------------------------------------------
Expected false-positive rate for binary_fuse8: ~0.39% (1/256)
Result: PASS
```

> Numbers vary slightly depending on the random seed and the number of probe keys, but they consistently match the expected ~1/256 theoretical rate.

## Memory footprint comparison

The filter is also much smaller than a classic Bloom filter for the same false-positive rate:

| Filter type | Bits per element | False-positive rate |
|-------------|------------------|---------------------|
| Standard Bloom (1% FP) | ~9.6 | 1% |
| Standard Bloom (0.39% FP) | ~13 | 0.39% |
| Binary fuse 8 | ~8 | ~0.39% |

This gives the claimed **30–40% memory reduction** over an equivalent Bloom filter, and the O(1) lookup is generally faster in practice because it touches only 3 cache-friendly array slots and avoids the complex hashing of a k-hash Bloom filter.

## Impact on address mode

In address mode the workflow is:

1. Load target addresses into a sorted array (`std::vector<std::string>` / `std::vector<uint8_t>`).
2. Build the binary fuse filter from the same keys.
3. For every generated key, compute the address, hash it to a 64-bit key, and ask the filter.
4. If the filter says **not present**, the address is discarded immediately.
5. If the filter says **present**, do a binary search on the sorted array to confirm.

Because the filter is so fast, the cost of computing public keys and hashing dominates the runtime, making this a net win compared to a full binary search per key.

## Notes and caveats

- The 8-bit filter is suitable for target sets where an occasional false positive only triggers a cheap binary search, not a false hit.
- For target sets larger than ~10 million keys, a 16-bit filter (`binary_fuse16`) would drop the false-positive rate to ~1/65536, at the cost of doubling memory. The filter code already supports this, but TrueCollider currently uses the 8-bit version.
- Duplicate keys in the input set are handled automatically during filter construction (`binary_fuse_sort_and_remove_dup`).
- Always verify that the binary search confirmation step is active, otherwise the false-positive rate would translate directly to false alerts.
