// CMOV vs Branch missprediction
// Measurement of when conditional move is better than prediction
// Hypothesis: Predict better when there is patterns, otherwise worse
// Results:
// --------------------------------------------------------------------
// Benchmark                          Time             CPU   Iterations
// --------------------------------------------------------------------
// BM_prediction/0/1048576/0     206787 ns       206736 ns         3418
// BM_prediction/0/1048576/1     205742 ns       205720 ns         3404
// BM_prediction/1/1048576/0     207733 ns       207657 ns         3362
// BM_prediction/1/1048576/1    2820243 ns      2819647 ns          252
// Explain: the conditional move (ones with 0 as second argument) have costant
// overhead regarless of sorted or not, so in that case the fact that is
// predictable or not is irrelevant. If is sorted and is branchy, then is as
// fast as, since the correct guesses make up for the wrong ones. If is branchy
// and non predictable, then is VERY SLOW. This goes back to the first quarter,
// where they suggested to always have conditional move rather than branches
// were possible, and here is the reason.
//
// I expected some difference in the branchless when sorted or not, but this
// is not the case.
//
// Setup: Using a sorted and unsorted algorithm.

#include <algorithm>
#include <benchmark/benchmark.h>
#include <random>
#include <vector>

inline int sum_branchless(int value, int median) {
  return (value < median) * value;
}

inline int sum_branchy(int value, int median) {
  if (value < median) {
    return value;
  }
  return 0;
}

static void BM_prediction(benchmark::State &state) {
  // START: Create arrays and select
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
  // END: Create arrays and select

  int res = 0;
  if (state.range(2) == 0) {
    for (auto _ : state) {
      for (int i = 0; i != n; i++) {
        res += sum_branchless(array[i], median);
      }
      benchmark::DoNotOptimize(res);
    }
  } else {
    for (auto _ : state) {
      for (int i = 0; i != n; i++) {
        res += sum_branchy(array[i], median);
      }
      benchmark::DoNotOptimize(res);
    }
  }
}

BENCHMARK(BM_prediction)
    ->Args({/*sorted=*/0, 1 << 20, 0})
    ->Args({/*sorted=*/0, 1 << 20, 1})
    ->Args({/*sorted=*/1, 1 << 20, 0})
    ->Args({/*sorted=*/1, 1 << 20, 1});

BENCHMARK_MAIN();
