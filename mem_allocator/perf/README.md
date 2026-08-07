# Performance improvement

## Goal

Using `perf`, find one optimization to apply to the `implicit_free_list`
allocator.

## TLDR profiling sequence

1. Write the code to proifile and a profiler benchmark code (make sure is not skewed)
2. Run Simple perf stat to have an idea. Repeat more specific metrics

```shell
 gcc -O2 perf/perf_binary.c implicit_free_list/allocator.c && sudo perf stat ./a.out
```

3. Dive deeper into cases that seems the event that is limiting execution, get ASM

```shell
[1] sudo perf record -e cycles -g ./a.out
[2] sudo perf record -e cache-misses -g ./a.out
[3] sudo perf report [--stdio]
```

4. Read ASM and compare with the code to find the problematic cluster
5. Try to understand the bottleneck of the cluster and why happens
6. Optimize it (change code or change data structure when required)
7. Benchmark to verify improvement
8. Repeat until not happy

## Command

### Build and see in UI

```shell
gcc -O2 perf/perf_binary.c implicit_free_list/allocator.c && sudo perf stat ./a.out

gcc -O2 perf/perf_binary.c implicit_free_list/allocator.c
sudo perf stat -e cycles,instructions,cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses,dTLB-loads,dTLB-load-misses,page-faults,branch-misses ./a.out
```

### Record

```shell
[1] sudo perf record -e cycles -g ./a.out
[2] sudo perf record -e cache-misses -g ./a.out
[3] sudo perf report [--stdio]
[4] sudo perf annotate --stdio alloc_malloc # ASM for a specific function
```

With the above `[1]` and `[2]` (`[3]` to visualize output) we figured out that
`alloc_malloc` wascausing the vast majority of all the cache misses, and so we
went down to read the ASM and find bottlenecks with the last one.

From the assembly we were able to trace down which functions were the one where
the CPU stall more of the time due to the events we were looking into. Usually the
main thing is cache misses anyway.

## Metrics to compute

1. IPC: Compute instructions / cycles. High (3+) = compute-busy. Low (<1) = waiting
2. Topdown: backend_bound high = memory or execution unit stalls. frontend_bound high = instruction fetch stalls.
3. Cache ratios: L1 for first level, LLC if you are leaving cache for DRAM
4. Absolute counts for OS-level things. Page faults, context switches, TLB misses

## Compiler arguments

- `%rdi` — first argument
- `%rsi` — second argument
- `%rdx` — third argument
- `%rcx`, `%r8`, `%r9` — ...

## ASM

```ASM
    0.03 :   159e:   mov    %r8,%rax
    0.00 :   15a1:   cmp    %rcx,%r8
    0.00 :   15a4:   jae    15c9 <alloc_malloc+0x49>
    0.00 :   15a6:   cs nopw 0x0(%rax,%rax,1)
    2.89 :   15b0:   cmpq   $0x0,0x8(%rax)
   71.55 :   15b5:   mov    (%rax),%rdx
   20.10 :   15b8:   jne    15bf <alloc_malloc+0x3f>
    0.00 :   15ba:   cmp    %rsi,%rdx
    0.00 :   15bd:   jae    15f0 <alloc_malloc+0x70>
    0.72 :   15bf:   lea    0x10(%rax,%rdx,1),%rax
```

The move is from what is on the left to what is on the right,
so `mov %r8, %rax` = `%rax = %r8`.

### Perf leaving

```ASM
    2.89 :   15b0:   cmpq   $0x0,0x8(%rax)
   71.55 :   15b5:   mov    (%rax),%rdx
   20.10 :   15b8:   jne    15bf <alloc_malloc+0x3f>
```

In this case, the `71.55%` does not mean that there is a miss on that
line 70% of the time, it means that that line waits for 70% of the
time for a cache missed from prev line. So the miss is actually in
the previous line, and this means that loading `%rax` is the main
bottleneck here.

In this case, this means that the header reading is the main bottleneck,
given the fact that there is a random jump and then cache cannot
be warm or used, going every time to memory. Algorithm is correct, just cannot be
better with this data structure.

> So when you read perf annotate output, the rule of thumb is: look at a small
> cluster of instructions with high sample counts together, and blame the load
> (or the loop-carried memory access) at the top of that cluster.
