// Compile:
// ---------------------------------------------------
// Flag           Purpose
// ---------------------------------------------------
// -O2            baseline
// -march=native  allow SIMD if body inlines
// ---------------------------------------------------
//
// Results (sum area of 4096 circles):
// -------------------------------------------------------------
// Benchmark        Time      Dispatch      Notes
// -------------------------------------------------------------
// BM_virtual      6502 ns    indirect      vtable call, no inline
// BM_final        1618 ns    direct        devirt fired, inlined
// BM_crtp         1585 ns    static        inlined + SIMD (ymm)
// -------------------------------------------------------------
// See 5_virtual_vs_crtp.s for annotated inner loops.

#include <benchmark/benchmark.h>
#include <memory>
#include <vector>

struct IShape {
  virtual ~IShape() = default;
  virtual double area() const = 0;
};

struct Circle : IShape {
  double r;
  explicit Circle(double r_) : r(r_) {}
  double area() const override { return 3.14159 * r * r; }
};

struct CircleFinal final : IShape {
  double r;
  explicit CircleFinal(double r_) : r(r_) {}
  double area() const override { return 3.14159 * r * r; }
};

template <typename D>
struct ShapeBase {
  double area() const { return static_cast<const D*>(this)->area_impl(); }
};

struct CircleC : ShapeBase<CircleC> {
  double r;
  explicit CircleC(double r_) : r(r_) {}
  double area_impl() const { return 3.14159 * r * r; }
};

static constexpr int N = 4096;

__attribute__((noinline)) double sum_virtual(const std::vector<IShape*>& v) {
  double s = 0;
  for (auto* p : v) s += p->area();
  return s;
}

__attribute__((noinline)) double sum_final(const std::vector<CircleFinal*>& v) {
  double s = 0;
  for (auto* p : v) s += p->area();
  return s;
}

__attribute__((noinline)) double sum_crtp(const std::vector<CircleC>& v) {
  double s = 0;
  for (const auto& c : v) s += c.area();
  return s;
}

static void BM_virtual(benchmark::State& state) {
  std::vector<std::unique_ptr<Circle>> owner;
  std::vector<IShape*> v;
  for (int i = 0; i < N; ++i) {
    owner.emplace_back(std::make_unique<Circle>(i + 1.0));
    v.push_back(owner.back().get());
  }
  benchmark::DoNotOptimize(v.data());
  for (auto _ : state) {
    double s = sum_virtual(v);
    benchmark::DoNotOptimize(s);
  }
}
BENCHMARK(BM_virtual);

static void BM_final(benchmark::State& state) {
  std::vector<std::unique_ptr<CircleFinal>> owner;
  std::vector<CircleFinal*> v;
  for (int i = 0; i < N; ++i) {
    owner.emplace_back(std::make_unique<CircleFinal>(i + 1.0));
    v.push_back(owner.back().get());
  }
  benchmark::DoNotOptimize(v.data());
  for (auto _ : state) {
    double s = sum_final(v);
    benchmark::DoNotOptimize(s);
  }
}
BENCHMARK(BM_final);

static void BM_crtp(benchmark::State& state) {
  std::vector<CircleC> v;
  for (int i = 0; i < N; ++i) v.emplace_back(i + 1.0);
  benchmark::DoNotOptimize(v.data());
  for (auto _ : state) {
    double s = sum_crtp(v);
    benchmark::DoNotOptimize(s);
  }
}
BENCHMARK(BM_crtp);

BENCHMARK_MAIN();
