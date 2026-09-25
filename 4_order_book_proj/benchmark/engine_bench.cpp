// Engine baseline benchmark: pre-built OUCH messages fed through
// Engine::process() on the benchmark thread, outbound drained by a consumer
// thread (the fan-out side), as in production.
//
// Traffic, from the op mix in DATA_STRUCTURE.md with the top-of-book query
// dropped (the engine has no such message), renormalised:
//   enter limit 50%, cancel 48%, market 2%.
// Limit prices are uniform within +-100 ticks of a fixed mid, buys biased
// below it and sells above, so a few percent of limits are marketable.
// Book is warmed up to `depth` resting orders before timing starts.
//
// Measured: engine core only (decode, match, book update, outbound encode and
// enqueue). Not measured: inbound MPSC hop, sockets. Those are Block 4/6.

#include "engine.hpp"

#include <benchmark/benchmark.h>

#include <cstring>
#include <memory>
#include <random>
#include <thread>
#include <vector>

namespace {

constexpr size_t QUEUE = 1 << 14;
constexpr size_t PRICES = 1024;
constexpr size_t POOL = 1 << 17;
constexpr uint64_t MID = 512;
constexpr uint64_t MKT = 0x7FFFFFFF;
constexpr uint32_t ACCOUNTS = 16;
constexpr uint32_t END = 0xFFFFFFFF;

using BenchEngine = Engine<QUEUE, PRICES, POOL>;

InboundMessage enter(uint32_t acct, uint32_t urn, SideEnum side, uint32_t qty,
                     uint64_t price) {
  auto b = EnterRequestBuilder()
               .UserRefNum(urn)
               .Side(side)
               .Quantity(qty)
               .Symbol("AAPL")
               .Price(price);
  InboundMessage m{acct, {}};
  std::memcpy(m.bytes.data(), b.bytes().data(), b.bytes().size());
  return m;
}

InboundMessage cancel(uint32_t acct, uint32_t urn) {
  auto b = CancelRequestBuilder().UserRefNum(urn).Quantity(0);
  InboundMessage m{acct, {}};
  std::memcpy(m.bytes.data(), b.bytes().data(), b.bytes().size());
  return m;
}

struct Traffic {
  std::vector<InboundMessage> warmup;
  std::vector<InboundMessage> ops;
};

// Deterministic: same seed => same messages.
Traffic generate(uint64_t depth, uint64_t n_ops, uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_int_distribution<uint32_t> acct_d(0, ACCOUNTS - 1);
  std::uniform_int_distribution<uint32_t> qty_d(1, 100);
  std::uniform_int_distribution<int> mix_d(0, 99);
  std::uniform_int_distribution<uint64_t> away_d(0, 100);
  std::uniform_int_distribution<uint64_t> cross_d(0, 4);

  std::vector<uint32_t> next_urn(ACCOUNTS, 1);
  std::vector<std::pair<uint32_t, uint32_t>> live;

  auto limit = [&](bool warm) {
    uint32_t acct = acct_d(rng);
    uint32_t urn = next_urn[acct]++;
    bool buy = rng() & 1;
    uint64_t away = away_d(rng);
    uint64_t price;
    if (warm)
      price = buy ? MID - 1 - away : MID + 1 + away;
    else
      price = buy ? MID - away + cross_d(rng) : MID + away - cross_d(rng);
    live.emplace_back(acct, urn);
    return enter(acct, urn, buy ? SideEnum::B : SideEnum::S, qty_d(rng),
                 price);
  };

  Traffic t;
  t.warmup.reserve(depth);
  for (uint64_t i = 0; i < depth; i++)
    t.warmup.push_back(limit(true));

  t.ops.reserve(n_ops);
  for (uint64_t i = 0; i < n_ops; i++) {
    int r = mix_d(rng);
    if (r < 50 || live.empty()) {
      t.ops.push_back(limit(false));
    } else if (r < 98) {
      size_t k = rng() % live.size();
      t.ops.push_back(cancel(live[k].first, live[k].second));
      live[k] = live.back();
      live.pop_back();
    } else {
      uint32_t acct = acct_d(rng);
      t.ops.push_back(enter(acct, next_urn[acct]++,
                            rng() & 1 ? SideEnum::B : SideEnum::S, qty_d(rng),
                            MKT));
    }
  }
  return t;
}

void BM_Engine(benchmark::State &state) {
  const uint64_t depth = state.range(0);
  const uint64_t n_ops = state.range(1);
  const Traffic traffic = generate(depth, n_ops, /*seed=*/42);

  auto in = std::make_unique<mpsc_queue<InboundMessage, QUEUE>>();
  auto out = std::make_unique<spsc_queue<OutboundMessage, QUEUE>>();

  uint64_t out_msgs = 0;
  std::thread consumer([&] {
    for (;;) {
      OutboundMessage m;
      out->dequeue(m);
      if (m.account == END)
        return;
      out_msgs++;
    }
  });

  for (auto _ : state) {
    state.PauseTiming();
    auto engine = std::make_unique<BenchEngine>(*in, *out);
    for (const auto &m : traffic.warmup)
      engine->process(m);
    state.ResumeTiming();

    for (const auto &m : traffic.ops)
      engine->process(m);

    state.PauseTiming();
    engine.reset();
    state.ResumeTiming();
  }

  out->enqueue(OutboundMessage{END, {}});
  consumer.join();

  state.SetItemsProcessed(state.iterations() * n_ops);
  state.counters["out_msgs_incl_warmup"] = static_cast<double>(out_msgs);
}

} // namespace

BENCHMARK(BM_Engine)
    ->Args({1'000, 100'000})
    ->Args({10'000, 100'000})
    ->Args({50'000, 100'000})
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime();

BENCHMARK_MAIN();
