#include "list_str.hpp"
#include <algorithm>
#include <cstdint>

OrderBook_List::~OrderBook_List() {
  while (buy_head) {
    BuyBlock *nxt = buy_head->next;
    delete buy_head;
    buy_head = nxt;
  }
  while (sell_head) {
    SellBlock *nxt = sell_head->next;
    delete sell_head;
    sell_head = nxt;
  }
}

uint64_t OrderBook_List::insert_buy_order(BuyOrder order) {
  order.id = ++counter;

  BuyBlock *node = new BuyBlock();
  node->order = order;

  BuyBlock *cur = buy_head;
  while (cur && cur->order > order)
    cur = cur->next;

  if (cur) {
    node->next = cur;
    node->prev = cur->prev;
    if (cur->prev)
      cur->prev->next = node;
    else
      buy_head = node;
    cur->prev = node;
  } else {
    node->prev = buy_tail;
    node->next = nullptr;
    if (buy_tail)
      buy_tail->next = node;
    else
      buy_head = node;
    buy_tail = node;
  }

  buy_index[order.id] = node;
  match();
  return order.id;
}

uint64_t OrderBook_List::insert_sell_order(SellOrder order) {
  order.id = ++counter;

  SellBlock *node = new SellBlock();
  node->order = order;

  SellBlock *cur = sell_head;
  while (cur && cur->order > order)
    cur = cur->next;

  if (cur) {
    node->next = cur;
    node->prev = cur->prev;
    if (cur->prev)
      cur->prev->next = node;
    else
      sell_head = node;
    cur->prev = node;
  } else {
    node->prev = sell_tail;
    node->next = nullptr;
    if (sell_tail)
      sell_tail->next = node;
    else
      sell_head = node;
    sell_tail = node;
  }

  sell_index[order.id] = node;
  match();
  return order.id;
}

void OrderBook_List::unlink(BuyBlock *n) {
  if (n->prev)
    n->prev->next = n->next;
  else
    buy_head = n->next;
  if (n->next)
    n->next->prev = n->prev;
  else
    buy_tail = n->prev;
  n->next = nullptr;
  n->prev = nullptr;
}

void OrderBook_List::unlink(SellBlock *n) {
  if (n->prev)
    n->prev->next = n->next;
  else
    sell_head = n->next;
  if (n->next)
    n->next->prev = n->prev;
  else
    sell_tail = n->prev;
  n->next = nullptr;
  n->prev = nullptr;
}

uint64_t OrderBook_List::cancel_buy_order(uint64_t id) {
  auto it = buy_index.find(id);
  if (it == buy_index.end())
    return 0;
  BuyBlock *n = it->second;
  unlink(n);
  buy_index.erase(it);
  delete n;
  return id;
}

uint64_t OrderBook_List::cancel_sell_order(uint64_t id) {
  auto it = sell_index.find(id);
  if (it == sell_index.end())
    return 0;
  SellBlock *n = it->second;
  unlink(n);
  sell_index.erase(it);
  delete n;
  return id;
}

uint64_t OrderBook_List::market_buy_order(BuyOrder order) {
  uint64_t remaining = order.size;
  uint64_t last_id = 0;
  while (remaining > 0 && sell_head) {
    SellBlock *n = sell_head;
    last_id = n->order.id;
    if (n->order.size <= remaining) {
      remaining -= n->order.size;
      unlink(n);
      sell_index.erase(n->order.id);
      delete n;
    } else {
      n->order.size -= remaining;
      remaining = 0;
    }
  }
  if (last_id == 0) {
    order.price = UINT64_MAX;
    return insert_buy_order(order);
  }
  return last_id;
}

uint64_t OrderBook_List::market_sell_order(SellOrder order) {
  uint64_t remaining = order.size;
  uint64_t last_id = 0;
  while (remaining > 0 && buy_head) {
    BuyBlock *n = buy_head;
    last_id = n->order.id;
    if (n->order.size <= remaining) {
      remaining -= n->order.size;
      unlink(n);
      buy_index.erase(n->order.id);
      delete n;
    } else {
      n->order.size -= remaining;
      remaining = 0;
    }
  }
  if (last_id == 0) {
    order.price = 0;
    return insert_sell_order(order);
  }
  return last_id;
}

std::pair<BuyOrder, SellOrder> OrderBook_List::top_of_book() {
  BuyOrder b = buy_head ? buy_head->order : BuyOrder();
  SellOrder s = sell_head ? sell_head->order : SellOrder();
  return {b, s};
}

bool OrderBook_List::match() {
  bool did = false;
  while (buy_head && sell_head &&
         buy_head->order.price >= sell_head->order.price) {
    uint64_t fill = std::min(buy_head->order.size, sell_head->order.size);
    buy_head->order.size -= fill;
    sell_head->order.size -= fill;
    if (buy_head->order.size == 0) {
      BuyBlock *b = buy_head;
      unlink(b);
      buy_index.erase(b->order.id);
      delete b;
    }
    if (sell_head->order.size == 0) {
      SellBlock *s = sell_head;
      unlink(s);
      sell_index.erase(s->order.id);
      delete s;
    }
    did = true;
  }
  return did;
}
