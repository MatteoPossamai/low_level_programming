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

**Q2 — Modern C++ and concurrency (in progress, near complete)**

- Books: _A Tour of C++_ 3rd ed (Stroustrup), _C++ Concurrency in Action_ 2nd ed (Williams)
- Built: RAII rewrite of Q1 allocator, threading exercises, SPSC lock-free ring buffer (TSAN-clean, benchmarked vs mutex version)
- Remaining: MPMC bounded queue (Vyukov design), write-up
- Key mental models acquired: RAII/ownership, move semantics, the C++ memory model, acquire/release semantics, atomics, why compilers and CPUs reorder operations and how to constrain them

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
- Standard toolchain: gcc/clang with `-std=c++20 -O2 -g -Wall -Wextra -Wshadow`, ThreadSanitizer for concurrent code, perf for profiling, godbolt for inspecting codegen.
