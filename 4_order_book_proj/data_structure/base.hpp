#pragma once

#include <cstdint>
#include <utility>

enum class OrderType { LIMIT, MARKET };

class BuyOrder {
public:
  uint64_t id = 0;
  uint64_t price = 0;
  uint32_t size = 0;

  bool operator>(const BuyOrder &o2) const {
    if (price > o2.price)
      return true;
    else if (price == o2.price)
      return id < o2.id;
    else
      return false;
  }

  bool operator<(const BuyOrder &o2) const {
    if (price < o2.price)
      return true;
    else if (price == o2.price)
      return id > o2.id;
    else
      return false;
  }
};

class SellOrder {
public:
  uint64_t id = 0;
  uint64_t price = 0;
  uint32_t size = 0;

  bool operator>(const SellOrder &o2) const {

    if (price < o2.price)
      return true;
    else if (price == o2.price)
      return id < o2.id;
    else
      return false;
  }

  bool operator<(const SellOrder &o2) const {
    if (price > o2.price)
      return true;
    else if (price == o2.price)
      return id > o2.id;
    else
      return false;
  }
};

// Interface contract. Concrete implementations inherit publicly and override
// every method. Benchmark dispatches statically via templates on the concrete
// type, so the vtable is not hit on the hot path; the base exists for
// documentation and to catch signature drift at compile time.
class OrderBook {
public:
  virtual ~OrderBook() = default;

  virtual uint64_t insert_buy_order(BuyOrder) = 0;
  virtual uint64_t insert_sell_order(SellOrder) = 0;
  virtual uint64_t cancel_buy_order(uint64_t) = 0;
  virtual uint64_t cancel_sell_order(uint64_t) = 0;
  virtual uint64_t market_buy_order(BuyOrder) = 0;
  virtual uint64_t market_sell_order(SellOrder) = 0;

  virtual std::pair<BuyOrder, SellOrder> top_of_book() = 0;
};

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
