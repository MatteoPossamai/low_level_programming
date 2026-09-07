// Compile:
// ----------------------------------------------------------
// Flag              Purpose
// ----------------------------------------------------------
// -O2               baseline
// (-fno-exceptions  would disable EH entirely; not used here)
// ----------------------------------------------------------
//
// Results:
// -------------------------------------------------------------------
// Benchmark            Time         Notes
// -------------------------------------------------------------------
// BM_ec_success        0.395 ns     always pays test+jne on return
// BM_ec_fail           0.494 ns     same, branch taken
// BM_throw_success     0.296 ns     ZERO cost happy path (faster!)
// BM_throw_fail         639 ns      ~1600x throw_success
// -------------------------------------------------------------------
// See 6_exceptions.s for caller_ec vs caller_try codegen diff.

#include <benchmark/benchmark.h>
#include <stdexcept>

[[gnu::noinline]] int helper_ec(int fail) {
  if (fail) return 1;
  return 0;
}

[[gnu::noinline]] void helper_throw(int fail) {
  if (fail) throw std::runtime_error("err");
}

int caller_ec(int fail) {
  int r = helper_ec(fail);
  if (r) return -1;
  return 42;
}

int caller_try(int fail) {
  try {
    helper_throw(fail);
  } catch (...) {
    return -1;
  }
  return 42;
}

static void BM_ec_success(benchmark::State& state) {
  int fail = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(fail);
    int r = helper_ec(fail);
    if (r) benchmark::DoNotOptimize(r);
    benchmark::DoNotOptimize(r);
  }
}
BENCHMARK(BM_ec_success);

static void BM_ec_fail(benchmark::State& state) {
  int fail = 1;
  for (auto _ : state) {
    benchmark::DoNotOptimize(fail);
    int r = helper_ec(fail);
    if (r) benchmark::DoNotOptimize(r);
    benchmark::DoNotOptimize(r);
  }
}
BENCHMARK(BM_ec_fail);

static void BM_throw_success(benchmark::State& state) {
  int fail = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(fail);
    try {
      helper_throw(fail);
    } catch (...) {
    }
  }
}
BENCHMARK(BM_throw_success);

static void BM_throw_fail(benchmark::State& state) {
  int fail = 1;
  for (auto _ : state) {
    benchmark::DoNotOptimize(fail);
    try {
      helper_throw(fail);
    } catch (...) {
    }
  }
}
BENCHMARK(BM_throw_fail);

BENCHMARK_MAIN();
