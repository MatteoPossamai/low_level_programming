---
name: compile-cpp-reproducibly
description: Compile or propose compilation commands for C++ targets in this repository. Use when choosing g++ flags for development, release, sanitizer, profiling, or benchmark builds, or when recording a build so it can be reproduced. Do not use for implementing core algorithms or changing build systems unless explicitly requested.
---

# Compile C++ Reproducibly

1. Read the repository `AGENTS.md` and
   `3_tools_n_specs/GCC_FLAGS.md` before choosing flags.
2. Inspect the target source and any existing build or benchmark command. Preserve
   the project's current structure unless the user asks to change it.
3. Identify the build's single primary purpose:
   - everyday development
   - release
   - ASan and UBSan testing
   - TSan testing
   - `perf` profiling
   - benchmarking
   - assembly inspection
4. Select the smallest matching recipe from `GCC_FLAGS.md`. Explain any added or
   removed flag briefly.
5. Keep incompatible configurations separate. In particular, never combine ASan
   and TSan, and never use sanitizer results as performance measurements.
6. Before compiling, report the compiler identity with `g++ --version` and show
   the exact command. Do not hide warnings.
7. Compile and run only what the user requested. Report the exact command, compiler
   version, outcome, and relevant warnings or errors so the build can be repeated.

Use `-O2` as the performance baseline. Add `-O3`, `-march=native`, `-flto`,
`-Ofast`, or individual optimisation flags only when the user requests an
experiment or measurements justify one. Record the CPU model whenever
`-march=native` or benchmark results are involved.

When helping the author debug compilation or concurrency problems, prefer a nudge
and evidence over directly rewriting core logic.
