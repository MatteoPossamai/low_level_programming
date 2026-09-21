#pragma once

#include "base.hpp"
#include <unordered_map>

class OrderBook_List final : public OrderBook {
  uint64_t counter = 1; // 0 reserved as the failed operation

  BuyBlock *buy_head = nullptr;
  BuyBlock *buy_tail = nullptr;
  SellBlock *sell_head = nullptr;
  SellBlock *sell_tail = nullptr;

  std::unordered_map<uint64_t, BuyBlock *> buy_index;
  std::unordered_map<uint64_t, SellBlock *> sell_index;

  bool match();

  void unlink(BuyBlock *n);
  void unlink(SellBlock *n);

public:
  OrderBook_List() = default;
  ~OrderBook_List() override;

  OrderBook_List(const OrderBook_List &) = delete;
  OrderBook_List &operator=(const OrderBook_List &) = delete;

  uint64_t insert_buy_order(BuyOrder) override;
  uint64_t insert_sell_order(SellOrder) override;
  uint64_t cancel_buy_order(uint64_t) override;
  uint64_t cancel_sell_order(uint64_t) override;
  uint64_t market_buy_order(BuyOrder) override;
  uint64_t market_sell_order(SellOrder) override;
  std::pair<BuyOrder, SellOrder> top_of_book() override;
};
