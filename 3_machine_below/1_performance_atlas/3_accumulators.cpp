// Number of accumulators
// Measurement of speedup with different number of accumulators
// Hypothesis: More accumulators, faster since data independent
// Results:
// ------------------------------------------------------------------
// Benchmark                        Time             CPU   Iterations
// ------------------------------------------------------------------
// BM_prediction/1/1048576     203516 ns       203454 ns         3402
// BM_prediction/2/1048576     103186 ns       103181 ns         6695
// BM_prediction/4/1048576      59763 ns        59758 ns        11938
// Explain: data are independent, multiple accumulators in theory
// can halve the time up to max number of hardware units to compute
// the operation (sum in this case)
//
// Setup: Sum with 1, 2, 4 accumulators

#include <benchmark/benchmark.h>
#include <random>
#include <vector>

inline int one_accumulator(int *numbers, size_t size) {
  int acc = 0;
  for (size_t i = 0; i != size; i++) {
    acc += numbers[i];
  }
  return acc;
}
inline int two_accumulator(int *numbers, size_t size) {
  int acc1 = 0;
  int acc2 = 0;
  for (size_t i = 0; i != size; i += 2) {
    acc1 += numbers[i];
    acc2 += numbers[i + 1];
  }
  return acc1 + acc2;
}
inline int four_accumulator(int *numbers, size_t size) {
  int acc1 = 0;
  int acc2 = 0;
  int acc3 = 0;
  int acc4 = 0;
  for (size_t i = 0; i != size; i += 4) {
    acc1 += numbers[i];
    acc2 += numbers[i + 1];
    acc3 += numbers[i + 2];
    acc4 += numbers[i + 3];
  }
  return acc1 + acc2 + acc3 + acc4;
}

static void BM_prediction(benchmark::State &state) {
  int n = state.range(1);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-100, 100);
  std::vector<int> unsorted(state.range(1));
  for (int i = 0; i < n; i++) {
    unsorted[i] = dist(rng);
  }
  int *numbers = unsorted.data();

  int res = 0;
  switch (state.range(0)) {
  case 1:
    for (auto _ : state) {
      res = one_accumulator(numbers, (size_t)n);
      benchmark::DoNotOptimize(res);
    }
    break;
  case 2:
    for (auto _ : state) {
      res = two_accumulator(numbers, (size_t)n);
      benchmark::DoNotOptimize(res);
    }
    break;
  case 4:
    for (auto _ : state) {
      res = four_accumulator(numbers, (size_t)n);
      benchmark::DoNotOptimize(res);
    }
    break;
  }
}

BENCHMARK(BM_prediction)
    ->Args({/*sorted=*/1, 1 << 20})
    ->Args({/*sorted=*/2, 1 << 20})
    ->Args({/*sorted=*/4, 1 << 20});

BENCHMARK_MAIN();
