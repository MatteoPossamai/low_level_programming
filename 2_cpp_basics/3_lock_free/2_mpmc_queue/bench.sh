#!/usr/bin/env bash
# Benchmark harness for the MPMC queue. Compiles mpmc.cpp with -DITER,
# times whole-process runs, prints stats to stdout.
set -euo pipefail
cd "$(dirname "$0")"

# ITER is per producer; total ops = ITER * PRODUCERS
ITER="${ITER:-2000000}"
RUNS="${RUNS:-10}"
PRODUCERS="${PRODUCERS:-5}"
CONSUMERS="${CONSUMERS:-5}"
CXX="${CXX:-g++}"
FLAGS="-O2 -std=c++20 -pthread -Wall -Wextra -Wshadow -DITER=${ITER} -DPRODUCERS=${PRODUCERS} -DCONSUMERS=${CONSUMERS}"

echo "Compiling (${FLAGS})..."
"$CXX" $FLAGS mpmc.cpp -o bench_mpmc

echo "Warmup..."
./bench_mpmc # warmup, not timed

times_ms=()
for ((i = 0; i < RUNS; i++)); do
  start=$(date +%s%N)
  ./bench_mpmc
  end=$(date +%s%N)
  ms=$(awk -v s="$start" -v e="$end" 'BEGIN { printf "%.1f", (e - s) / 1e6 }')
  times_ms+=("$ms")
  echo "run $((i + 1))/${RUNS}: ${ms} ms"
done

printf '%s\n' "${times_ms[@]}" | sort -n | awk -v iter="$((ITER * PRODUCERS))" '
  { a[NR] = $1; sum += $1 }
  END {
    min = a[1]
    med = (NR % 2) ? a[(NR + 1) / 2] : (a[NR / 2] + a[NR / 2 + 1]) / 2
    mean = sum / NR
    mops = iter / (med * 1000)
    printf "\nmin %.1f ms | median %.1f ms | mean %.1f ms | %.1f M ops/s\n", min, med, mean, mops
    printf "(one op = one enqueue + one dequeue pair; throughput = ITER / median)\n"
  }'

rm -f bench_mpmc
