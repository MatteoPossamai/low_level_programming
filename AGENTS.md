# Learning Direction — Context for Agents

This file gives context to any agent or tool working in this repo. Read it fully before proposing plans, generating code, or answering questions about direction.

## What this repo is

A self-directed systems programming curriculum, organised in quarters. Usually one book + one project per quarter; Q6 deliberately has two interleaved tracks and can run longer. Projects are the deliverables; reading supports them.

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
- Standard toolchain: gcc/clang with `-std=c++20 -O2 -g -Wall -Wextra -Wshadow`, ThreadSanitizer for concurrent code, perf for profiling, godbolt for inspecting codegen. See [the `g++` flag and build recipe reference](0_tools_n_specs/GCC_FLAGS.md).
- For compilation tasks, follow the repository workflow in [`compile-cpp-reproducibly`](skills/compile-cpp-reproducibly/SKILL.md).
- When he asks for diffs first, show the diff and wait for an explicit go ("apply", "go", "do it"). One issue at a time when he says so.
- Do only what was asked. No unrequested API additions (e.g. he rejected an added `try_enqueue`); flag the need instead and let him decide.
- He may delegate boilerplate once he has done the pattern himself (mirrored buy/sell logic, protocol builders, standard linked-list ops). Still his call each time.
- When he says "no comments", add none; only remove the ones he asked to remove.

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

**Q4 - Matching engine (complete for the agreed scope)**

Blocks 1-4 are complete. Block 5 (drop-copy) was removed from scope. Blocks 6-7 are complete: latency benchmarks, perf investigation, one targeted optimization, and the project write-up. The write-up is in [`ENGINE.md`](ENGINE.md) and may still be edited. Full plan lives in Notion. Block summary:

- **Block 1** — Price-level data structure chosen by measurement (map vs sorted vector vs flat array vs intrusive lists, pool allocator underneath). Atlas method. Read WK Selph's order book post ONLY after forming own hypothesis.
  - **Status**: three structures implemented (`sorted_vector`, intrusive `list`, `tick_offset` sparse array), one shared Google-Benchmark workload harness. Winner by measurement at real-book scales: **tick_offset**. Full write-up + numbers in [`4_order_book_proj/DATA_STRUCTURE.md`](4_order_book_proj/DATA_STRUCTURE.md). Project layout, conventions, open items in [`4_order_book_proj/README.md`](4_order_book_proj/README.md).
  - **Pool allocator**: done — `src/allocator.hpp`, mmap-backed fixed-size free list (typed `Pointer *free_head`), single-threaded by design, GoogleTest suite in `tests/allocator_test.cpp` under ASan+UBSan. Used by the engine for order blocks.
  - **Not done**: flat vector index replacing `unordered_map` for cancel, bitset for best-idx walk, op-mix parametrisation.
- **Block 2** — NASDAQ OUCH 5.0 codec (spec on nasdaqtrader.com), zero-copy, fuzzed, round-trip tested. FIX 4.2 parser is a STRETCH ONLY (text-vs-binary comparison artifact); must not eat the quarter.
  - **Status**: `src/messages.hpp` — views (decode) and builders (encode) for Enter, Cancel, Accepted, Canceled, Executed, offsets checked against the spec PDF (UserRefNum is 4 bytes). Round-trip check in `benchmark/codec_bench.cpp`.
  - **Not done**: fuzzing; variable-length optional appendages (Appendage Length is written as 0); message types beyond the five above.
- **Block 3** — Design doc first, then matching core: price-time priority, limit/market/cancel. Modify = stretch. Self-trade prevention = out of scope. Deterministic replay input, invariant/property tests.
  - **Status**: design in `4_order_book_proj/DESIGN_DOC.md`; engine in `src/engine.hpp` (tick-offset book, intrusive FIFO per level, pool-allocated blocks). Orders keyed by `(account << 32 | UserRefNum)` per spec. Sends Accepted first, two Executed per match (`R` incoming / `A` resting, resting price, shared match number), Canceled (`I` market rest, `Z` pool full, `U` user). Cancel = reduce to new open size, 0 removes. `process()` / `run()` entry points.
  - **Tests**: `tests/engine_test.cpp` — 15 exact input→output scenarios + a 200k-message random property test, ASan+UBSan clean. Mutation-checked: 5 injected bugs all caught.
  - **Baseline** (`benchmark/engine_bench.cpp`, 50/48/2 enter/cancel/market, -O2, unpinned, CPU scaling on): ~12.3M msg/s (~81 ns/msg) at depth 1k, 11.5M/s at 50k. This throughput benchmark is separate from Block 6's latency measurements.
  - **Known POC limits (accepted)**: price used raw as array index (inbound must reject price >= BUFFER_SIZE and 0); `MARKETPRICE` 0x7FFFFFFF assumed on both sides; sides `T`/`E` throw; cancel qty read as new open size (spec wording ambiguous); no inbound validation stage yet.
- **Block 4** — DONE. Transport interface, epoll backend, and io_uring backend are implemented and benchmarked head-to-head. The loopback comparison and interpretation are in `4_order_book_proj/benchmark/README.md`.
  - **Queues already in place** (`src/queues.hpp` + `queues_impl.hpp`): inbound MPSC (Vyukov-style, producers yield, consumer pauses) carrying `InboundMessage{account, raw bytes}` — the receiver must attach the connection's OUCH account; outbound SPSC carrying `OutboundMessage{account, raw bytes}`. Both blocking-only by his choice.
- **Block 5** — REMOVED FROM SCOPE: drop-copy fan-out.
- **Block 6** — DONE for the current benchmark scope. `internal_engine_bench` measures engine processing through outbound enqueue; `wire_to_wire_bench` measures loopback client round trips. Both report p50/p99/p99.9/max. `perf stat`, topdown, and sampled call stacks were used to investigate CPU cost. Profiling localized long scans across empty price levels; tracking resting-order counts and stopping scans when a side is empty reduced the internal benchmark's recorded overall p99.9 from about 4.08 ms to 424 ns, and max from about 4.55 ms to 6.2 μs. Results and method are in `4_order_book_proj/benchmark/README.md`.
- **Block 7** — DONE: project write-up is in [`ENGINE.md`](ENGINE.md); the document may still be edited.

**Q5 - Networking (complete for the agreed scope)**

Matteo confirmed completion of the agreed networking reading and experiments.
The quarter is complete. Code, experiment notes, and
captures live in [`5_network/`](5_network/), indexed by
[`5_network/README.md`](5_network/README.md).

- **Reading scope completed (author-reported)**: kernel receive path, TCP behaviour including Nagle / delayed ACK, UDP multicast for market data, and architectural understanding of kernel bypass and polling vs interrupts. This does not imply a DPDK/Onload implementation or a full-book read.
- **Built**: length-prefixed TCP framing (`framing/`); one epoll receiver assembling data separately for multiple TCP connections (`multi_thread_rx/`); UDP multicast publisher/subscribers with sequence numbers, membership changes, and gap detection (`udp_mcast/`). Despite its folder name, `multi_thread_rx` uses one receiver event loop and a sender thread.
- **Inspection**: TCP and UDP packet captures alongside the experiments; shared packet-inspection notes in [`0_tools_n_specs/WIRESHARK.md`](0_tools_n_specs/WIRESHARK.md).
- **Scope limits**: multicast gap detection is implemented; recovery/retransmission is discussed, not implemented. These are learning experiments, not a production networking stack or physical-network latency benchmarks.
- **Key mental models**: TCP is a byte stream and preserves no application message boundaries; per-connection receive state; readiness-based multiplexing; multicast fan-out and application-level loss detection.

## Current position - Q6 planned: SIMD and storage

Matteo chose two interleaved C++ tracks. Implementation has not started.
The plan and Notion-compatible checkboxes are in
[`6_simd_n_storage/README.md`](6_simd_n_storage/README.md);
hardware recommendations and dated purchasing notes are in
[`6_simd_n_storage/HARDWARE.md`](6_simd_n_storage/HARDWARE.md).

- **SIMD**: scalar image references, compiler auto-vectorisation, handwritten Arm Neon kernels, stencils/reductions, and a Pi camera application using pretrained face detection/recognition. Compare kernel and end-to-end costs; no assumed speedup or frame rate.
- **Storage**: a small single-process/writer persistent key-value store, append/read, replayed index, explicit durability, record-integrity checks, failure recovery, compaction, and measurements. SQL, replication, and multi-key transactions are deferred.
- **Theory is required**: Algorithmica SIMD; selected Computer, Enhance! Part 3/5 lessons; Arm Neon documentation; all Database Internals Part I chapters 1-7; SQLite durability and Linux fsync/rename semantics; OpenCV/Pi camera concepts. Readings are assigned to blocks, not an optional appendix.
- **Rhythm**: SIMD 1 -> Storage 1 -> SIMD 2 -> Storage 2 -> SIMD 3 -> Storage 3 -> Camera -> Write-up. Finish the current block before switching; the quarter may run longer than earlier ones.
- **Hardware recommendation, not a purchase**: Pi 5 4 GB, cooling, official supply, microSD, Camera Module 3 Standard. Budget is unconfirmed. The first blocks can start on the current computer; the Pi is needed for the Neon block.
- **Important distinctions**: Casey's x86 examples and uops.info timings do not directly predict Arm timings; killing a process does not simulate OS/power failure; reading about B-trees/LSM trees does not commit the project to implementing them.
- **Integration**: SIMD/camera and storage remain independently testable deliverables. Using the store for profiles/events is stretch work after both work separately. Neural-network training and GPU/NPU programming are deferred.

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
- **Q5 completed as networking**: the former networking roadmap is complete for the agreed scope; artifacts are in `5_network/`.
- **Q6 selected**: SIMD/image processing and persistent storage, alternating complete blocks with required theory. Casey's course supports selected CPU/SIMD topics rather than committing to the full 8086 simulator course. Rust via Mara Bos remains deferred.

## Repos and artifacts

- `github.com/MatteoPossamai/low_level_programming` — allocators, perf guide, and later work. This repo.
- Performance atlas: `3_machine_below/` — CMake + Google Benchmark, one folder per experiment with hypothesis/result/why READMEs.
- Networking: `5_network/` — completed Q5 experiments, notes, and packet captures; start with `5_network/README.md`.
- Q6: `6_simd_n_storage/` - planned SIMD/camera and storage tracks; README contains the copyable plan, HARDWARE.md the purchasing snapshot.
- Q1-Q4 plans live in Notion as checkbox markdown.

## First interaction style

- Q1-Q5 are complete for their agreed scope. Q6 is planned as SIMD and storage; help with the requested block or planning detail. Do not restart the curriculum-selection discussion unless asked.
- Don't summarise this document back at him.
- Don't propose restructuring the curriculum. The plan is good.
- What he needs: execution support, reviews, nudges, honest measurement interpretation.
