#pragma once

#include "base.hpp"
#include <array>
#include <cstdint>
#include <unordered_map>

template <int Size> class OrderBook_TickOffset : public OrderBook {
  struct BuyList {
    BuyBlock *head;
    BuyBlock *tail;
  };
  struct SellList {
    SellBlock *head;
    SellBlock *tail;
  };

  uint64_t counter = 1; // 0 reserved as the failed operation
  std::unordered_map<uint64_t, BuyBlock *> buy_keys;
  std::unordered_map<uint64_t, SellBlock *> sell_keys;

  std::array<BuyList *, Size> buy_buffer{};
  std::array<SellList *, Size> sell_buffer{};
  uint64_t best_buy_idx = 0;
  uint64_t best_sell_idx = 0;
  bool match();

public:
  uint64_t insert_buy_order(BuyOrder) override;
  uint64_t insert_sell_order(SellOrder) override;
  uint64_t cancel_buy_order(uint64_t) override;
  uint64_t cancel_sell_order(uint64_t) override;
  uint64_t market_buy_order(BuyOrder) override;
  uint64_t market_sell_order(SellOrder) override;
  std::pair<BuyOrder, SellOrder> top_of_book() override;
};
