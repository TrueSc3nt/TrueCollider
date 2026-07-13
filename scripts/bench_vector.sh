#!/usr/bin/env bash
# Benchmark CPU vectorization levels for TrueCollider address search.
# Usage: ./scripts/bench_vector.sh [path/to/keyhunt] [seconds_per_run]
#
# Requires a small target file and a fixed key range for reproducible throughput.

set -euo pipefail

KEYHUNT="${1:-./keyhunt}"
SECONDS="${2:-15}"
TARGET="${TARGET_FILE:-/tmp/bench_targets.txt}"
RANGE_START="${RANGE_START:-1}"
RANGE_END="${RANGE_END:-1000000000000}"

if [[ ! -x "$KEYHUNT" && ! -f "$KEYHUNT" ]]; then
  echo "[E] keyhunt not found: $KEYHUNT" >&2
  exit 1
fi

if [[ ! -f "$TARGET" ]]; then
  # Minimal target file (P2PKH hash160 of a random key — only for throughput, not hits)
  echo "1BgGZ9tcN4rm9KBzDnYbzDtcUV4ggqAggH" > "$TARGET"
fi

MODES=(none sse avx512 auto)

echo "TrueCollider vector benchmark"
echo "  binary:  $KEYHUNT"
echo "  seconds: $SECONDS per mode"
echo "  targets: $TARGET"
echo "  range:   $RANGE_START .. $RANGE_END"
echo

for mode in "${MODES[@]}"; do
  echo "=== -A $mode ==="
  /usr/bin/time -f 'elapsed=%e user=%U sys=%S maxrss=%MKB' \
    "$KEYHUNT" -m address -f "$TARGET" -r "$RANGE_START:$RANGE_END" \
    -A "$mode" -t 1 -q 2>/dev/null || true
  echo
done

echo "Compare 'elapsed' times. On AVX-512 hardware, 'avx512' or 'auto' should be fastest."
echo "On CPUs without AVX-512, 'auto' and 'sse' should be similar."
