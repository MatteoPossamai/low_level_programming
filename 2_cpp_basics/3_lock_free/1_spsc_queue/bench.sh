#!/usr/bin/env bash
# Benchmark harness for the SPSC queue implementations.
# Compiles each .cpp with -DITER, times whole-process runs, writes RESULTS.md.
set -euo pipefail
cd "$(dirname "$0")"

ITER="${ITER:-10000000}"
RUNS="${RUNS:-10}"
CXX="${CXX:-g++}"
FLAGS="-O2 -std=c++17 -pthread -DITER=${ITER}"

IMPLS=(spsc_ring spsc_linked spsc_lock)

echo "Compiling (${FLAGS})..."
for impl in "${IMPLS[@]}"; do
  "$CXX" $FLAGS "${impl}.cpp" -o "bench_${impl}"
done

ROWS=""
for impl in "${IMPLS[@]}"; do
  echo "Benchmarking ${impl}..."
  "./bench_${impl}" # warmup, not timed

  times_ms=()
  for ((i = 0; i < RUNS; i++)); do
    start=$(date +%s%N)
    "./bench_${impl}"
    end=$(date +%s%N)
    times_ms+=("$(awk -v s="$start" -v e="$end" 'BEGIN { printf "%.1f", (e - s) / 1e6 }')")
  done

  stats=$(printf '%s\n' "${times_ms[@]}" | sort -n | awk -v iter="$ITER" '
    { a[NR] = $1; sum += $1 }
    END {
      min = a[1]
      med = (NR % 2) ? a[(NR + 1) / 2] : (a[NR / 2] + a[NR / 2 + 1]) / 2
      mean = sum / NR
      mops = iter / (med * 1000)
      printf "%.1f|%.1f|%.1f|%.1f", min, med, mean, mops
    }')
  ROWS+="| ${impl} | ${stats//|/ | } |"$'\n'
done

CPU=$(lscpu | awk -F': +' '/Model name/ { print $2; exit }')
CORES=$(nproc)
CXX_VER=$("$CXX" --version | head -1)

cat >RESULTS.md <<EOF
# SPSC queue benchmark results

- Date: $(date +%F)
- CPU: ${CPU} (${CORES} cores)
- Compiler: ${CXX_VER}
- Flags: \`${FLAGS}\`
- Iterations per run: ${ITER}, timed runs per impl: ${RUNS} (plus 1 warmup)
- Timing: wall clock of the whole process (producer + consumer threads)

| Implementation | Min (ms) | Median (ms) | Mean (ms) | Throughput (M ops/s) |
|---|---|---|---|---|
${ROWS}
Throughput = ITER / median time. One "op" = one enqueue + one dequeue pair.

Reproduce: \`./bench.sh\` (override with e.g. \`ITER=1000000 RUNS=5 ./bench.sh\`)
EOF

rm -f bench_spsc_ring bench_spsc_linked bench_spsc_lock
echo "Done. Results in RESULTS.md"
