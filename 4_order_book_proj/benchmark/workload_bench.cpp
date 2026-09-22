// Order-book workload benchmark.
//
// Op mix from DATA_STRUCTURE.md:
//   insert 45%, cancel 43%, market 2%, top-of-book (levels proxy) 10%.
//
// Op script is generated once per (seed, size) and replayed against a freshly
// warmed-up book for each impl. Same sequence hits every impl, so results are
// directly comparable.
//
// To benchmark a new order-book type:
//   1. Include its header.
//   2. Add one line:
//        BENCHMARK_TEMPLATE(BM_Workload, OrderBook_Yours)->Args({...});
//   No other changes needed. Static dispatch, no vtable overhead.

#include "list_str.hpp"
#include "sorted_vector_str.hpp"
#include "tick_offset_str.hpp"

#include <benchmark/benchmark.h>
#include <cstdint>
#include <random>
#include <vector>

namespace {

struct Op {
  enum Kind : uint8_t {
    INS_BUY,
    INS_SELL,
    CAN_BUY,
    CAN_SELL,
    MKT_BUY,
    MKT_SELL,
    TOP,
  };
  Kind kind;
  uint64_t price;
  uint64_t size;
  uint64_t pick; // for CAN_*: index-mod into live-ids at replay time
};

// Deterministic op script. Same seed => same sequence, so different order-book
// impls run identical workloads.
std::vector<Op> generate(uint64_t n_ops, uint64_t seed) {
  std::mt19937_64 rng(seed);
  std::uniform_int_distribution<int> mix(0, 99);
  std::uniform_int_distribution<int> side(0, 1);
  std::uniform_int_distribution<uint64_t> price(1, 1000);
  std::uniform_int_distribution<uint64_t> size(1, 100);

  std::vector<Op> ops;
  ops.reserve(n_ops);
  for (uint64_t i = 0; i < n_ops; i++) {
    int r = mix(rng);
    Op op{};
    op.price = price(rng);
    op.size = size(rng);
    op.pick = rng();
    bool is_buy = side(rng) == 0;
    if (r < 45)
      op.kind = is_buy ? Op::INS_BUY : Op::INS_SELL;
    else if (r < 88)
      op.kind = is_buy ? Op::CAN_BUY : Op::CAN_SELL;
    else if (r < 90)
      op.kind = is_buy ? Op::MKT_BUY : Op::MKT_SELL;
    else
      op.kind = Op::TOP;
    ops.push_back(op);
  }
  return ops;
}

template <typename Book>
void warmup(Book &book, uint64_t depth, uint64_t seed,
            std::vector<uint64_t> &buy_ids, std::vector<uint64_t> &sell_ids) {
  std::mt19937_64 rng(seed);
  std::uniform_int_distribution<int> side(0, 1);
  std::uniform_int_distribution<uint64_t> price(1, 1000);
  std::uniform_int_distribution<uint64_t> size(1, 100);
  for (uint64_t i = 0; i < depth; i++) {
    if (side(rng) == 0) {
      BuyOrder o{0, price(rng), size(rng)};
      buy_ids.push_back(book.insert_buy_order(o));
    } else {
      SellOrder o{0, price(rng), size(rng)};
      sell_ids.push_back(book.insert_sell_order(o));
    }
  }
}

template <typename Book>
void replay(Book &book, const std::vector<Op> &ops,
            std::vector<uint64_t> &buy_ids, std::vector<uint64_t> &sell_ids) {
  for (const auto &op : ops) {
    switch (op.kind) {
    case Op::INS_BUY: {
      BuyOrder o{0, op.price, op.size};
      buy_ids.push_back(book.insert_buy_order(o));
      break;
    }
    case Op::INS_SELL: {
      SellOrder o{0, op.price, op.size};
      sell_ids.push_back(book.insert_sell_order(o));
      break;
    }
    case Op::CAN_BUY: {
      if (buy_ids.empty())
        break;
      auto idx = op.pick % buy_ids.size();
      book.cancel_buy_order(buy_ids[idx]);
      buy_ids[idx] = buy_ids.back();
      buy_ids.pop_back();
      break;
    }
    case Op::CAN_SELL: {
      if (sell_ids.empty())
        break;
      auto idx = op.pick % sell_ids.size();
      book.cancel_sell_order(sell_ids[idx]);
      sell_ids[idx] = sell_ids.back();
      sell_ids.pop_back();
      break;
    }
    case Op::MKT_BUY: {
      BuyOrder o{0, 0, op.size};
      benchmark::DoNotOptimize(book.market_buy_order(o));
      break;
    }
    case Op::MKT_SELL: {
      SellOrder o{0, 0, op.size};
      benchmark::DoNotOptimize(book.market_sell_order(o));
      break;
    }
    case Op::TOP: {
      auto t = book.top_of_book();
      benchmark::DoNotOptimize(t);
      break;
    }
    }
  }
}

} // namespace

template <typename Book> static void BM_Workload(benchmark::State &state) {
  const uint64_t depth = state.range(0);
  const uint64_t n_ops = state.range(1);

  const auto ops = generate(n_ops, /*seed=*/42);

  for (auto _ : state) {
    state.PauseTiming();
    Book book;
    std::vector<uint64_t> buy_ids, sell_ids;
    buy_ids.reserve(depth + n_ops);
    sell_ids.reserve(depth + n_ops);
    warmup(book, depth, /*seed=*/7, buy_ids, sell_ids);
    state.ResumeTiming();

    replay(book, ops, buy_ids, sell_ids);
  }
  state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) *
                          static_cast<int64_t>(n_ops));
}

// {book_depth, ops_per_iteration}. Tune upper end if runs get slow.
BENCHMARK_TEMPLATE(BM_Workload, OrderBook_SortedVector)
    ->Args({100, 10000})
    ->Args({1000, 10000})
    ->Args({10000, 5000})
    ->Args({50000, 1000});

BENCHMARK_TEMPLATE(BM_Workload, OrderBook_List)
    ->Args({100, 10000})
    ->Args({1000, 10000})
    ->Args({10000, 5000})
    ->Args({50000, 1000});

BENCHMARK_TEMPLATE(BM_Workload, OrderBook_TickOffset<1024>)
    ->Args({100, 10000})
    ->Args({1000, 10000})
    ->Args({10000, 5000})
    ->Args({50000, 1000});

BENCHMARK_MAIN();
