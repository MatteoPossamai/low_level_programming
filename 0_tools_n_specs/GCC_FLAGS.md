# Useful `g++` Flags

A compact reference for developing, testing, profiling, and inspecting C++20
programs with GCC.

## Stages

```text
source → preprocessor → compiler/optimiser → assembler → linker → executable
```

Tags used below:

- **PP** — preprocessor
- **C** — compilation and optimisation
- **A** — assembly generation
- **L** — linking
- **R** — affects runtime behaviour
- **D** — diagnostics

## Flag reference

| Flag | Stage | What it does | When to use it | Important trade-off |
| --- | --- | --- | --- | --- |
| `-std=c++20` | C | Selects ISO C++20. | Every build. | State it explicitly instead of relying on the compiler default. |
| `-o FILE` | L | Names the output file. | Every build. | Without it, the executable is normally called `a.out`. |
| `-c` | C, A | Produces an object file without linking. | Projects with multiple source files. | A separate link command is then required. |
| `-pthread` | PP, C, L, R | Enables POSIX threading support. | Programs using `std::thread` or pthreads. | Use it when compiling and linking. |
| `-Wall` | D | Enables a broad set of useful warnings. | Every development build. | It does not literally enable all GCC warnings. |
| `-Wextra` | D | Enables more useful warnings. | Every development build. | May expose unused parameters and similar issues. |
| `-Wpedantic` | D | Warns about extensions outside the selected ISO standard. | Normal portable C++ builds. | GNU-specific code may warn intentionally. |
| `-Wshadow` | D | Warns when a declaration hides another name. | Every development build. | Can be noisy in code that deliberately reuses names. |
| `-Wconversion` | D | Warns about implicit conversions that may change a value. | Occasional audit builds. | Noisy, especially around integer sizes. Do not add casts blindly. |
| `-Wsign-conversion` | D | Warns about signed/unsigned conversions. | Auditing indexes, sizes, and binary data. | Particularly noisy around `std::size_t`. |
| `-Werror` | D | Treats enabled warnings as errors. | CI after the warning policy is stable. | A new compiler warning can break the build. |
| `-O0` | C | Disables most optimisation. | Simple source-level debugging. | Produces unrealistic and often very slow code. |
| `-Og` | C | Optimises while trying to preserve debugging quality. | Everyday debug builds. | Generated instructions still need not follow source order exactly. |
| `-O1` | C | Applies moderate optimisation. | Sanitizer builds. | A useful balance between realistic execution and readable traces. |
| `-O2` | C | Applies broad, generally reliable optimisation. | Release builds and profiling. | Best default performance baseline. |
| `-O3` | C | Adds more aggressive inlining, loop, and vector transformations. | An experiment after measuring `-O2`. | Larger code can be slower because of instruction-cache pressure. |
| `-Ofast` | C, R | Allows optimisations that can relax language and floating-point rules. | Rare numerical experiments. | Can change program results; it is not simply “faster `-O3`.” |
| `-g` | C, A, L | Adds debug information. | Debug, sanitizer, release-symbol, and profiling builds. | Makes files larger but normally does not slow execution. |
| `-g3` | PP, C, A | Adds extra debug information, including macros. | Development and difficult debugging. | Produces larger debug information than `-g`. |
| `-DNDEBUG` | PP, R | Disables standard `assert` calls. | Release builds when this is intended. | Removes runtime checks; assertions must never contain required side effects. |
| `-D_GLIBCXX_ASSERTIONS` | PP, R | Enables inexpensive checks in libstdc++. | Development and sanitizer builds. | GCC/libstdc++-specific. |
| `-D_GLIBCXX_DEBUG` | PP, R | Enables extensive container and iterator checking. | A dedicated STL-debug build. | Slow and ABI-incompatible with normal libstdc++ container builds. |
| `-fsanitize=address` | C, L, R | Detects many out-of-bounds, use-after-free, and memory-lifetime errors. | Memory-correctness testing. | Large runtime and memory overhead; do not combine with TSan. |
| `-fsanitize=undefined` | C, L, R | Detects many forms of undefined behaviour. | Usually combined with ASan. | It cannot detect every kind of undefined behaviour. |
| `-fsanitize=thread` | C, L, R | Detects data races. | Concurrent correctness testing. | Very slow and changes thread scheduling; use a separate build. |
| `-fno-omit-frame-pointer` | C, A | Keeps frame pointers for easier stack unwinding. | Sanitizer and `perf` builds. | Uses a register and may slightly change performance. |
| `-march=native` | C, A | Uses instructions available on the current CPU. | Local performance experiments. | The executable may not run on a different or older CPU. |
| `-flto` | C, L | Allows optimisation across source-file boundaries. | Measured release experiments. | Slower linking and must also be used during the link step. |
| `-S` | C, A | Stops after compilation and writes assembly. | Inspecting generated machine instructions. | Does not create an executable. |
| `-masm=intel` | A | Prints x86 assembly in Intel syntax. | With `-S`, if Intel syntax is preferred. | Only changes assembly presentation. |
| `-fopt-info-vec` | C, D | Reports successful vectorisation. | Investigating loop optimisation. | A report does not prove the change matters to total runtime. |
| `-fopt-info-vec-missed` | C, D | Explains some missed vectorisation opportunities. | Understanding dependencies or aliasing in loops. | Not every missed optimisation is worth fixing. |

## Recipes

### Everyday development

```bash
g++ -std=c++20 -Og -g3 \
  -Wall -Wextra -Wpedantic -Wshadow \
  -D_GLIBCXX_ASSERTIONS \
  -pthread source.cpp -o program
```

### Strict warning audit

```bash
g++ -std=c++20 -O2 -g \
  -Wall -Wextra -Wpedantic -Wshadow \
  -Wconversion -Wsign-conversion \
  -pthread source.cpp -o program
```

Understand each conversion before adding a cast merely to silence a warning.

### Memory and undefined-behaviour testing

```bash
g++ -std=c++20 -O1 -g3 \
  -Wall -Wextra -Wpedantic -Wshadow \
  -fsanitize=address,undefined \
  -fno-omit-frame-pointer \
  -pthread source.cpp -o program_asan
```

### Data-race testing

```bash
g++ -std=c++20 -O1 -g3 \
  -Wall -Wextra -Wpedantic -Wshadow \
  -fsanitize=thread \
  -fno-omit-frame-pointer \
  -pthread source.cpp -o program_tsan
```

Keep TSan separate from ASan. A clean run covers only the executions and
interleavings that occurred.

### Release build

```bash
g++ -std=c++20 -O2 -g -DNDEBUG \
  -Wall -Wextra -Wpedantic -Wshadow \
  -pthread source.cpp -o program
```

Keeping `-g` makes production crashes and profiling output easier to interpret.

### `perf` profiling build

```bash
g++ -std=c++20 -O2 -g \
  -fno-omit-frame-pointer \
  -pthread source.cpp -o program_perf
```

Profile optimised code. `-O0` changes code layout, memory traffic, inlining, and
timing too much to represent the release program.

### Local CPU experiment

```bash
g++ -std=c++20 -O3 -g -march=native \
  -pthread source.cpp -o program_native
```

Compare it against `-O2`. Record the compiler version, flags, and CPU model.

## Inspecting compiler output

### See the assembly

```bash
g++ -std=c++20 -O2 -S -masm=intel source.cpp -o source.s
```

Useful for seeing loads, stores, branches, inlining, vector instructions,
atomics, and fences.

To inspect the final executable instead:

```bash
objdump -drwC -Mintel program
```

`objdump` is a separate binutils tool. `-C` demangles C++ names.

### Ask about vectorisation

```bash
g++ -std=c++20 -O2 \
  -fopt-info-vec -fopt-info-vec-missed \
  source.cpp -o program
```

This can show whether dependencies, aliasing, or control flow prevented a loop
from being vectorised.

### See which optimisations a level enables

```bash
g++ -O2 -Q --help=optimizers
```

Compare the output for `-O0`, `-Og`, `-O2`, and `-O3`. The exact contents of
these optimisation levels can change between GCC versions and CPU targets.

### Discover available flags

```bash
g++ --help=warnings
g++ --help=optimizers
g++ --help=target
g++ --version
```

These are useful when this guide points you toward a category and you want to
explore further.

## Dangerous or commonly misunderstood

- **`-Wall` is not every warning.** It is a useful curated group. Add other
  warnings only when they enforce something relevant.
- **`-Werror` does not find more bugs.** It only changes warning policy, and a
  compiler upgrade can introduce a warning that breaks the build.
- **`-O0` is not safer C++.** If optimisation breaks correct-looking code,
  investigate undefined behaviour, data races, lifetimes, and uninitialised data.
- **`-O3` is not automatically faster.** Extra inlining and unrolling can hurt
  instruction-cache behaviour. Compare it with `-O2`.
- **`-Ofast` can change numerical results.** Use it only when its relaxed rules
  fit the program's numerical contract.
- **`-march=native` is machine-specific.** Good for a local experiment, poor as
  an unnoticed default.
- **Sanitizers alter execution.** Use them for correctness, never for benchmarks.
- **ASan and TSan require separate builds.** Their instrumentation is not
  compatible.
- **`-g` and optimisation are independent.** `-O2 -g` is normal and desirable
  for realistic profiling.
- **`_GLIBCXX_DEBUG` changes ABI.** Do not mix its containers across object files
  compiled with different settings.
- **Avoid tuning individual optimisation passes first.** Begin with `-O2`, form
  a hardware-level hypothesis, change one option, and measure.

## Default starting point

```bash
g++ -std=c++20 -Og -g3 \
  -Wall -Wextra -Wpedantic -Wshadow \
  -D_GLIBCXX_ASSERTIONS \
  -pthread source.cpp -o program
```

Use a separate recipe when the goal changes: sanitizer testing, release,
profiling, or a performance experiment.
