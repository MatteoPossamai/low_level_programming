// Compile:
// ------------------------------------------------------------
// Flag                Purpose
// ------------------------------------------------------------
// -O2                 baseline
// (-fno-exceptions    would disable EH entirely - not used here)
// ------------------------------------------------------------
//
// Results (1024 push_backs, ~10 reallocations):
// ------------------------------------------------------------------
// Benchmark                    Time       Notes
// ------------------------------------------------------------------
// BM_push_fragile             32203 ns    COPIED on realloc  (no noexcept)
// BM_push_movable             15841 ns    moved on realloc   (~2x)
// BM_push_movable_reserve     14440 ns    no reallocs (baseline)
// ------------------------------------------------------------------
// See 4_noexcept.s for caller_a vs caller_b codegen diff.

#include <benchmark/benchmark.h>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct Fragile {
  std::string data;
  Fragile() : data(64, 'x') {}
  Fragile(const Fragile& o) : data(o.data) {}
  Fragile(Fragile&& o) noexcept(false) : data(std::move(o.data)) {}
  Fragile& operator=(const Fragile&) = default;
  Fragile& operator=(Fragile&&) noexcept(false) = default;
};

struct Movable {
  std::string data;
  Movable() : data(64, 'x') {}
  Movable(const Movable& o) : data(o.data) {}
  Movable(Movable&& o) noexcept : data(std::move(o.data)) {}
  Movable& operator=(const Movable&) = default;
  Movable& operator=(Movable&&) noexcept = default;
};

static_assert(!std::is_nothrow_move_constructible_v<Fragile>);
static_assert(std::is_nothrow_move_constructible_v<Movable>);

static constexpr int N = 1024;

static void BM_push_fragile(benchmark::State& state) {
  for (auto _ : state) {
    std::vector<Fragile> v;
    for (int i = 0; i < N; ++i) v.emplace_back();
    benchmark::DoNotOptimize(v.data());
  }
}
BENCHMARK(BM_push_fragile);

static void BM_push_movable(benchmark::State& state) {
  for (auto _ : state) {
    std::vector<Movable> v;
    for (int i = 0; i < N; ++i) v.emplace_back();
    benchmark::DoNotOptimize(v.data());
  }
}
BENCHMARK(BM_push_movable);

static void BM_push_movable_reserve(benchmark::State& state) {
  for (auto _ : state) {
    std::vector<Movable> v;
    v.reserve(N);
    for (int i = 0; i < N; ++i) v.emplace_back();
    benchmark::DoNotOptimize(v.data());
  }
}
BENCHMARK(BM_push_movable_reserve);

// Codegen demo: same body, one noexcept one not.
[[gnu::noinline]] void may_throw(int x) {
  if (x == 0) throw std::runtime_error("zero");
}

void caller_a(int x) { may_throw(x); }
void caller_b(int x) noexcept { may_throw(x); }

BENCHMARK_MAIN();
