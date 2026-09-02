// Benchmark Branch Missprediction
// Measurement of the penality of branch missprediction
// Hypothesis: Is going to be 20 times faster since accessing Cache is 1-4
// cycles VS the 50 cycles for Memory
// Results:
// ------------------------------------------------------------------
// Benchmark                        Time             CPU   Iterations
// ------------------------------------------------------------------
// BM_prediction/0/1048576    2887514 ns      2887083 ns          245
// BM_prediction/1/1048576    6797019 ns      6796604 ns          102
//
// Setup: Using a sorted and unsorted algorithm. With sorted, the number of
// branch prediction is very high since the branch predictor learns fast
// the patterns and is able to get it quite correct after not long.
// If is not sorted, then you are going to get random, so on average 50%
// misses. This makes the prediction made above over optimistic. Not direct
// way to measure those. Too little, and the chronometer could just take it
// over. Need to sink result to avoid compiler optimization.
//
//

#include <algorithm>
#include <benchmark/benchmark.h>
#include <random>
#include <vector>

static void BM_prediction(benchmark::State &state) {
  // untimed: build your data here
  const int n = state.range(1);
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(-100, 100);
  std::vector<int> unsorted(state.range(1));

  for (int i = 0; i < n; i++) {
    unsorted[i] = dist(rng);
  }
  std::vector<int> sorted = unsorted;
  std::sort(sorted.begin(), sorted.end());
  int median = sorted[state.range(1) / 2];

  std::vector<int> array = state.range(0) == 0 ? sorted : unsorted;

  int res = 0;
  for (auto _ : state) {
    // timed: the scan-and-branch goes here
    for (int i = 0; i != n; i++) {
      if (median > array[i]) {
        res += array[i];
      }
    }
    benchmark::DoNotOptimize(res);
  }
}

BENCHMARK(BM_prediction)
    ->Args({/*sorted=*/0, 1 << 20})
    ->Args({/*sorted=*/1, 1 << 20});

BENCHMARK_MAIN();
