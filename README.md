# Low Level Programming

Self-educating repository to learn concepts of low level programming, C/C++.
Organised in quarters: one book + one project each. Full curriculum history and
direction in [AGENTS.md](AGENTS.md).

## Map

| Part | Directory | What it holds |
| --- | --- | --- |
| Q1 | [`1_mem_allocator`](1_mem_allocator/) | Four memory allocators in C (bump, implicit/explicit free-list, coalescing) plus an SoA-layout experiment, all profiled with `perf` |
| Q2 | [`2_cpp_basics`](2_cpp_basics/) | Modern C++ and concurrency: RAII allocator rewrite, SPSC ring buffer, Vyukov MPMC queue, work-stealing thread pool |
| Q3 | [`3_machine_below`](3_machine_below/) | Performance atlas (hardware effects) and compiler atlas (codegen decisions), both benchmarked with annotated asm; epoll and io_uring echo servers |
| Q4 | [`4_order_book_proj`](4_order_book_proj/) | Matching engine: price-level structure chosen by measurement, pool allocator, OUCH 5.0 codec, lock-free queues, price-time matching core with scenario tests and a baseline benchmark |
| — | [`0_tools_n_specs`](0_tools_n_specs/) | Tooling references (compiler flags, build recipes) |

## Books covered

- _Computer Systems: A Programmer's Perspective_ (Bryant & O'Hallaron), ch. 3, 5, 6, 9
- _A Tour of C++_ 3rd ed (Stroustrup)
- _C++ Concurrency in Action_ 2nd ed (Williams)
- _The Art of Writing Efficient Programs_ (Pikus)
- _Effective Modern C++_ (Meyers)

## References

- [Useful `g++` flags and build recipes](0_tools_n_specs/GCC_FLAGS.md)
- [Reproducible C++ compilation skill](skills/compile-cpp-reproducibly/SKILL.md)
