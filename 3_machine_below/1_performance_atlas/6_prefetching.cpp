// Prefetching
// Measurement of __builtin_prefetch: where it helps and where it doesn't
// Hypothesis: No effect on sequential scan (hardware prefetcher already
// streams ahead). Big win on random access via an index array, because the
// hardware cannot guess the next address but the software can read idx[i+D]
// ahead of time and start the memory fetch early.
// Result:
// ----------------------------------------------------------------------
// Benchmark                            Time             CPU   Iterations
// ----------------------------------------------------------------------
// BM_prefetch/0/16777216/0/0     6445688 ns      6444742 ns           86
// BM_prefetch/0/16777216/1/16    7341953 ns      7341100 ns           98
// BM_prefetch/1/16777216/0/0    85143500 ns     85124488 ns            8
// BM_prefetch/1/16777216/1/4    87526480 ns     87506659 ns            8
// BM_prefetch/1/16777216/1/16   85201025 ns     85190963 ns            8
// BM_prefetch/1/16777216/1/64   83632653 ns     83617603 ns            8

#include <algorithm>
#include <benchmark/benchmark.h>
#include <numeric>
#include <random>
#include <vector>

static void BM_prefetch(benchmark::State &state) {
  const int n = static_cast<int>(state.range(1));
  const bool random_pattern = state.range(0) == 1;
  const bool use_prefetch = state.range(2) == 1;
  const int dist = static_cast<int>(state.range(3));

  std::vector<int> data(n, 1);
  std::vector<int> idx(n);
  std::iota(idx.begin(), idx.end(), 0);
  if (random_pattern) {
    std::mt19937 rng(42);
    std::shuffle(idx.begin(), idx.end(), rng);
  }

  long res = 0;
  for (auto _ : state) {
    if (use_prefetch) {
      for (int i = 0; i < n - dist; i++) {
        __builtin_prefetch(&data[idx[i + dist]]);
        res += data[idx[i]];
      }
      for (int i = n - dist; i < n; i++) { // tail, no lookahead left
        res += data[idx[i]];
      }
    } else {
      for (int i = 0; i < n; i++) {
        res += data[idx[i]];
      }
    }
    benchmark::DoNotOptimize(res);
  }
}

BENCHMARK(BM_prefetch)
    ->Args({/*seq=*/0, 1 << 24, /*prefetch=*/0, 0})
    ->Args({/*seq=*/0, 1 << 24, /*prefetch=*/1, 16})
    ->Args({/*rand=*/1, 1 << 24, /*prefetch=*/0, 0})
    ->Args({/*rand=*/1, 1 << 24, /*prefetch=*/1, 4})
    ->Args({/*rand=*/1, 1 << 24, /*prefetch=*/1, 16})
    ->Args({/*rand=*/1, 1 << 24, /*prefetch=*/1, 64});

BENCHMARK_MAIN();
