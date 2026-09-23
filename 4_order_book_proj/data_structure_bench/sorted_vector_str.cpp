#include "sorted_vector_str.hpp"
#include <algorithm>
#include <cstdint>

uint64_t OrderBook_SortedVector::insert_buy_order(BuyOrder order) {
  order.id = ++counter;
  auto iter = std::lower_bound(buy_book.begin(), buy_book.end(), order);
  buy_book.insert(iter, std::move(order));
  match();
  return counter;
}

uint64_t OrderBook_SortedVector::insert_sell_order(SellOrder order) {
  order.id = ++counter;
  auto iter = std::lower_bound(sell_book.begin(), sell_book.end(), order);
  sell_book.insert(iter, std::move(order));
  match();
  return counter;
}

uint64_t OrderBook_SortedVector::cancel_buy_order(uint64_t id) {
  auto it = std::find_if(buy_book.begin(), buy_book.end(),
                         [id](const BuyOrder &o) { return o.id == id; });
  if (it == buy_book.end())
    return 0;
  buy_book.erase(it);
  return id;
}

uint64_t OrderBook_SortedVector::cancel_sell_order(uint64_t id) {
  auto it = std::find_if(sell_book.begin(), sell_book.end(),
                         [id](const SellOrder &o) { return o.id == id; });
  if (it == sell_book.end())
    return 0;
  sell_book.erase(it);
  return id;
}

uint64_t OrderBook_SortedVector::market_buy_order(BuyOrder order) {
  uint64_t remaining = order.size;
  uint64_t last_id = 0;
  while (remaining > 0 && !sell_book.empty()) {
    SellOrder &best = sell_book.back();
    last_id = best.id;
    if (best.size <= remaining) {
      remaining -= best.size;
      sell_book.pop_back();
    } else {
      best.size -= remaining;
      remaining = 0;
    }
  }
  if (last_id == 0) {
    order.price = UINT64_MAX;
    return insert_buy_order(order);
  }
  return last_id;
}

uint64_t OrderBook_SortedVector::market_sell_order(SellOrder order) {
  uint64_t remaining = order.size;
  uint64_t last_id = 0;
  while (remaining > 0 && !buy_book.empty()) {
    BuyOrder &best = buy_book.back();
    last_id = best.id;
    if (best.size <= remaining) {
      remaining -= best.size;
      buy_book.pop_back();
    } else {
      best.size -= remaining;
      remaining = 0;
    }
  }
  if (last_id == 0) {
    order.price = 0;
    return insert_sell_order(order);
  }
  return last_id;
}

bool OrderBook_SortedVector::match() {
  bool res = false;
  while (!buy_book.empty() && !sell_book.empty() &&
         buy_book.back().price >= sell_book.back().price) {
    BuyOrder &b = buy_book.back();
    SellOrder &s = sell_book.back();
    uint64_t fill = std::min(b.size, s.size);
    b.size -= fill;
    s.size -= fill;
    bool pop_b = b.size == 0;
    bool pop_s = s.size == 0;
    if (pop_b)
      buy_book.pop_back();
    if (pop_s)
      sell_book.pop_back();
    res = true;
  }
  return res;
}

std::pair<BuyOrder, SellOrder> OrderBook_SortedVector::top_of_book() {
  auto top_buy = buy_book.empty() ? BuyOrder() : buy_book.back();
  auto top_sell = sell_book.empty() ? SellOrder() : sell_book.back();
  return {top_buy, top_sell};
}
