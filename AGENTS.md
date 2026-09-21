# Learning Direction — Context for Agents

This file gives context to any agent or tool working in this repo. Read it fully before proposing plans, generating code, or answering questions about direction.

## What this repo is

A self-directed systems programming curriculum, organised in quarters. Each quarter = one book + one project. The project is the deliverable; the book supports it.

## Who you're working with

Matteo — software engineer moving into quant development (C++/low-latency direction), starting a quant dev role in Singapore. This curriculum is ~9 months in and is the vehicle for that transition.

- Strong and improving: C, modern C++ (17/20), perf profiling, concurrency, memory models.
- Background before this curriculum: Python back-office/infra work, CS degree, one uni compilers course (FSA/grammars level), read _Inside the Machine_ and _Operating Systems: Three Easy Pieces_.
- Baseline ~6 hrs/week on this, more when he can. Quarters take 3-4+ months in practice. That is fine — never compress learning to hit dates.

## How to work with him

Rules learned over months. Follow them.

- Dry, direct, actionable. No padding, no cheerleading.
- Core logic (data structures, algorithms) is written by hand by the author. Agents help with: test generation, debugging hints, concept explanations, reviews, plans. Never write core logic unless explicitly asked.
- During debugging: give NUDGES, not answers. Ask "what do you think X is?" before revealing. Only unblock fully when he asks directly. The diagnostic process is part of the training.
- He responds well to reasoned pushback. Push back when he is wrong; explain why.
- Reviews: prioritised lists (bugs first, then design, then nits) with honest verdicts.
- Plans: Notion-compatible markdown — blocks (not weeks), checkboxes, dry goals, explicit "done when" flags, resource links at the end.
- Hypothesis-before-measurement is core to how he learns: predict, measure, explain the gap. Preserve this pattern in every experiment/review.
- Prefer explanations that connect code to hardware (cache lines, memory ordering, syscalls) over surface-level fixes.
- Projects stay small, self-contained, benchmarkable. Correctness first, then measurement, then optimisation — in that order.
- Standard toolchain: gcc/clang with `-std=c++20 -O2 -g -Wall -Wextra -Wshadow`, ThreadSanitizer for concurrent code, perf for profiling, godbolt for inspecting codegen. See [the `g++` flag and build recipe reference](3_tools_n_specs/GCC_FLAGS.md).
- For compilation tasks, follow the repository workflow in [`compile-cpp-reproducibly`](skills/compile-cpp-reproducibly/SKILL.md).

## Completed so far

**Q1 — Systems foundations (C)**

- Books: _Computer Systems: A Programmer's Perspective_ (Bryant & O'Hallaron) chapters 3, 5, 6, 9; Drepper's _What Every Programmer Should Know About Memory_.
- Built: four memory allocators in C (bump, implicit free-list, coalescing with boundary tags, explicit free-list), plus an experimental SoA-layout allocator (headers and data in separate buffers) built after profiling.
- Profiled everything with `perf` (stat, record, report, annotate), traced cache misses to specific instructions, understood skid/attribution, wrote a reusable profiling guide (see `mem_allocator/perf` README).
- Key mental models acquired: memory hierarchy and cache lines, why access patterns dominate performance, instruction-level parallelism and dependency chains (multiple accumulators), loop reordering, virtual memory / page faults / mmap.

**Q2 — Modern C++ and concurrency**

- Books: _A Tour of C++_ 3rd ed (Stroustrup), _C++ Concurrency in Action_ 2nd ed (Williams). Watched Sutter's _atomic<> Weapons_.
- Built: RAII rewrite of Q1 allocator, threading exercises, SPSC lock-free ring buffer (TSAN-clean, benchmarked vs mutex version), bounded MPMC lock-free queue based on the Vyukov design, thread pool with futures/promises, cooperative shutdown, per-worker queues, work stealing.
- Benchmarked the MPMC queue including cache-line padding and weaker atomic memory orderings; documented the sequence-number protocol and memory-model reasoning.
- Key mental models acquired: RAII/ownership, move semantics, C++ memory model, acquire/release semantics, atomics, sequence-number-based slot ownership, false sharing and cache-line contention, cooperative cancellation, futures/promises, task scheduling, work stealing.

**Q3 — The machine below**

- Books: _The Art of Writing Efficient Programs_ (Fedor Pikus), _Effective Modern C++_ (Scott Meyers, cover to cover).
- Built (all in `3_machine_below/`):
  - **Performance atlas** — Google Benchmark experiments isolating hardware effects: branch misprediction (sorted vs unsorted), `cmov` vs branch, multiple accumulators / ILP, stride vs cache-line utilisation, false sharing, `__builtin_prefetch`.
  - **Compiler atlas** — experiments isolating compiler decisions, each with annotated inner-loop asm: inlining as vectorisation enabler, signed-overflow UB as an optimisation contract (`-fwrapv` diff), auto-vectoriser preconditions (`__restrict__`, calls in loop), missing `noexcept` on move ctor forcing vector realloc copies, virtual vs `final` devirtualisation vs CRTP, Itanium zero-cost exception model.
  - **Event poll** — two small C++20 echo servers: epoll (readiness-based, level-triggered) and io_uring (completion-based, SQE/CQE rings, per-operation request state via `user_data`).
- Method per experiment: question → written hypothesis → minimal pair → defend against optimiser → verify asm → measure → explain mechanism in own words.
- Tooling absorbed: modern CMake, Google Benchmark (`DoNotOptimize`/`ClobberMemory`), sanitizers by default, clang-format, clangd, cachegrind, godbolt as daily driver, `-fopt-info-vec`.
- Key mental models acquired: readiness vs completion I/O models, syscall batching, why the real cost of virtual dispatch is missed inlining, UB as the price of optimisation, what makes the auto-vectoriser fire or bail, exception happy-path vs thrown-path asymmetry, and (from Meyers) modern C++ idiom — `auto`, move semantics pitfalls, `noexcept`, smart-pointer ownership, perfect forwarding.

## Current position — Q4: matching engine

Just starting. Full plan lives in Notion. Block summary:

- **Block 1** — Price-level data structure chosen by measurement (map vs sorted vector vs flat array vs intrusive lists, pool allocator underneath). Atlas method. Read WK Selph's order book post ONLY after forming own hypothesis.
- **Block 2** — NASDAQ OUCH 5.0 codec (spec on nasdaqtrader.com), zero-copy, fuzzed, round-trip tested. FIX 4.2 parser is a STRETCH ONLY (text-vs-binary comparison artifact); must not eat the quarter.
- **Block 3** — Design doc first, then matching core: price-time priority, limit/market/cancel. Modify = stretch. Self-trade prevention = out of scope. Deterministic replay input, invariant/property tests.
- **Block 4** — Transport interface FIRST (~3 functions), then epoll backend, then io_uring backend, swappable, benchmarked head-to-head. Engine consumes via his Q2 SPSC queue. Expect epoll ≈ io_uring on loopback; explaining why is the finding.
- **Block 5** — Drop-copy fan-out. Slow consumer must not affect hot path; policy = drop/disconnect laggards, documented.
- **Block 6** — Latency measurement: two views (RDTSC internal spans + wire-to-wire loopback), percentiles only (p50/p99/p99.9/max), HDR histogram, one perf-driven optimisation, one tail-latency spike investigated to root cause.
- **Block 7** — Write-up / blog post — flagship portfolio piece.

**Sequencing rule**: each stage end-to-end before the next. Shed from the tail, never the middle.

Talks queued: Carl Cook _When a Microsecond Is an Eternity_ (before Block 3), David Gross _Trading at Light Speed_, Pikus _Branchless Programming_, Alexandrescu _Speed Is Found in the Minds of People_, Skarupke mutex/spinlock talk.

## Direction from here

Deep expertise in systems programming, C++ as the primary language. Priority order:

1. **Language-agnostic mental models first.** Cache behaviour, memory models, operation reordering, how compilers transform code, how the OS provides memory and scheduling — these transfer across C, C++, Rust, and everything else. When choosing between "learn more C++ syntax" and "learn how the machine actually works," prefer the machine.
2. **High confidence in C++ second.** Read and write modern C++ (17/20) comfortably — templates, RAII, the standard library, concurrency primitives — before adding other languages.
3. **Rust deferred, deliberately.** Rust is planned but intentionally postponed until C++ fundamentals are solid, so ownership/borrowing/Send/Sync land against a strong baseline rather than blurring into general learning.

## Decisions already made (do not re-litigate unless asked)

- **C++ first, Rust later.** Rationale above. Mara Bos _Rust Atomics and Locks_ is the planned entry point when Rust starts.
- **Master's degree deprioritised** in favour of self-study + portfolio route. OMSCS remains a fallback if structure/credential become useful later.
- **Dragon Book rejected** for compiler learning. Codegen understanding comes via godbolt experiments + Pikus + Godbolt/Carruth talks instead.
- **Q5 leading candidate**: Casey Muratori's _Performance-Aware Programming_ (computerenhance.com, paid). Overlaps ~60% with Q1/Q3; the value is the other 40% — 8086 simulator approach, uops.info cycle estimation, paging deep-cuts, hand-written SIMD (his biggest remaining gap). Alternative Q5: Rust via Mara Bos. Decide after Q4 with the engine in hand.
- **Networking roadmap sketched**: kernel receive path → TCP gotchas (Nagle / delayed-ACK) → UDP multicast for market data → architectural (not hands-on) understanding of kernel bypass (DPDK/Onload, busy-polling vs interrupts). Beej → TLPI ch. 56-61 when the time comes.

## Repos and artifacts

- `github.com/MatteoPossamai/low_level_programming` — allocators, perf guide, and later work. This repo.
- Performance atlas: `3_machine_below/` — CMake + Google Benchmark, one folder per experiment with hypothesis/result/why READMEs.
- Q1-Q4 plans live in Notion as checkbox markdown.

## First interaction style

- Ask where he is in Q4, then help with exactly that block.
- Don't summarise this document back at him.
- Don't propose restructuring the curriculum. The plan is good.
- What he needs: execution support, reviews, nudges, honest measurement interpretation.
