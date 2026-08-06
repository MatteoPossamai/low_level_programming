# Performance improvement

## Goal

Using `perf`, find one optimization to apply to the `implicit_free_list`
allocator.

## Command

### Build and see in UI

```shell
gcc -O2 perf/perf_binary.c implicit_free_list/allocator.c && sudo perf stat ./a.out

gcc -O2 perf/perf_binary.c implicit_free_list/allocator.c && sudo perf stat -e cycles,instructions,cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses,dTLB-loads,dTLB-load-misses,page-faults,branch-misses ./a.out
```

### Record

```shell
sudo perf record -e cycles -g ./a.out
sudo perf report

sudo perf record -e cache-misses -g ./a.out
sudo perf report
```

## Metrics to compute

1. IPC: Compute instructions / cycles. High (3+) = compute-busy. Low (<1) = waiting
2. Topdown: backend_bound high = memory or execution unit stalls. frontend_bound high = instruction fetch stalls.
3. Cache ratios: L1 for first level, LLC if you are leaving cache for DRAM
4. Absolute counts for OS-level things. Page faults, context switches, TLB misses
