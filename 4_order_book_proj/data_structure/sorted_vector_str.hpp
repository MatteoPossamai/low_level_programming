#pragma once

#include "base.hpp"
#include <vector>

class OrderBook_SortedVector final : public OrderBook {
  std::vector<BuyOrder> buy_book;
  std::vector<SellOrder> sell_book;
  uint64_t counter = 1; // 0 reserved as the failed operation
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
