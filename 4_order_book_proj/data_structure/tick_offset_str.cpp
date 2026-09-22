#include "tick_offset_str.hpp"

template <int Size>
uint64_t OrderBook_TickOffset<Size>::insert_buy_order(BuyOrder order) {
  if (order.price == 0 || order.price >= Size)
    return 0;
  auto new_node = new BuyBlock();
  order.id = counter++;
  new_node->order = order;
  if (buy_buffer[order.price] == nullptr) {
    auto list = new BuyList();
    buy_buffer[order.price] = list;
    list->head = new_node;
    list->tail = new_node;
  } else {
    BuyList *lst = buy_buffer[order.price];
    new_node->prev = lst->tail;
    new_node->next = nullptr;
    lst->tail->next = new_node;
    lst->tail = new_node;
  }

  best_buy_idx =
      best_buy_idx == 0 ? order.price : std::max(best_buy_idx, order.price);
  buy_keys[order.id] = new_node;
  match();
  return order.id;
}

template <int Size>
uint64_t OrderBook_TickOffset<Size>::insert_sell_order(SellOrder order) {
  if (order.price == 0 || order.price >= Size)
    return 0;
  auto new_node = new SellBlock();
  order.id = counter++;
  new_node->order = order;
  if (sell_buffer[order.price] == nullptr) {
    auto list = new SellList();
    sell_buffer[order.price] = list;
    list->head = new_node;
    list->tail = new_node;
  } else {
    SellList *lst = sell_buffer[order.price];
    new_node->prev = lst->tail;
    new_node->next = nullptr;
    lst->tail->next = new_node;
    lst->tail = new_node;
  }

  best_sell_idx =
      best_sell_idx == 0 ? order.price : std::min(best_sell_idx, order.price);
  sell_keys[order.id] = new_node;
  match();
  return order.id;
}
//
// template <int Size>
// uint64_t OrderBook_TickOffset<Size>::cancel_buy_order(uint64_t id) {}
//
// template <int Size>
// uint64_t OrderBook_TickOffset<Size>::cancel_sell_order(uint64_t id) {}
//
// template <int Size>
// uint64_t OrderBook_TickOffset<Size>::market_buy_order(BuyOrder order) {}
//
// template <int Size>
// uint64_t OrderBook_TickOffset<Size>::market_sell_order(SellOrder order) {}
//
// template <int Size> bool OrderBook_TickOffset<Size>::match() {}
//

template <int Size>
std::pair<BuyOrder, SellOrder> OrderBook_TickOffset<Size>::top_of_book() {
  BuyOrder best_buy;
  SellOrder best_sell;
  if (best_buy_idx != 0) {
    best_buy = buy_buffer[best_buy_idx]->head->order;
  }
  if (best_sell_idx != 0) {
    best_sell = sell_buffer[best_sell_idx]->head->order;
  }
  return {best_buy, best_sell};
}

int main() {
  OrderBook_TickOffset<100000> test;
  return 0;
}
