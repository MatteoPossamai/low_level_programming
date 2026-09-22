// OUCH codec micro-benchmark.
//
// Measures encode (Builder → wire), decode (bytes → View + type dispatch),
// and per-field-read cost on a decoded view. Partial-read vs full-read shows
// the zero-copy payoff: full-read pays for every field, partial-read only
// touches what it needs.
//
// Builders live as named locals so their span/array stays valid across the
// full expression. Never dot-chain a builder into a span-consuming call in
// one statement — the temporary dies mid-expression.

#include "messages.hpp"

#include <benchmark/benchmark.h>
#include <concepts>
#include <cstdio>
#include <cstdlib>
#include <type_traits>
#include <variant>

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::fprintf(stderr, "roundtrip FAIL: %s at %s:%d\n", #cond, __FILE__,   \
                   __LINE__);                                                  \
      std::abort();                                                            \
    }                                                                          \
  } while (0)

static EnterRequestBuilder make_populated() {
  EnterRequestBuilder b;
  b.UserRefNum(0xDEADBEEFCAFEBABEULL)
      .Side(SideEnum::B)
      .Quantity(100)
      .Symbol("AAPL")
      .Price(15000)
      .TimeInForce(TimeInForceEnum::IOC)
      .Display(DisplayEnum::Y)
      .Capacity(CapacityEnum::A)
      .InterMarketSweepElig(InterMarketSweepEligEnum::N)
      .CrossType(CrossTypeEnum::N)
      .ClOrdID("order-42");
  return b;
}

static void BM_EncodeEnterRequest(benchmark::State &state) {
  for (auto _ : state) {
    EnterRequestBuilder b = make_populated();
    auto out = b.bytes();
    benchmark::DoNotOptimize(out.data());
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_EncodeEnterRequest);

static void BM_DecodeDispatch(benchmark::State &state) {
  EnterRequestBuilder b = make_populated();
  auto wire = b.bytes();

  for (auto _ : state) {
    auto msg = decode(wire.data());
    benchmark::DoNotOptimize(msg);
  }
}
BENCHMARK(BM_DecodeDispatch);

static void BM_DecodePartialRead(benchmark::State &state) {
  EnterRequestBuilder b = make_populated();
  auto wire = b.bytes();

  for (auto _ : state) {
    auto msg = decode(wire.data());
    std::visit(
        [](auto &v) {
          benchmark::DoNotOptimize(v.UserRefNum());
        },
        msg);
  }
}
BENCHMARK(BM_DecodePartialRead);

static void BM_DecodeFullRead(benchmark::State &state) {
  EnterRequestBuilder b = make_populated();
  auto wire = b.bytes();

  for (auto _ : state) {
    auto msg = decode(wire.data());
    std::visit(
        [](auto &v) {
          using T = std::remove_reference_t<decltype(v)>;
          if constexpr (std::same_as<T, EnterRequestView>) {
            benchmark::DoNotOptimize(v.UserRefNum());
            benchmark::DoNotOptimize(v.Side());
            benchmark::DoNotOptimize(v.Quantity());
            benchmark::DoNotOptimize(v.Symbol());
            benchmark::DoNotOptimize(v.Price());
            benchmark::DoNotOptimize(v.TimeInForce());
            benchmark::DoNotOptimize(v.Display());
            benchmark::DoNotOptimize(v.Capacity());
            benchmark::DoNotOptimize(v.InterMarketSweepElig());
            benchmark::DoNotOptimize(v.CrossType());
            benchmark::DoNotOptimize(v.ClOrdID());
          }
        },
        msg);
  }
}
BENCHMARK(BM_DecodeFullRead);

// Roundtrip: build a message with every field set, decode, assert every
// accessor returns what we wrote. Runs before the benchmarks; any failure
// aborts before we measure anything.
static void run_roundtrip_check() {
  EnterRequestBuilder b = make_populated();
  auto wire = b.bytes();
  CHECK(wire.size() == EnterRequestView::WIRE_SIZE);

  auto msg = decode(wire.data());
  auto &v = std::get<EnterRequestView>(msg);
  CHECK(v.UserRefNum() == 0xDEADBEEFCAFEBABEULL);
  CHECK(v.Side() == SideEnum::B);
  CHECK(v.Quantity() == 100);
  CHECK(v.Symbol() == "AAPL    ");
  CHECK(v.Price() == 15000);
  CHECK(v.TimeInForce() == TimeInForceEnum::IOC);
  CHECK(v.Display() == DisplayEnum::Y);
  CHECK(v.Capacity() == CapacityEnum::A);
  CHECK(v.InterMarketSweepElig() == InterMarketSweepEligEnum::N);
  CHECK(v.CrossType() == CrossTypeEnum::N);
  CHECK(v.ClOrdID() == "order-42      ");

  CancelRequestBuilder cb;
  cb.UserRefNum(7).Quantity(50);
  auto cwire = cb.bytes();
  auto cmsg = decode(cwire.data());
  auto &cv = std::get<CancelRequestView>(cmsg);
  CHECK(cv.UserRefNum() == 7);
  CHECK(cv.Quantity() == 50);

  std::puts("roundtrip: OK");
}

int main(int argc, char **argv) {
  run_roundtrip_check();
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
  return 0;
}
