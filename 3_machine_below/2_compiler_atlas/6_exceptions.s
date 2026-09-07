## Full asm in build/6_exceptions.s
##
## Bench:
##   BM_ec_success      0.395 ns    check return value  (small cost, always)
##   BM_ec_fail         0.494 ns    check return value + branch
##   BM_throw_success   0.296 ns    ZERO cost happy path (faster than ec!)
##   BM_throw_fail       639 ns     ~1600x throw_success. µs range.
##
## Zero-cost model: throw_success beats ec_success because ec path always
## pays `test + jne` on return. throw path has no per-call check on success.


## caller_ec(int fail)  - error code path
## --------------------------------------
_Z9caller_eci:
    endbr64
    call    _Z9helper_eci
    test    eax, eax             ## check return: always paid
    jne     .L108
    mov     eax, 42              ## success: return 42
    ret
.L108:
    mov     eax, -1              ## error: return -1
    ret
## No LSDA, no unwind info needed. Simple structure.


## caller_try(int fail)  - exception path, HAPPY only
## --------------------------------------------------
_Z10caller_tryi:
    .cfi_personality ...         ## unwinder metadata
    .cfi_lsda .LLSDA4295
    endbr64
    sub     rsp, 8               ## frame (needed so unwinder can walk)
.LEHB10:                         ## "here be exceptions" range START
    call    _Z12helper_throwi    ## <-- just a call. No check on return.
.LEHE10:                         ## range END
    mov     eax, 42              ## success path: no branch, no test
    add     rsp, 8
    ret

## No test-and-branch on return. Happy path is 3 instructions (call,
## mov, ret) + frame setup. Catch handler lives ELSEWHERE (.text.unlikely).


## caller_try.cold  - the catch handler, in a cold section
## -------------------------------------------------------
_Z10caller_tryi.cold:
.L110:
    mov     rdi, rax             ## rax = exception ptr from unwinder
    call    __cxa_begin_catch
    call    __cxa_end_catch
    mov     eax, -1
    jmp     back_to_hot_return


## .gcc_except_table  - the map runtime uses on throw
## --------------------------------------------------
.LLSDA4295:
    ## records: PC range [.LEHB10 - .LEHE10] -> landing pad .L112
    ## Not read unless an exception actually propagates.


## Takeaway
## --------
## Zero-cost = happy path pays NOTHING for exception support (throw_success
## beats ec_success). Cost lives in two places, both invisible on happy path:
##   1. Cold catch code moved to .text.unlikely
##   2. Unwind tables (.gcc_except_table, .eh_frame) - only read by
##      libunwind DURING a throw
##
## When throw fires: allocate exception object, walk stack via .eh_frame,
## run personality func per frame, destroy locals, jump to matching landing
## pad. ~1600x cost of the check. Amortises only if throws are truly rare.
