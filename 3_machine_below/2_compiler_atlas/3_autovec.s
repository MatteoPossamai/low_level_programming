## Inner loops only. Full asm in build/3_autovec.s
## Compiled with -O2 -march=native (AVX2 available -> 32-byte ymm).
##
## Bench (per 4096-elem call):
##   sum_vec       103 ns   vectorised (8 ints/iter)
##   add_alias     726 ns   scalar   (compiler couldn't prove no-alias)
##   add_restrict  155 ns   vectorised (8 ints/iter, ~5x vs add_alias)
##   sum_call     1619 ns   scalar   (call in loop = no vec)


## sum_vec(const int* a, int n)   -> reduction
## -------------------------------------------
.L19:
    vpaddd  ymm1, ymm1, YMMWORD PTR [rax]    ## 8 ints added into accumulator
    add     rax, 32                          ## advance 8 ints (32 bytes)
    cmp     rax, rdx
    jne     .L19
## Then horizontal reduce ymm1 -> single int with vextracti128 + vpsrldq chain.


## add_alias(int* dst, const int* src, int n)   -> scalar!
## -------------------------------------------------------
.L31:
    mov     ecx, DWORD PTR [rsi+rax]         ## load 1 int from src
    add     ecx, 1
    mov     DWORD PTR [rdi+rax], ecx         ## store 1 int to dst
    add     rax, 4                           ## advance 1 int
    cmp     rax, rdx
    jne     .L31
## No SIMD. Compiler cannot rule out dst == src, or dst overlapping src.
## Would need runtime alias check + fallback -> deemed unprofitable here.


## add_restrict(int* __restrict dst, const int* __restrict src, int n)
## -------------------------------------------------------------------
.L37:
    vpaddd  ymm0, ymm1, YMMWORD PTR [rsi+rax]  ## src[i..i+7] + 1
    vmovdqu YMMWORD PTR [rdi+rax], ymm0        ## store to dst[i..i+7]
    add     rax, 32                            ## advance 8 ints
    cmp     rax, rdx
    jne     .L37
## __restrict__ promises "dst and src don't overlap" -> vectoriser fires.
## ymm1 pre-loaded with 8 copies of 1 (via vpcmpeqd + vpsrld splat trick).


## helper(int)   -> tiny standalone symbol
## ---------------------------------------
_Z6helperi:
    imul    edi, edi
    lea     eax, 1[rdi]
    ret


## sum_call(const int* a, int n)   -> scalar, call in loop
## -------------------------------------------------------
.L49:
    mov     edi, DWORD PTR [rdx]             ## load a[i] into arg reg
    add     rdx, 4
    call    _Z6helperi                       ## <-- optimisation barrier
    add     ecx, eax                         ## sum += helper(a[i])
    cmp     rdx, rsi
    jne     .L49
## Call in loop kills vectorisation. Would need helper inlined OR
## a #pragma omp declare simd + -fopenmp-simd variant of helper.
