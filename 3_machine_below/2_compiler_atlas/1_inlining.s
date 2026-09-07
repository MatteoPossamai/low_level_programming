## Inner loops only. Full asm in build/1_inlining.s
## Bench: BM_inline 124 ns, BM_no_inline 407 ns (per 1024-elem sum)


## no_inline_square(int) - standalone symbol emitted
## --------------------------------------------------
_Z16no_inline_squarei:
    imul    edi, edi
    mov     eax, edi
    ret

## always_inline helper has NO symbol - body pasted into caller.


## BM_inline inner loop - autovectorised, 4 ints/iter
## --------------------------------------------------
.L45:
    movdqu  xmm0, XMMWORD PTR [rax]      ## load 4 ints
    add     rax, 16                      ## advance 4 ints
    movdqa  xmm1, xmm0                   ## \
    pmuludq xmm1, xmm0                   ##  | 32x32 -> 64-bit squares
    psrlq   xmm0, 32                     ##  | (pmuludq of xmm gives 2 x 64)
    pmuludq xmm0, xmm0                   ##  |
    pshufd  xmm1, xmm1, 8                ##  | pack low 32-bit halves
    pshufd  xmm0, xmm0, 8                ##  | back into one xmm
    punpckldq xmm1, xmm0                 ## /
    paddd   xmm2, xmm1                   ## accumulate 4 squares
    cmp     rax, rbx
    jne     .L45
## Multiply body visible -> vectoriser fired. No call.


## BM_no_inline inner loop - scalar, 1 elem/iter
## ---------------------------------------------
.L21:
    mov     edi, DWORD PTR [rdx]         ## load 1 int (arg for call)
    add     rdx, 4                       ## advance 1 int
    call    _Z16no_inline_squarei        ## <-- optimisation barrier
    add     ecx, eax                     ## sum += ret
    cmp     rdx, rbx
    jne     .L21
## Call in loop = no vectorisation, no hoisting, no reordering
## across the call. Caller-saved regs must be treated as clobbered.
