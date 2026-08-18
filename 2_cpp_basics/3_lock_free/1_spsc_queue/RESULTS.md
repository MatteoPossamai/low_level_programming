# SPSC queue benchmark results

- Date: 2026-08-18
- CPU: 13th Gen Intel(R) Core(TM) i7-13700 (24 cores)
- Compiler: g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0
- Flags: `-O2 -std=c++17 -pthread -DITER=10000000`
- Iterations per run: 10000000, timed runs per impl: 10 (plus 1 warmup)
- Timing: wall clock of the whole process (producer + consumer threads)

| Implementation | Min (ms) | Median (ms) | Mean (ms) | Throughput (M ops/s) |
|---|---|---|---|---|
| spsc_ring | 647.0 | 732.8 | 751.7 | 13.6 |
| spsc_linked | 4888.3 | 5391.8 | 5282.4 | 1.9 |
| spsc_lock | 802.1 | 839.0 | 920.7 | 11.9 |

Throughput = ITER / median time. One "op" = one enqueue + one dequeue pair.

Reproduce: `./bench.sh` (override with e.g. `ITER=1000000 RUNS=5 ./bench.sh`)
