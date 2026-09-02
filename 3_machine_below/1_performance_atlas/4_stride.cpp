// Stride
// Different strides effect on runtime
// Hypothesis: Higher stride, more cache misses, longer time
// Result
// -------------------------------------------------------------------
// Benchmark                         Time             CPU   Iterations
// -------------------------------------------------------------------
// BM_prediction/1/1048576      271452 ns       271441 ns         2506
// BM_prediction/2/1048576      276748 ns       276709 ns         2579
// BM_prediction/4/1048576      301392 ns       301374 ns         2299
// BM_prediction/8/1048576      389114 ns       389068 ns         1683
// BM_prediction/16/1048576     651557 ns       651490 ns         1048
// BM_prediction/32/1048576    1280172 ns      1279630 ns          539
// BM_prediction/64/1048576    1736999 ns      1736618 ns          408
// Explanation: the more stride, the more branch BM_prediction. At the beginning
// minor since there are more int in the cache line, but later this causes
// misses on every time, and this makes it slower
//
// Setting: higher up stride every time.

#include <benchmark/benchmark.h>
#include <random>

int sum_stride(int *numbers, int size, int stride) {
  int res = 0;
  int times = size / stride;
  for (int i = 0; i != stride; i++) {
    for (int j = 0; j != times; j++) {
      res += numbers[i + j * stride];
    }
  }

  return res;
}

static void BM_prediction(benchmark::State &state) {
  int n = state.range(1);
  int stride = state.range(0);

  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-100, 100);
  std::vector<int> unsorted(state.range(1));
  for (int i = 0; i < n; i++) {
    unsorted[i] = dist(rng);
  }
  int *numbers = unsorted.data();

  int res;
  for (auto _ : state) {
    res = sum_stride(numbers, n, stride);
    benchmark::DoNotOptimize(res);
  }
}

BENCHMARK(BM_prediction)
    ->Args({1, 1 << 20})
    ->Args({2, 1 << 20})
    ->Args({4, 1 << 20})
    ->Args({8, 1 << 20})
    ->Args({16, 1 << 20})
    ->Args({32, 1 << 20})
    ->Args({64, 1 << 20});

BENCHMARK_MAIN();
