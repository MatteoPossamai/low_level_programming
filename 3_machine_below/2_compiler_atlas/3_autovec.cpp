// Compile:
// --------------------------------------------------------------------
// Flag                        Purpose
// --------------------------------------------------------------------
// -O2                         baseline optimisation (vectoriser on)
// -march=native               enable this CPU's SIMD (AVX2, AVX-512...)
// -fopt-info-vec              log loops that WERE vectorised
// -fopt-info-vec-missed       log loops the vectoriser gave up on
// -fopt-info-vec-all=vec.log  both, routed to build/vec.log
// --------------------------------------------------------------------
// See CMakeLists for the exact recipe. Missed-vec reasons show up in
// build/vec.log after `cmake --build build --target 3_autovec`.
//
// Results (4096-elem, AVX2 ymm):
// --------------------------------------------------
// Benchmark             Time      Vectorised?   Why
// --------------------------------------------------
// BM_sum_vec            103 ns    yes (ymm)     clean reduction
// BM_add_alias          726 ns    NO            src/dst may alias
// BM_add_restrict       155 ns    yes (ymm)     __restrict__ promise
// BM_sum_call          1619 ns    NO            call in loop
// --------------------------------------------------
// See build/vec.log for compiler diagnostics per loop.

#include <benchmark/benchmark.h>
#include <numeric>
#include <vector>

__attribute__((noinline)) int sum_vec(const int* a, int n) {
  int s = 0;
  for (int i = 0; i < n; ++i) s += a[i];
  return s;
}

__attribute__((noinline)) void add_alias(int* dst, const int* src, int n) {
  for (int i = 0; i < n; ++i) dst[i] = src[i] + 1;
}

__attribute__((noinline)) void add_restrict(int* __restrict__ dst,
                                            const int* __restrict__ src,
                                            int n) {
  for (int i = 0; i < n; ++i) dst[i] = src[i] + 1;
}

__attribute__((noinline)) int helper(int x) { return x * x + 1; }

__attribute__((noinline)) int sum_call(const int* a, int n) {
  int s = 0;
  for (int i = 0; i < n; ++i) s += helper(a[i]);
  return s;
}

static constexpr int N = 4096;

static void BM_sum_vec(benchmark::State& state) {
  std::vector<int> in(N);
  std::iota(in.begin(), in.end(), 1);
  benchmark::DoNotOptimize(in.data());
  for (auto _ : state) {
    int s = sum_vec(in.data(), N);
    benchmark::DoNotOptimize(s);
  }
}
BENCHMARK(BM_sum_vec);

static void BM_add_alias(benchmark::State& state) {
  std::vector<int> src(N), dst(N);
  std::iota(src.begin(), src.end(), 1);
  benchmark::DoNotOptimize(src.data());
  benchmark::DoNotOptimize(dst.data());
  for (auto _ : state) {
    add_alias(dst.data(), src.data(), N);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_add_alias);

static void BM_add_restrict(benchmark::State& state) {
  std::vector<int> src(N), dst(N);
  std::iota(src.begin(), src.end(), 1);
  benchmark::DoNotOptimize(src.data());
  benchmark::DoNotOptimize(dst.data());
  for (auto _ : state) {
    add_restrict(dst.data(), src.data(), N);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_add_restrict);

static void BM_sum_call(benchmark::State& state) {
  std::vector<int> in(N);
  std::iota(in.begin(), in.end(), 1);
  benchmark::DoNotOptimize(in.data());
  for (auto _ : state) {
    int s = sum_call(in.data(), N);
    benchmark::DoNotOptimize(s);
  }
}
BENCHMARK(BM_sum_call);

BENCHMARK_MAIN();
