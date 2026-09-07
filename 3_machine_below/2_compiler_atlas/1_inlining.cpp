// Results:
// -------------------------------------------------------
// Benchmark             Time             CPU   Iterations
// -------------------------------------------------------
// BM_inline           124 ns          124 ns      5626228
// BM_no_inline        407 ns          407 ns      1722227

#include <benchmark/benchmark.h>
#include <numeric>
#include <vector>

__attribute__((always_inline)) inline int inline_square(int number) {
  return number * number;
}

__attribute__((noinline)) int no_inline_square(int number) {
  return number * number;
}

static constexpr int N = 1024;

static void BM_inline(benchmark::State &state) {
  std::vector<int> in(N);
  std::iota(in.begin(), in.end(), 1);
  benchmark::DoNotOptimize(in.data());

  for (auto _ : state) {
    int sum = 0;
    for (int i = 0; i < N; ++i) {
      sum += inline_square(in[i]);
    }
    benchmark::DoNotOptimize(sum);
  }
}
BENCHMARK(BM_inline);

static void BM_no_inline(benchmark::State &state) {
  std::vector<int> in(N);
  std::iota(in.begin(), in.end(), 1);
  benchmark::DoNotOptimize(in.data());

  for (auto _ : state) {
    int sum = 0;
    for (int i = 0; i < N; ++i) {
      sum += no_inline_square(in[i]);
    }
    benchmark::DoNotOptimize(sum);
  }
}
BENCHMARK(BM_no_inline);

BENCHMARK_MAIN();
