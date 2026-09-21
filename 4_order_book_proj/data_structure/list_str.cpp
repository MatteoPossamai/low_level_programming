#include "list_str.hpp"

// Destructor: walks each list and deletes every node. Boilerplate RAII; the
// algorithm methods below are stubs for you to fill in.
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

// Stubs so the target links. Benchmark numbers for OrderBook_List are
// meaningless until these are implemented.

uint64_t OrderBook_List::insert_buy_order(BuyOrder) { return 0; }
uint64_t OrderBook_List::insert_sell_order(SellOrder) { return 0; }
uint64_t OrderBook_List::cancel_buy_order(uint64_t) { return 0; }
uint64_t OrderBook_List::cancel_sell_order(uint64_t) { return 0; }
uint64_t OrderBook_List::market_buy_order(BuyOrder) { return 0; }
uint64_t OrderBook_List::market_sell_order(SellOrder) { return 0; }

std::pair<BuyOrder, SellOrder> OrderBook_List::top_of_book() { return {}; }

bool OrderBook_List::match() { return false; }
