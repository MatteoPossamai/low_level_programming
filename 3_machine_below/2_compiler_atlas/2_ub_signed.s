## Top = default -O2  (signed overflow = UB)
## Bottom = -O2 -fwrapv (signed overflow defined as wrap)


## always_true(int a)  ->  a + 1 > a
## -----------------------------------

# DEFAULT
_Z11always_truei:
    mov     eax, 1              ## folded to constant
    ret

# WRAPV
_Z11always_truei:
    cmp     edi, 2147483647     ## a == INT_MAX ?
    setne   al                  ## false only when a wraps
    ret


## divide_by_two(int x)  ->  (x * 2) / 2
## -------------------------------------

# DEFAULT
_Z13divide_by_twoi:
    mov     eax, edi            ## folded to x
    ret

# WRAPV
_Z13divide_by_twoi:
    lea     eax, [rdi+rdi]      ## x*2
    sar     eax                 ## /2  (identity broken by wrap)
    ret


## sum_scaled(const int* a, int n)  ->  for i<n: s += a[i*2]
## ---------------------------------------------------------

# DEFAULT (5 insns/iter, pointer-stride)
_Z10sum_scaledPKii:
    test    esi, esi
    jle     .L55
    movsxd  rsi, esi
    xor     eax, eax
    lea     rcx, [rdi+rsi*8]             ## end = a + 2n ints
.L54:
    movsxd  rdx, DWORD PTR [rdi]
    add     rdi, 8                       ## ptr += 2 ints (strength-reduced)
    add     rax, rdx
    cmp     rdi, rcx
    jne     .L54
    ret

# WRAPV (6 insns/iter, index sign-extended each iter)
_Z10sum_scaledPKii:
    test    esi, esi
    jle     .L55
    add     esi, esi
    xor     eax, eax
    xor     edx, edx
.L54:
    movsxd  rcx, eax                     ## widen i to 64-bit EACH iter
    add     eax, 2
    movsxd  rcx, DWORD PTR [rdi+rcx*4]
    add     rdx, rcx
    cmp     esi, eax
    jne     .L54
    mov     rax, rdx
    ret
