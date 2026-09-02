// False sharing
// Performance when accumulators in same line or padded
// Hypothesis: Way slower when not padded, due to false sharing
// Results:
// ----------------------------------------------------------------------------
// Benchmark                                  Time             CPU   Iterations
// ----------------------------------------------------------------------------
// BM_prediction/0/1048576/real_time    4644456 ns        36661 ns          150
// BM_prediction/1/1048576/real_time   19984188 ns        60846 ns           33
// Explain: with no padding same cache line, requires to make sure all is good,
// so this incurs in a penality due to false sharing
//
// Setting: Two threads and two counters

#include <atomic>
#include <benchmark/benchmark.h>
#include <thread>

struct Summer_1 {
  std::atomic<int> sum1;
  std::atomic<int> sum2;
};

struct Summer_2 {
  std::atomic<int> sum1;
  char padding[1024];
  std::atomic<int> sum2;
};
auto summer1 = Summer_1{};
auto summer2 = Summer_2{};

void thread_summer_1(int idx, int n) {
  if (idx == 1) {
    for (int i = 0; i < n; i++) {
      summer1.sum1++;
    }
  } else {
    for (int i = 0; i < n; i++) {
      summer1.sum2++;
    }
  }
}

void thread_summer_2(int idx, int n) {
  if (idx == 1) {
    for (int i = 0; i < n; i++) {
      summer2.sum1++;
    }
  } else {
    for (int i = 0; i < n; i++) {
      summer2.sum2++;
    }
  }
}

static void BM_prediction(benchmark::State &state) {
  int n = state.range(1);
  int summer_idx = state.range(0);

  if (summer_idx == 1) {
    for (auto _ : state) {
      std::thread th1(thread_summer_1, 1, n);
      std::thread th2(thread_summer_1, 2, n);
      th1.join();
      th2.join();
    }
  } else {
    for (auto _ : state) {
      std::thread th1(thread_summer_2, 1, n);
      std::thread th2(thread_summer_2, 2, n);
      th1.join();
      th2.join();
    }
  }
}

BENCHMARK(BM_prediction)->Args({0, 1 << 20})->Args({1, 1 << 20})->UseRealTime();
;

BENCHMARK_MAIN();
