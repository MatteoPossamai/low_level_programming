# SIMD - Continuation 2

## Compiler instructions

The compiler will only try to vectorize if the code is compiled with `-02` or more.
`-fno-vec` tells the compiler to completely skip vectorization.
`-vec-report` to have the report on compilation of what has been vectorized.
Clang uses `'-Rpass=.*'` instead.

## Headers

`<immintrin.h>` is a subset of `<x86intrin.h>`, but the latter is only GCC, CLANG. It has some extra
exclusively x86 operations, but most of it is already covered anyway. Prefer the first for portability,
the second for completeness.

## Type Suffixes

| Suffix | Meaning | Element type | Bits per element | Example intrinsic |
|---|---|---|---|---|
| `ps` | Packed Single | `float` | 32 | `_mm256_add_ps` |
| `pd` | Packed Double | `double` | 64 | `_mm256_add_pd` |
| `ss` | Scalar Single | `float` | 32 | `_mm_add_ss` |
| `sd` | Scalar Double | `double` | 64 | `_mm_add_sd` |
| `epi8` | Extended Packed Integer | Signed integer | 8 | `_mm256_add_epi8` |
| `epi16` | Extended Packed Integer | Signed integer | 16 | `_mm256_add_epi16` |
| `epi32` | Extended Packed Integer | Signed integer | 32 | `_mm256_add_epi32` |
| `epi64` | Extended Packed Integer | Signed integer | 64 | `_mm256_add_epi64` |
| `epu8` | Extended Packed Unsigned | Unsigned integer | 8 | `_mm256_avg_epu8` |
| `epu16` | Extended Packed Unsigned | Unsigned integer | 16 | `_mm256_avg_epu16` |
| `epu32` | Extended Packed Unsigned | Unsigned integer | 32 | `_mm256_min_epu32` |
| `epu64` | Extended Packed Unsigned | Unsigned integer | 64 | `_mm512_min_epu64` |
| `si32` | Scalar Integer | Signed integer | 32 | `_mm_cvtsi32_si128` |
| `si64` | Scalar Integer | Signed integer | 64 | `_mm_cvtsi64_si128` |
| `si128` | SIMD Integer | Integer bit vector | 128 total | `_mm_loadu_si128` |
| `si256` | SIMD Integer | Integer bit vector | 256 total | `_mm256_loadu_si256` |
| `si512` | SIMD Integer | Integer bit vector | 512 total | `_mm512_loadu_si512` |
| `su32` | Scalar Unsigned Integer | Unsigned integer | 32 | No standard `su32`-suffixed intrinsic |
| `su64` | Scalar Unsigned Integer | Unsigned integer | 64 | No standard `su64`-suffixed intrinsic |

## Intel Guide

- [Link guide](https://www.intel.com/content/dam/develop/external/us/en/documents/31848-compilerautovectorizationguide.pdf)

### Loops vectoralizable

To be able to be vectoralizable, they should be:

1. Countable: Number of times the loop is performed must NOT be known at compile time, but
  be known and constant at runtime on the first iteration. Hence, must not be data dependent
  once the iteration started
2. Single entry, single exit: no `if (cond) break`. Loop should be linear in that sense.
3. Straight line code: no branching and switching, unless this can be implemented with masking
4. Inner most loop: cannot be done on outer loops, unless these are unrolled by other optimizations
5. No functions call: exceptions are functions that can be inlined

### Obstacles

1. Non contiguous memory access: if data are scattered, the compiler needs more load operations rather than
  one packed load operation. This makes it slower to perform and the compiler rarely does it;
2. Data dependencies: if a previous operation has impact on the following cannot be vectorized. Operations must be
  independent from one another to be vectorized effectively (or correctly at all)

### Guides

1. Prefer simple, single entry single exit, constant iterations `for` loops rather than complex ones.
2. No branching, straight code when possible
3. Avoid data dependencies
4. Prefer array notation to pointer notation: aliasing might be an issue, so make it restricted
5. Use loop index for array subscription to make it clearer how to vectorize it all
6. Efficient memory access:
  - No stride
  - No indirect aliasing
  - Align data to 16/32/64 bytes boundary for `SSE`/`AVX`/`AVX512` instructions.
7. Align data: if there are some miss-match, loads/stores will have to be unaligned (`u`) incurring
  in performance penalties.
8. Structure of Array (SoA) over Array of Structures (AoS): better locality and contiguous data
9. Use smallest data type possible: allows for more packing and more parallel operation
10. Try avoid mixed types loops
11. Avoid operations not supported by SIMD, such as 80 bits integer and modulo (`%`) operation
12. `#pragma vector always` to tell the compiler to, regardless of cost analysis to vectorize
13. `#pragma ivdep` ignore potential dependencies
14. `#pragma novector` when numbers of iterations is known to be very little and not worth the time thinking about vectorizing
15. `#pragma loop count(n)` to suggest typical number of iterations
16. `#pragma vector always` vectorize all of time time when safe, if performance improve or not
17. `#pragma vector align` assert alignment of data
18. `#pragma vector nontemporal`: data used only once, bypasses cache
19. `restricted` if possible. If not, it needs a runtime check for contiguous access

### Reports

Return loops vectorized and why other loops where not vectorized.
