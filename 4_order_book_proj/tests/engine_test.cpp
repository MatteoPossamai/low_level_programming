#include "engine.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

// Scenario tests: each scenario is a list of steps. A step is one inbound
// message plus the exact outbound messages it must produce, in order.
// Outbound messages are rendered as short strings so expectations read easily:
//   A <acct>/<urn> <side> <qty>@<price> ref=<order ref>   Accepted
//   E <acct>/<urn> <qty>@<price> <liquidity> m=<match>     Executed
//   C <acct>/<urn> <qty> <reason>                          Canceled
// After each step the test pushes an end marker onto the outbound queue and
// reads up to it, so missing and extra messages both fail instead of hanging.

namespace {

constexpr uint64_t MKT = 0x7FFFFFFF;
constexpr size_t QUEUE = 1024;
constexpr size_t PRICES = 128;
constexpr uint32_t END = 0xFFFFFFFF;

using TestEngine = Engine<QUEUE, PRICES, 1024>;
using TinyPoolEngine = Engine<QUEUE, PRICES, 1>;

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

InboundMessage cancel(uint32_t acct, uint32_t urn, uint32_t qty) {
  auto b = CancelRequestBuilder().UserRefNum(urn).Quantity(qty);
  InboundMessage m{acct, {}};
  std::memcpy(m.bytes.data(), b.bytes().data(), b.bytes().size());
  return m;
}

std::string render(const OutboundMessage &m) {
  char buf[128];
  auto decoded = decode(m.bytes.data());
  if (auto *a = std::get_if<AcceptResponseView>(&decoded)) {
    std::snprintf(buf, sizeof buf, "A %u/%u %c %u@%llu ref=%llu", m.account,
                  a->UserRefNum(), static_cast<char>(a->Side()), a->Quantity(),
                  static_cast<unsigned long long>(a->Price()),
                  static_cast<unsigned long long>(a->OrderReferenceNumber()));
  } else if (auto *e = std::get_if<ExecutedResponseView>(&decoded)) {
    std::snprintf(buf, sizeof buf, "E %u/%u %u@%llu %c m=%llu", m.account,
                  e->UserRefNum(), e->Quantity(),
                  static_cast<unsigned long long>(e->Price()),
                  e->LiquidityFlag(),
                  static_cast<unsigned long long>(e->MatchNumber()));
  } else if (auto *c = std::get_if<CancelledResponseView>(&decoded)) {
    std::snprintf(buf, sizeof buf, "C %u/%u %u %c", m.account, c->UserRefNum(),
                  c->Quantity(), c->Reason());
  } else {
    std::snprintf(buf, sizeof buf, "unexpected type %c",
                  static_cast<char>(m.bytes[0]));
  }
  return buf;
}

struct Step {
  InboundMessage in;
  std::vector<std::string> out;
};

struct Scenario {
  std::string name;
  std::vector<Step> steps;
  bool tiny_pool = false;
};

using B = SideEnum;

std::vector<Scenario> scenarios() {
  return {
      {"RestsWithoutCross",
       {
           {enter(1, 1, B::S, 10, 105), {"A 1/1 S 10@105 ref=1"}},
           {enter(2, 1, B::B, 5, 100), {"A 2/1 B 5@100 ref=2"}},
       }},
      {"ExactMatchRemovesBoth",
       {
           {enter(1, 1, B::S, 10, 105), {"A 1/1 S 10@105 ref=1"}},
           {enter(2, 1, B::B, 10, 105),
            {"A 2/1 B 10@105 ref=2", "E 2/1 10@105 R m=1",
             "E 1/1 10@105 A m=1"}},
           {cancel(1, 1, 0), {}},
           {cancel(2, 1, 0), {}},
       }},
      {"IncomingPartiallyFillsResting",
       {
           {enter(1, 1, B::S, 10, 105), {"A 1/1 S 10@105 ref=1"}},
           {enter(2, 1, B::B, 4, 110),
            {"A 2/1 B 4@110 ref=2", "E 2/1 4@105 R m=1", "E 1/1 4@105 A m=1"}},
           {cancel(1, 1, 0), {"C 1/1 6 U"}},
       }},
      {"SweepsLevelsThenRests",
       {
           {enter(1, 1, B::S, 5, 101), {"A 1/1 S 5@101 ref=1"}},
           {enter(1, 2, B::S, 5, 102), {"A 1/2 S 5@102 ref=2"}},
           {enter(1, 3, B::S, 5, 110), {"A 1/3 S 5@110 ref=3"}},
           {enter(2, 1, B::B, 12, 105),
            {"A 2/1 B 12@105 ref=4", "E 2/1 5@101 R m=1", "E 1/1 5@101 A m=1",
             "E 2/1 5@102 R m=2", "E 1/2 5@102 A m=2"}},
           {enter(3, 1, B::S, 2, 100),
            {"A 3/1 S 2@100 ref=5", "E 3/1 2@105 R m=3", "E 2/1 2@105 A m=3"}},
       }},
      {"TimePriorityAtSameLevel",
       {
           {enter(1, 1, B::S, 5, 105), {"A 1/1 S 5@105 ref=1"}},
           {enter(2, 1, B::S, 5, 105), {"A 2/1 S 5@105 ref=2"}},
           {enter(3, 1, B::B, 7, 105),
            {"A 3/1 B 7@105 ref=3", "E 3/1 5@105 R m=1", "E 1/1 5@105 A m=1",
             "E 3/1 2@105 R m=2", "E 2/1 2@105 A m=2"}},
           {cancel(2, 1, 0), {"C 2/1 3 U"}},
       }},
      {"PricePriorityBestBidFirst",
       {
           {enter(1, 1, B::B, 5, 100), {"A 1/1 B 5@100 ref=1"}},
           {enter(2, 1, B::B, 5, 102), {"A 2/1 B 5@102 ref=2"}},
           {enter(3, 1, B::B, 5, 101), {"A 3/1 B 5@101 ref=3"}},
           {enter(4, 1, B::S, 12, 99),
            {"A 4/1 S 12@99 ref=4", "E 4/1 5@102 R m=1", "E 2/1 5@102 A m=1",
             "E 4/1 5@101 R m=2", "E 3/1 5@101 A m=2", "E 4/1 2@100 R m=3",
             "E 1/1 2@100 A m=3"}},
           {cancel(1, 1, 0), {"C 1/1 3 U"}},
       }},
      {"MarketBuySweepsAndCancelsRest",
       {
           {enter(1, 1, B::S, 3, 105), {"A 1/1 S 3@105 ref=1"}},
           {enter(1, 2, B::S, 3, 106), {"A 1/2 S 3@106 ref=2"}},
           {enter(2, 1, B::B, 10, MKT),
            {"A 2/1 B 10@2147483647 ref=3", "E 2/1 3@105 R m=1",
             "E 1/1 3@105 A m=1", "E 2/1 3@106 R m=2", "E 1/2 3@106 A m=2",
             "C 2/1 4 I"}},
       }},
      {"MarketSellWithNoBidsIsCancelled",
       {
           {enter(1, 1, B::S, 5, MKT),
            {"A 1/1 S 5@2147483647 ref=1", "C 1/1 5 I"}},
       }},
      {"MarketSellSweepsBids",
       {
           {enter(1, 1, B::B, 4, 50), {"A 1/1 B 4@50 ref=1"}},
           {enter(1, 2, B::B, 4, 40), {"A 1/2 B 4@40 ref=2"}},
           {enter(2, 1, B::S, 6, MKT),
            {"A 2/1 S 6@2147483647 ref=3", "E 2/1 4@50 R m=1",
             "E 1/1 4@50 A m=1", "E 2/1 2@40 R m=2", "E 1/2 2@40 A m=2"}},
       }},
      {"CancelReducesButNeverIncreases",
       {
           {enter(1, 1, B::B, 50, 100), {"A 1/1 B 50@100 ref=1"}},
           {cancel(1, 1, 20), {"C 1/1 30 U"}},
           {cancel(1, 1, 40), {}},
           {cancel(1, 1, 20), {}},
           {enter(2, 1, B::S, 25, 100),
            {"A 2/1 S 25@100 ref=2", "E 2/1 20@100 R m=1",
             "E 1/1 20@100 A m=1"}},
           {cancel(2, 1, 0), {"C 2/1 5 U"}},
       }},
      {"SameUserRefNumOnTwoAccounts",
       {
           {enter(1, 1, B::B, 10, 50), {"A 1/1 B 10@50 ref=1"}},
           {enter(2, 1, B::S, 20, 60), {"A 2/1 S 20@60 ref=2"}},
           {cancel(2, 1, 0), {"C 2/1 20 U"}},
           {cancel(1, 1, 4), {"C 1/1 6 U"}},
           {cancel(3, 1, 0), {}},
       }},
      {"CancelBestAskMovesBestAsk",
       {
           {enter(1, 1, B::S, 5, 105), {"A 1/1 S 5@105 ref=1"}},
           {enter(1, 2, B::S, 5, 110), {"A 1/2 S 5@110 ref=2"}},
           {cancel(1, 1, 0), {"C 1/1 5 U"}},
           {enter(2, 1, B::B, 5, 107), {"A 2/1 B 5@107 ref=3"}},
           {enter(3, 1, B::S, 1, 107),
            {"A 3/1 S 1@107 ref=4", "E 3/1 1@107 R m=1", "E 2/1 1@107 A m=1"}},
       }},
      {"CancelBestBidMovesBestBid",
       {
           {enter(1, 1, B::B, 5, 100), {"A 1/1 B 5@100 ref=1"}},
           {enter(1, 2, B::B, 5, 90), {"A 1/2 B 5@90 ref=2"}},
           {cancel(1, 1, 0), {"C 1/1 5 U"}},
           {enter(2, 1, B::S, 5, 95), {"A 2/1 S 5@95 ref=3"}},
           {enter(3, 1, B::B, 5, 95),
            {"A 3/1 B 5@95 ref=4", "E 3/1 5@95 R m=1", "E 2/1 5@95 A m=1"}},
       }},
      {"FilledOrderCannotBeCancelled",
       {
           {enter(1, 1, B::B, 5, 100), {"A 1/1 B 5@100 ref=1"}},
           {enter(2, 1, B::S, 5, 100),
            {"A 2/1 S 5@100 ref=2", "E 2/1 5@100 R m=1", "E 1/1 5@100 A m=1"}},
           {cancel(1, 1, 0), {}},
           {enter(3, 1, B::S, 5, 100), {"A 3/1 S 5@100 ref=3"}},
       }},
      {"PoolFullCancelsRest",
       {
           {enter(1, 1, B::B, 5, 100), {"A 1/1 B 5@100 ref=1"}},
           {enter(1, 2, B::B, 5, 99), {"A 1/2 B 5@99 ref=2", "C 1/2 5 Z"}},
           {cancel(1, 1, 0), {"C 1/1 5 U"}},
       },
       /*tiny_pool=*/true},
  };
}

template <typename EngineT> void run_scenario(const Scenario &sc) {
  auto in = std::make_unique<mpsc_queue<InboundMessage, QUEUE>>();
  auto out = std::make_unique<spsc_queue<OutboundMessage, QUEUE>>();
  auto engine = std::make_unique<EngineT>(*in, *out);

  auto drain = [&] {
    out->enqueue(OutboundMessage{END, {}});
    std::vector<std::string> got;
    for (;;) {
      OutboundMessage m;
      out->dequeue(m);
      if (m.account == END)
        return got;
      got.push_back(render(m));
    }
  };

  for (size_t i = 0; i < sc.steps.size(); i++) {
    SCOPED_TRACE("step " + std::to_string(i));
    engine->process(sc.steps[i].in);
    EXPECT_EQ(drain(), sc.steps[i].out);
  }
}

class EngineScenario : public ::testing::TestWithParam<Scenario> {};

TEST_P(EngineScenario, ProducesExpectedMessages) {
  if (GetParam().tiny_pool)
    run_scenario<TinyPoolEngine>(GetParam());
  else
    run_scenario<TestEngine>(GetParam());
}

INSTANTIATE_TEST_SUITE_P(
    Sequences, EngineScenario, ::testing::ValuesIn(scenarios()),
    [](const ::testing::TestParamInfo<Scenario> &param_info) {
      return param_info.param.name;
    });

// Random traffic, drained by a consumer thread as in production. Checks
// structural properties of the output rather than exact messages:
//   - every match number appears exactly twice: one 'R' and one 'A' side,
//     same quantity and price
//   - every order is Accepted before any of its Executed/Canceled
//   - an order never gets more shares executed + cancelled than it entered
TEST(EngineRandom, OutputIsConsistent) {
  constexpr int N = 200'000;
  constexpr uint32_t ACCOUNTS = 8;

  auto in = std::make_unique<mpsc_queue<InboundMessage, QUEUE>>();
  auto out = std::make_unique<spsc_queue<OutboundMessage, QUEUE>>();
  auto engine = std::make_unique<Engine<QUEUE, PRICES, 1 << 16>>(*in, *out);

  std::vector<OutboundMessage> seen;
  std::thread consumer([&] {
    for (;;) {
      OutboundMessage m;
      out->dequeue(m);
      if (m.account == END)
        return;
      seen.push_back(m);
    }
  });

  std::mt19937_64 rng(7);
  std::vector<uint32_t> next_urn(ACCOUNTS, 1);
  std::map<std::pair<uint32_t, uint32_t>, uint32_t> entered;
  for (int k = 0; k < N; k++) {
    uint32_t acct = rng() % ACCOUNTS;
    if (rng() % 100 < 55 || next_urn[acct] == 1) {
      uint32_t urn = next_urn[acct]++;
      uint32_t qty = 1 + rng() % 100;
      bool buy = rng() % 2;
      uint64_t price = rng() % 50 == 0 ? MKT : 1 + rng() % (PRICES - 1);
      entered[{acct, urn}] = qty;
      engine->process(enter(acct, urn, buy ? B::B : B::S, qty, price));
    } else {
      uint32_t urn = 1 + rng() % (next_urn[acct] - 1);
      engine->process(cancel(acct, urn, rng() % 3 == 0 ? 10 : 0));
    }
  }
  out->enqueue(OutboundMessage{END, {}});
  consumer.join();

  struct Leg {
    int count = 0;
    uint32_t qty = 0;
    uint64_t price = 0;
    int removed = 0;
  };
  std::map<uint64_t, Leg> matches;
  std::map<std::pair<uint32_t, uint32_t>, uint32_t> used;
  std::map<std::pair<uint32_t, uint32_t>, bool> accepted;

  for (const auto &m : seen) {
    auto decoded = decode(m.bytes.data());
    if (auto *a = std::get_if<AcceptResponseView>(&decoded)) {
      accepted[{m.account, a->UserRefNum()}] = true;
    } else if (auto *e = std::get_if<ExecutedResponseView>(&decoded)) {
      std::pair<uint32_t, uint32_t> id{m.account, e->UserRefNum()};
      ASSERT_TRUE(accepted[id]) << "Executed before Accepted";
      used[id] += e->Quantity();
      auto &leg = matches[e->MatchNumber()];
      if (leg.count == 0) {
        leg.qty = e->Quantity();
        leg.price = e->Price();
      } else {
        EXPECT_EQ(leg.qty, e->Quantity());
        EXPECT_EQ(leg.price, e->Price());
      }
      leg.count++;
      leg.removed += e->LiquidityFlag() == 'R';
    } else if (auto *c = std::get_if<CancelledResponseView>(&decoded)) {
      std::pair<uint32_t, uint32_t> id{m.account, c->UserRefNum()};
      ASSERT_TRUE(accepted[id]) << "Canceled before Accepted";
      used[id] += c->Quantity();
    } else {
      FAIL() << "unexpected outbound type";
    }
  }

  EXPECT_GT(matches.size(), 0u);
  for (const auto &[match, leg] : matches) {
    EXPECT_EQ(leg.count, 2) << "match " << match;
    EXPECT_EQ(leg.removed, 1) << "match " << match;
  }
  for (const auto &[id, qty] : used)
    EXPECT_LE(qty, entered[id]) << "order " << id.first << "/" << id.second;
}

} // namespace
