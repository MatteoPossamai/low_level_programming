## Full asm in build/4_noexcept.s
##
## Bench (per push_back of 1024 elems, ~10 reallocs):
##   BM_push_fragile          32203 ns   (Fragile: move NOT noexcept -> COPIED on realloc)
##   BM_push_movable          15841 ns   (Movable: move noexcept -> moved on realloc)  ~2x
##   BM_push_movable_reserve  14440 ns   (reserve upfront -> no reallocs at all)
##
## Missing `noexcept` on Fragile::Fragile(Fragile&&) forces vector to
## use COPY for reallocations, to preserve strong exception safety.
## std::string(64) allocates on the heap -> copy = malloc + memcpy.
## Move = pointer swap. Hence the 2x gap.


## caller_a(int)   - throwing allowed
## ----------------------------------
_Z8caller_ai:
    endbr64
    jmp     _Z9may_throwi        ## tail call (jmp, not call)
## No frame, no LSDA. If may_throw throws, unwinder walks through
## caller_a as if it were transparent - no cleanup needed here.


## caller_b(int) noexcept   - must guard the boundary
## --------------------------------------------------
_Z8caller_bi:
    .cfi_personality 0x9b, DW.ref.__gxx_personality_v0
    .cfi_lsda 0x1b, .LLSDA4367
    endbr64
    sub     rsp, 8               ## stack frame set up
    call    _Z9may_throwi        ## real call, not jmp
    add     rsp, 8
    ret

.section .gcc_except_table       ## LSDA: "any exception here -> terminate"
.LLSDA4367:
    .byte 0xff, 0xff, 0x1
    ...

## Counter-intuitive result: noexcept caller is LARGER than throwing
## caller here! Because noexcept is a PROMISE the compiler must enforce
## at the ABI boundary: if may_throw throws, this frame must catch
## and call std::terminate. Cannot tail-call.
##
## Where noexcept actually saves code = at *callers of* noexcept funcs:
## they can drop landing-pad machinery around the call. Payoff compounds
## in deep call chains, not in two-line leaf callers like these.
