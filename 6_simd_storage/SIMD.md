# SIMD

Single instruction multiple data.

- [Source](https://en.algorithmica.org/hpc/simd/)
- [Intel Resource](https://www.intel.com/content/dam/develop/external/us/en/documents/31848-compilerautovectorizationguide.pdf)
- [Cheat-Sheet](https://db.in.tum.de/~finis/x86%20intrinsics%20cheat%20sheet%20v1.0.pdf)

## Introduction

`avx2` extension for SIMD. GCC assumes that at most CPU has 128b registers
while CPUs with `avx2` and `avx-512` are higher. Telling compiler necessary to improve
performance automatically.

Both GCC and Clang default to the x86-64 baseline (SSE2, 128-bit vectors). Wider vector instruction sets must be explicitly enabled.

| Instruction set | Vector width | GCC / Clang flag |
|---|---|---|
| SSE2 (x86-64 baseline) | 128 bits | Default |
| AVX | 256 bits | `-mavx` |
| AVX2 | 256 bits | `-mavx2` |
| AVX-512 | 512 bits | `-mavx512f` |
| Native CPU | CPU-dependent | `-march=native` |

### Architecture Levels

Both compilers support portable x86-64 architecture levels:

| Architecture | SIMD capabilities | Compiler flag |
|---|---|---|
| x86-64 | SSE2 | `-march=x86-64` |
| x86-64-v2 | SSE3, SSSE3, SSE4.1, SSE4.2 | `-march=x86-64-v2` |
| x86-64-v3 | AVX, AVX2, FMA | `-march=x86-64-v3` |
| x86-64-v4 | AVX-512 (F, BW, CD, DQ, VL) | `-march=x86-64-v4` |

## Intrinsics

Can see what CPU supports by `cat /proc/cpuinfo` or `__builtin_cpu_supports("sse")`.

Can tell compiler with:

```c++
#pragma GCC target("avx2")
#pragma GCC optimize("O3")
#include <x86intrin.h>
#include <bits/stdc++.h>
```

Registers for SIMD:

- SSE (1999) added 16 128-bit registers called xmm0 through xmm15.
- AVX (2011) added 16 256-bit registers called ymm0 through ymm15.
- AVX512 (2017) added1 16 512-bit registers called zmm0 through zmm15.

Intrinsics are C style function to do assembly like instructions. Example:

```c++
double a[100], b[100], c[100];
for (int i = 0; i < 100; i += 4) {
    __m256d x = _mm256_loadu_pd(&a[i]);
    __m256d y = _mm256_loadu_pd(&b[i]);
    __m256d z = _mm256_add_pd(x, y);
    _mm256_storeu_pd(&c[i], z);
}
```

If the array is not divisible by the size of the register, can either pad it or
do the last iteration in a scalar way. First preferred usually.

Typical intrinsic format: `_mm<size>_<action>_<type>`. Useful [cheat-sheet](https://db.in.tum.de/~finis/x86%20intrinsics%20cheat%20sheet%20v1.0.pdf)
These are very CPU specific. A bit more portable is:

```c++
typedef int v8si __attribute__ (( vector_size(32) ));
// type ^   ^ typename          size in bytes ^
```

Although this does not guarantee perfect portability. This way though you can have
same syntax as the normal scalar operations.

## Moving data

While element wise operations are easy to understand, there are operations for moving data
whose goal is to make the price to make it all work worth it.

### Aligned load and store

Operations are `load`, `loadu`, `store`, `storeu`, where `u` means unaligned. First if
data fits in cache line, otherwise not aligned, but in that case is slower. To
guarantee alignment use `alignas` with the dimension required.

```c++
alignas(32) int a[64];
```

Get elements from a index of the register can be done with `_mm256_extract_epi32`. And the
opposite with `_mm256_insert_epi32`. To fill an array with constants there is intrinsic
`__m256 iota = _mm256_setr_epi32(0, 1, 2, 3, 4, 5, 6, 7);`. You can set all to be the same
with `__m256i v = _mm256_set1_epi32(42);`.

Gather and scatter operations to get data that are not contiguous in memory, but in that
case those are limited by memory bandwidth rather than CPU execution time.

## Reductions (or folding)

Computing the value of associative operation over a range of elements. For reductions such
as summing a lot of elements together can rather have multiple accumulators in a SIMD
register and leverage it in order to move the performance up. Similar to when in C use
instruction level parallelism. And can have even more, with more vector accumulators
based on how many SIMD register are available on the CPU.

Can also use instruction to sum contiguous and then reduce them all together. This might
be faster in some cases, but generally is equivalent.

## Masking and Blending

Conditional code is not straightforward in SIMD. Requires some branch-less programming.

To check for dimension can use `_mm256_cmpgt_epi32` that masks as -1 in case is larger in the first and 0 otherwise, and `_mm256_blendv_epi8`, which blends (combines) the values of two vectors based on the provided mask.

So generate a mask, and then blend together two arrays given the mask so that it follows
the mask. In the example can get the one that are below 50 and otherwise make 0, and then
sum them all together.

Finding can be done by generating a mask and then reading it after comparing the baseline. This is quite useful but has some more overhead in some cases.

To count occurrences can generate a mask for the given number, and then sum all together.

## Auto Vectorization

Not always the compiler is able to vectorize automatically loops and operations
due to some technical difficulties. You can help it with Contract Programming.

To allow simpler vectorization, make clear to the compiler the size of arrays, and
pad them yourself to be multiple of cache lines, to make it more obvious to it. Also
restrict parameter to avoid memory aliasing, so that the compiler can be sure and optimize
it well. Try to align all in multiples of the cache and register sizes.

Read ASM or use `-fopt-info-vec-optimized` to verify weather vectorization happened.

SPMD = single program multiple data is when write code that is scalar but it works on
vectorized data as well. Specific to languages and APIs.

Compile with `-S` to see the assembly only, or `-save-temps=obj` to see all. Vectorized operations start with `v` usually.
