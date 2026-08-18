# SPSC queue benchmark results - acquire/release memory ordering

Same benchmark as `RESULTS.md`, after replacing the default seq_cst atomics in
`spsc_ring.cpp` and `spsc_linked.cpp` with explicit acquire/release/relaxed
orderings. `spsc_lock` is unchanged and acts as a control.

- Date: 2026-08-18
- CPU: 13th Gen Intel(R) Core(TM) i7-13700 (24 cores)
- Compiler: g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0
- Flags: `-O2 -std=c++17 -pthread -DITER=10000000`
- Iterations per run: 10000000, timed runs per impl: 10 (plus 1 warmup)
- Timing: wall clock of the whole process (producer + consumer threads)

| Implementation | Min (ms) | Median (ms) | Mean (ms) | Throughput (M ops/s) |
|---|---|---|---|---|
| spsc_ring | 412.8 | 523.3 | 540.9 | 19.1 |
| spsc_linked | 5084.4 | 5655.3 | 5604.3 | 1.8 |
| spsc_lock | 683.9 | 838.1 | 884.2 | 11.9 |

Throughput = ITER / median time. One "op" = one enqueue + one dequeue pair.

Versus seq_cst (`RESULTS.md`): ring 13.6 -> 19.1 M ops/s (~40% faster, seq_cst
stores were full fences). Linked 1.9 -> 1.8 M ops/s (no change, heap
allocation per node dominates). Lock 11.9 -> 11.9 (control, as expected).

Reproduce: `./bench.sh` (override with e.g. `ITER=1000000 RUNS=5 ./bench.sh`)
