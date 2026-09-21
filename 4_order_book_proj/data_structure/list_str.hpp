#pragma once

#include "base.hpp"

// Intrusive doubly-linked list nodes. The book owns every allocated node and
// frees them in its destructor. No shared_ptr / weak_ptr overhead. next/prev
// are non-owning; the head/tail pair in OrderBook_List is the sole owner.

class BuyBlock {
public:
  BuyOrder order;
  BuyBlock *next = nullptr;
  BuyBlock *prev = nullptr;
};

class SellBlock {
public:
  SellOrder order;
  SellBlock *next = nullptr;
  SellBlock *prev = nullptr;
};

class OrderBook_List final : public OrderBook {
  uint64_t counter = 1; // 0 reserved as the failed operation

  BuyBlock *buy_head = nullptr;
  BuyBlock *buy_tail = nullptr;
  SellBlock *sell_head = nullptr;
  SellBlock *sell_tail = nullptr;

  bool match();

public:
  OrderBook_List() = default;
  ~OrderBook_List() override;

  // Raw ownership => no default copy semantics.
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
