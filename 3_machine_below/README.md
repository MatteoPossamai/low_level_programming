# The machine below

Two small experiment collections that probe what the machine and the compiler
actually do to your code. Each experiment is one `.cpp` file (Google Benchmark)
with results at the top; some also carry an annotated `.s` file with the
inner-loop asm.

## [1 - Performance atlas](1_performance_atlas/)

Hardware effects. Same source, different access patterns or layouts, big
runtime differences.

1. [`1_branch_missprediction`](1_performance_atlas/1_branch_missprediction.cpp) - sorted vs unsorted input flipping branch predictor accuracy.
2. [`2_cmov_vs_branch`](1_performance_atlas/2_cmov_vs_branch.cpp) - `cmov` is constant-time; a branch beats it when predictable, catastrophic when not.
3. [`3_accumulators`](1_performance_atlas/3_accumulators.cpp) - multiple accumulators break the reduction dependency chain, unlock ILP.
4. [`4_stride`](1_performance_atlas/4_stride.cpp) - increasing stride wastes cache lines, then thrashes them.
5. [`5_false_sharing`](1_performance_atlas/5_false_sharing.cpp) - two threads writing distinct atomics on the same cache line ping-pong ownership.
6. [`6_prefetching`](1_performance_atlas/6_prefetching.cpp) - `__builtin_prefetch` helps random gather, wasted on sequential scan.

## [2 - Compiler atlas](2_compiler_atlas/)

Compiler decisions. Same source, different flag or annotation, different asm.
Annotated inner-loop asm in each `<n>_<topic>.s`.

1. [`1_inlining`](2_compiler_atlas/1_inlining.cpp) - `always_inline` vs `noinline`; inlining unlocks vectorisation, not just call elimination.
2. [`2_ub_signed`](2_compiler_atlas/2_ub_signed.cpp) - signed overflow as UB contract; `-fwrapv` diff shows which optimisations UB paid for.
3. [`3_autovec`](2_compiler_atlas/3_autovec.cpp) - what makes the auto-vectoriser fire or bail; `__restrict__`, calls in loop, `-fopt-info-vec-all`.
4. [`4_noexcept`](2_compiler_atlas/4_noexcept.cpp) - missing `noexcept` on move ctor forces `std::vector` to copy on realloc (~2x slowdown).
5. [`5_virtual_vs_crtp`](2_compiler_atlas/5_virtual_vs_crtp.cpp) - virtual vs devirt (`final`) vs CRTP; real cost is missed inlining, not the indirect branch.
6. [`6_exceptions`](2_compiler_atlas/6_exceptions.cpp) - Itanium zero-cost model; happy path free (beats error code), thrown path ~1600x slower.

## Build

Each atlas has its own `CMakeLists.txt`. Standard flow:

```
cd <atlas>/ && cmake -S . -B build && cmake --build build
./build/<experiment>                                    # run bench
cmake --build build --target <experiment>_asm           # dump asm (compiler_atlas only)
```

Requires Google Benchmark installed system-wide.
