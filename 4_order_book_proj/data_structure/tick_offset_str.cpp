#include "tick_offset_str.hpp"
#include <algorithm>

template <int Size> void OrderBook_TickOffset<Size>::unlink(BuyBlock *n) {
  if (n->prev)
    n->prev->next = n->next;
  if (n->next)
    n->next->prev = n->prev;
  n->prev = nullptr;
  n->next = nullptr;
}

template <int Size> void OrderBook_TickOffset<Size>::unlink(SellBlock *n) {
  if (n->prev)
    n->prev->next = n->next;
  if (n->next)
    n->next->prev = n->prev;
  n->prev = nullptr;
  n->next = nullptr;
}

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

  best_sell_idx = best_sell_idx == Size ? order.price
                                        : std::min(best_sell_idx, order.price);
  sell_keys[order.id] = new_node;
  match();
  return order.id;
}
template <int Size>
uint64_t OrderBook_TickOffset<Size>::cancel_buy_order(uint64_t id) {
  if (!buy_keys.contains(id)) {
    return 0;
  }
  auto order_block = buy_keys[id];
  uint64_t price = order_block->order.price;
  if (order_block->prev == nullptr && order_block->next == nullptr) {
    delete buy_buffer[price];
    buy_buffer[price] = nullptr;
  } else if (order_block->next == nullptr) {
    buy_buffer[price]->tail = order_block->prev;
  } else if (order_block->prev == nullptr) {
    buy_buffer[price]->head = order_block->next;
  }
  unlink(order_block);
  buy_keys.erase(order_block->order.id);
  delete order_block;
  while (best_buy_idx > 0 && buy_buffer[best_buy_idx] == nullptr) {
    best_buy_idx--;
  }
  return id;
}

template <int Size>
uint64_t OrderBook_TickOffset<Size>::cancel_sell_order(uint64_t id) {
  if (!sell_keys.contains(id)) {
    return 0;
  }
  auto order_block = sell_keys[id];
  uint64_t price = order_block->order.price;
  if (order_block->prev == nullptr && order_block->next == nullptr) {
    delete sell_buffer[price];
    sell_buffer[price] = nullptr;
  } else if (order_block->next == nullptr) {
    sell_buffer[price]->tail = order_block->prev;
  } else if (order_block->prev == nullptr) {
    sell_buffer[price]->head = order_block->next;
  }
  unlink(order_block);
  sell_keys.erase(order_block->order.id);
  delete order_block;
  while (best_sell_idx < Size && sell_buffer[best_sell_idx] == nullptr) {
    best_sell_idx++;
  }
  return id;
}

template <int Size>
uint64_t OrderBook_TickOffset<Size>::market_buy_order(BuyOrder order) {
  uint64_t remaining = order.size;
  uint64_t last_id = 0;
  while (remaining > 0 && best_sell_idx < Size) {
    SellBlock *n = sell_buffer[best_sell_idx]->head;
    last_id = n->order.id;
    if (n->order.size <= remaining) {
      remaining -= n->order.size;
      sell_keys.erase(n->order.id);
      if (n->next == nullptr) {
        delete sell_buffer[best_sell_idx];
        sell_buffer[best_sell_idx] = nullptr;
      } else {
        sell_buffer[best_sell_idx]->head = n->next;
        sell_buffer[best_sell_idx]->head->prev = nullptr;
      }
      unlink(n);
      delete n;
      while (best_sell_idx < Size && sell_buffer[best_sell_idx] == nullptr) {
        best_sell_idx++;
      }
    } else {
      n->order.size -= remaining;
      remaining = 0;
    }
  }
  if (last_id == 0) {
    order.price = Size - 1;
    return insert_buy_order(order);
  }
  return last_id;
}

template <int Size>
uint64_t OrderBook_TickOffset<Size>::market_sell_order(SellOrder order) {
  uint64_t remaining = order.size;
  uint64_t last_id = 0;
  while (remaining > 0 && best_buy_idx > 0) {
    BuyBlock *n = buy_buffer[best_buy_idx]->head;
    last_id = n->order.id;
    if (n->order.size <= remaining) {
      remaining -= n->order.size;
      buy_keys.erase(n->order.id);
      if (n->next == nullptr) {
        delete buy_buffer[best_buy_idx];
        buy_buffer[best_buy_idx] = nullptr;
      } else {
        buy_buffer[best_buy_idx]->head = n->next;
        buy_buffer[best_buy_idx]->head->prev = nullptr;
      }
      unlink(n);
      delete n;
      while (best_buy_idx > 0 && buy_buffer[best_buy_idx] == nullptr) {
        best_buy_idx--;
      }
    } else {
      n->order.size -= remaining;
      remaining = 0;
    }
  }
  if (last_id == 0) {
    order.price = 1;
    return insert_sell_order(order);
  }
  return last_id;
}

template <int Size> bool OrderBook_TickOffset<Size>::match() {
  bool res = false;

  while (best_buy_idx > 0 && best_sell_idx < Size &&
         best_buy_idx >= best_sell_idx) {
    BuyOrder &b = buy_buffer[best_buy_idx]->head->order;
    SellOrder &s = sell_buffer[best_sell_idx]->head->order;
    uint64_t fill = std::min(b.size, s.size);
    res = true;
    b.size -= fill;
    s.size -= fill;
    bool pop_b = b.size == 0;
    bool pop_s = s.size == 0;
    if (pop_b) {
      auto old_head = buy_buffer[best_buy_idx]->head;
      buy_keys.erase(old_head->order.id);
      if (old_head->next == nullptr) {
        delete buy_buffer[best_buy_idx];
        buy_buffer[best_buy_idx] = nullptr;
      } else {
        buy_buffer[best_buy_idx]->head = old_head->next;
        buy_buffer[best_buy_idx]->head->prev = nullptr;
      }
      unlink(old_head);
      delete old_head;
      while (best_buy_idx > 0 && buy_buffer[best_buy_idx] == nullptr) {
        best_buy_idx--;
      }
    }
    if (pop_s) {
      auto old_head = sell_buffer[best_sell_idx]->head;
      sell_keys.erase(old_head->order.id);
      if (old_head->next == nullptr) {
        delete sell_buffer[best_sell_idx];
        sell_buffer[best_sell_idx] = nullptr;
      } else {
        sell_buffer[best_sell_idx]->head = old_head->next;
        sell_buffer[best_sell_idx]->head->prev = nullptr;
      }
      unlink(old_head);
      delete old_head;
      while (best_sell_idx < Size && sell_buffer[best_sell_idx] == nullptr) {
        best_sell_idx++;
      }
    }
  }

  return res;
}

template <int Size>
std::pair<BuyOrder, SellOrder> OrderBook_TickOffset<Size>::top_of_book() {
  BuyOrder best_buy;
  SellOrder best_sell;
  if (best_buy_idx != 0) {
    best_buy = buy_buffer[best_buy_idx]->head->order;
  }
  if (best_sell_idx != Size) {
    best_sell = sell_buffer[best_sell_idx]->head->order;
  }
  return {best_buy, best_sell};
}

template <int Size> OrderBook_TickOffset<Size>::~OrderBook_TickOffset() {
  for (uint64_t i = 0; i < Size; i++) {
    if (buy_buffer[i]) {
      BuyBlock *cur = buy_buffer[i]->head;
      while (cur) {
        BuyBlock *nxt = cur->next;
        delete cur;
        cur = nxt;
      }
      delete buy_buffer[i];
    }
    if (sell_buffer[i]) {
      SellBlock *cur = sell_buffer[i]->head;
      while (cur) {
        SellBlock *nxt = cur->next;
        delete cur;
        cur = nxt;
      }
      delete sell_buffer[i];
    }
  }
}

template class OrderBook_TickOffset<1024>;
