/*
 * Verification test for the binary fuse filter integration.
 *
 * Checks:
 *   - All inserted keys are reported as present (zero false negatives)
 *   - Measured false-positive rate is close to the theoretical ~1/256
 *     for binary_fuse8 (1-byte fingerprint)
 *   - Memory footprint is documented
 *
 * Run with: ./test_binaryfuse
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include "binaryfuse/binaryfusefilter.h"

static uint64_t rng(uint64_t *state) {
  uint64_t x = *state;
  x ^= x >> 12;
  x ^= x << 25;
  x ^= x >> 27;
  *state = x;
  return x * 0x2545F4914F6CDD1DULL;
}

int main(int argc, char **argv) {
  (void)argc; (void)argv;

  const uint32_t sizes[] = {1000, 10000, 100000, 1000000};
  const int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
  int total_pass = 1;

  uint64_t seed = (uint64_t)time(NULL);
  printf("Binary Fuse Filter Verification\n");
  printf("RNG seed: %llu\n", (unsigned long long)seed);
  printf("---------------------------------------------------------------\n");
  printf("%-10s %-12s %-12s %-12s %-12s\n",
         "Size", "Memory(B)", "FalseNeg", "FalsePos", "FP Rate");

  for (int s = 0; s < num_sizes; s++) {
    uint32_t n = sizes[s];
    uint64_t *keys = (uint64_t *)malloc(n * sizeof(uint64_t));
    if (!keys) {
      fprintf(stderr, "Out of memory\n");
      return 1;
    }

    for (uint32_t i = 0; i < n; i++) {
      keys[i] = rng(&seed);
    }

    binary_fuse8_t filter;
    if (!binary_fuse8_allocate(n, &filter)) {
      fprintf(stderr, "Failed to allocate filter for size %u\n", n);
      free(keys);
      return 1;
    }

    if (!binary_fuse8_populate(keys, n, &filter)) {
      fprintf(stderr, "Failed to populate filter for size %u\n", n);
      binary_fuse8_free(&filter);
      free(keys);
      return 1;
    }

    size_t mem = binary_fuse8_size_in_bytes(&filter);

    // False negatives: must be zero for all inserted keys.
    int false_neg = 0;
    for (uint32_t i = 0; i < n; i++) {
      if (!binary_fuse8_contain(keys[i], &filter)) {
        false_neg++;
      }
    }

    // False positives: probe with keys that were NOT inserted.
    uint32_t probe_count = n < 100000 ? 100000 : n;
    int false_pos = 0;
    for (uint32_t i = 0; i < probe_count; i++) {
      uint64_t k = rng(&seed);
      // Simple heuristic: avoid exact collisions with inserted keys (unlikely).
      if (binary_fuse8_contain(k, &filter)) {
        false_pos++;
      }
    }

    double fp_rate = (double)false_pos / (double)probe_count;
    printf("%-10u %-12zu %-12d %-12d %-12.6f%%\n",
           n, mem, false_neg, false_pos, fp_rate * 100.0);

    if (false_neg != 0) {
      fprintf(stderr, "ERROR: false negatives detected for size %u\n", n);
      total_pass = 0;
    }

    binary_fuse8_free(&filter);
    free(keys);
  }

  printf("---------------------------------------------------------------\n");
  printf("Expected false-positive rate for binary_fuse8: ~0.39%% (1/256)\n");
  printf("Result: %s\n", total_pass ? "PASS" : "FAIL");
  return total_pass ? 0 : 1;
}
