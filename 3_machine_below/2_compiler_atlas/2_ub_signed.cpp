// Results:
// -------------------------------------------------------------------
// Benchmark            default -O2      -O2 -fwrapv     slowdown
// -------------------------------------------------------------------
// BM_always_true        0.194 ns         0.293 ns        1.5x
// BM_divide_by_two      0.243 ns         0.291 ns        1.2x
// BM_sum_scaled          791 ns          1427 ns         1.8x
// -------------------------------------------------------------------
// -fwrapv is used to make it such that int behaves as int for UB

#include <benchmark/benchmark.h>
#include <numeric>
#include <vector>

__attribute__((noinline)) bool always_true(int a) {
  return a + 1 > a;
}

__attribute__((noinline)) int divide_by_two(int x) {
  return (x * 2) / 2;
}

__attribute__((noinline)) long sum_scaled(const int* a, int n) {
  long s = 0;
  for (int i = 0; i < n; ++i) s += a[i * 2];
  return s;
}

static void BM_always_true(benchmark::State& state) {
  int a = 42;
  for (auto _ : state) {
    benchmark::DoNotOptimize(a);
    bool r = always_true(a);
    benchmark::DoNotOptimize(r);
  }
}
BENCHMARK(BM_always_true);

static void BM_divide_by_two(benchmark::State& state) {
  int x = 42;
  for (auto _ : state) {
    benchmark::DoNotOptimize(x);
    int r = divide_by_two(x);
    benchmark::DoNotOptimize(r);
  }
}
BENCHMARK(BM_divide_by_two);

static void BM_sum_scaled(benchmark::State& state) {
  const int N = 4096;
  std::vector<int> in(N * 2);
  std::iota(in.begin(), in.end(), 1);
  benchmark::DoNotOptimize(in.data());
  for (auto _ : state) {
    long s = sum_scaled(in.data(), N);
    benchmark::DoNotOptimize(s);
  }
}
BENCHMARK(BM_sum_scaled);

BENCHMARK_MAIN();
