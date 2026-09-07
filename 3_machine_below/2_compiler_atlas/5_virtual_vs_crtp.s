## Full asm in build/5_virtual_vs_crtp.s
##
## Bench (per sum over 4096 circles):
##   BM_virtual   6502 ns   indirect call per elem, no inline
##   BM_final     1618 ns   devirtualised, inlined arith         ~4x
##   BM_crtp      1585 ns   inlined + auto-vectorised (AVX2)     ~4x


## sum_virtual  - vtable dispatch per elem
## ---------------------------------------
.L24:
    mov     rdi, QWORD PTR [rbx]         ## p = *iter (load Circle*)
    vmovsd  QWORD PTR 8[rsp], xmm1       ## spill accumulator: xmm1 caller-saved across call
    add     rbx, 8
    mov     rax, QWORD PTR [rdi]         ## rax = vptr (first 8 bytes of object)
    call    [QWORD PTR 16[rax]]          ## <-- INDIRECT CALL through vtable slot 2
    vmovsd  xmm1, QWORD PTR 8[rsp]       ## reload accumulator
    cmp     rbp, rbx
    vaddsd  xmm1, xmm1, xmm0
    jne     .L24
## Per iter: load ptr, load vptr, indirect call, spill+reload accumulator.
## No inlining -> area body opaque. No vectorisation. Branch predictor
## must guess indirect target every iter.


## sum_final  - devirtualisation fired (CircleFinal is `final`)
## ------------------------------------------------------------
.L122:
    mov     rdx, QWORD PTR [rax]         ## p = *iter
    add     rax, 8
    cmp     rcx, rax
    vmovsd  xmm1, QWORD PTR 8[rdx]       ## load p->r (skip vptr at offset 0)
    vmulsd  xmm0, xmm1, xmm3             ## r * 3.14159
    vmulsd  xmm0, xmm0, xmm1             ## * r
    vaddsd  xmm2, xmm2, xmm0             ## accumulate
    jne     .L122
## No call. `final` proved exact type -> compiler devirtualised, inlined
## the multiply. Scalar though: ptrs scatter across heap, no SIMD load.


## sum_crtp  - static dispatch + SIMD (CircleC by value, contiguous)
## ----------------------------------------------------------------
.L222:
    vmovupd ymm2, YMMWORD PTR [rax]      ## load 4 doubles (4 CircleC.r values)
    add     rax, 32
    cmp     rax, rcx
    vmulpd  ymm1, ymm2, ymm4             ## SIMD: r * 3.14159 (4 lanes)
    vmulpd  ymm1, ymm1, ymm2             ## SIMD: * r
    vaddsd  xmm0, xmm0, xmm1             ## horizontal reduce
    vunpckhpd xmm2, xmm1, xmm1
    vextractf128 xmm1, ymm1, 0x1
    vaddsd  xmm0, xmm0, xmm2
    ...
## CRTP + values-in-vector = contiguous doubles -> auto-vectorised.
## 4 elems per iter. ymm4 preloaded with broadcast of 3.14159.


## Takeaway
## --------
## Virtual dispatch cost = mostly missed INLINING, not the call itself.
## Once the body is 2 multiplies, inlining saves ~4x.
## `final` on a class unlocks devirtualisation and matches CRTP for
## simple bodies. CRTP wins further only if data layout allows SIMD
## (contiguous values). vector<Base*> scatters memory even after devirt.
