# Learning Direction — Context for Agents

This file gives context to any agent or tool working in this repo. Read it before proposing plans, generating code, or answering questions about direction.

## What this repo is

A self-directed systems programming curriculum, organised in quarters. Each quarter = one book + one project. The project is the deliverable; the book supports it.

## Completed so far

**Q1 — Systems foundations (C)**

- Book: _Computer Systems: A Programmer's Perspective_ (Bryant & O'Hallaron), chapters 3, 5, 6, 9
- Built: four memory allocators in C (bump, implicit free-list, coalescing with boundary tags, explicit free-list), plus an experimental SoA-layout allocator (headers and data in separate buffers) built after profiling
- Profiled everything with `perf` (stat, record, report, annotate), traced cache misses to specific instructions, understood skid/attribution, wrote a reusable profiling guide (see mem_allocator/perf README)
- Key mental models acquired: memory hierarchy and cache lines, why access patterns dominate performance, instruction-level parallelism and dependency chains (multiple accumulators), loop reordering, virtual memory / page faults / mmap

**Q2 — Modern C++ and concurrency (complete)**

- Books: _A Tour of C++_ 3rd ed (Stroustrup), _C++ Concurrency in Action_ 2nd ed (Williams)
- Built: RAII rewrite of Q1 allocator, threading exercises, SPSC lock-free ring buffer (TSAN-clean, benchmarked vs mutex version), bounded MPMC lock-free queue based on the Vyukov design, and a thread pool with futures/promises, cooperative shutdown, per-worker queues, and work stealing
- Benchmarked the MPMC queue, including the effects of cache-line padding and weaker atomic memory orderings, and documented its sequence-number protocol and memory-model reasoning
- Key mental models acquired: RAII/ownership, move semantics, the C++ memory model, acquire/release semantics, atomics, sequence-number-based slot ownership, false sharing and cache-line contention, cooperative cancellation, futures/promises, task scheduling, and work stealing

**Q3 — The machine below (complete)**

- Books: _The Art of Writing Efficient Programs_ (Fedor Pikus), _Effective Modern C++_ (Scott Meyers)
- Built (all in `3_machine_below/`):
  - **Performance atlas** — six Google Benchmark experiments isolating hardware effects: branch misprediction (sorted vs unsorted), `cmov` vs branch, multiple accumulators / ILP, stride vs cache-line utilisation, false sharing, `__builtin_prefetch`
  - **Compiler atlas** — six experiments isolating compiler decisions, each with annotated inner-loop asm: inlining as vectorisation enabler, signed-overflow UB as an optimisation contract (`-fwrapv` diff), auto-vectoriser preconditions (`__restrict__`, calls in loop), missing `noexcept` on move ctor forcing vector realloc copies, virtual vs `final` devirtualisation vs CRTP, Itanium zero-cost exception model
  - **Event poll** — two small C++20 echo servers: epoll (readiness-based, level-triggered) and io_uring (completion-based, SQE/CQE rings, per-operation request state via `user_data`)
- Key mental models acquired: readiness vs completion I/O models, syscall batching, why the real cost of virtual dispatch is missed inlining, UB as the price of optimisation, what makes the auto-vectoriser fire or bail, exception happy-path vs thrown-path asymmetry, and (from Meyers) modern C++ idiom — `auto`, move semantics pitfalls, `noexcept`, smart-pointer ownership, perfect forwarding

## Direction from here

The goal is deep expertise in systems programming, with C++ as the primary language. The priority order is:

1. **Language-agnostic mental models first.** Cache behaviour, memory models, operation reordering, how compilers transform code, how the OS provides memory and scheduling — these transfer across C, C++, Rust, and everything else. When choosing between "learn more C++ syntax" and "learn how the machine actually works," prefer the machine.
2. **High confidence in C++ second.** The aim is to read and write modern C++ (17/20) comfortably — templates, RAII, the standard library, concurrency primitives — before adding other languages.
3. **Rust deferred, deliberately.** Rust is planned but intentionally postponed until C++ fundamentals are solid, so its language-specific concepts (ownership/borrowing as a type system, Send/Sync) land against a strong baseline rather than blurring into general learning.

## Rules for agents working here

- Core logic is written by hand by the author. Agents help with: test generation, debugging assistance, explaining concepts, reviewing code. Agents should NOT write core data structures or algorithms unless explicitly asked.
- When the author is debugging, prefer nudges and questions over answers. The diagnostic process is part of the training.
- Prefer explanations that connect code to hardware (cache lines, memory ordering, syscalls) over surface-level fixes.
- Projects should stay small, self-contained, and benchmarkable. Correctness first, then measurement, then optimisation — in that order.
- Standard toolchain: gcc/clang with `-std=c++20 -O2 -g -Wall -Wextra -Wshadow`, ThreadSanitizer for concurrent code, perf for profiling, godbolt for inspecting codegen. See [the `g++` flag and build recipe reference](3_tools_n_specs/GCC_FLAGS.md) when choosing or explaining compiler options.
- For compilation tasks, follow the repository workflow in [`compile-cpp-reproducibly`](skills/compile-cpp-reproducibly/SKILL.md).
