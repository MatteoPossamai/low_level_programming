#pragma once
#include "allocator.hpp"
#include "messages.hpp"
#include "queues.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <unordered_map>

template <size_t QUEUE_SIZE, size_t BUFFER_SIZE, size_t ALLOCATOR_SIZE>
class Engine {
  constexpr static uint64_t MARKETPRICE = 0x7FFFFFFF;
  constexpr static uint64_t MAXLIMIT = 1'999'999'9900;

  mpsc_queue<InboundMessage, QUEUE_SIZE> &incoming_queue;
  spsc_queue<OUCHMessageOut, QUEUE_SIZE> &outgoing_queue;

  struct OrderBlock {
    std::array<std::byte, EnterRequestView::WIRE_SIZE> raw; // own copy
    uint32_t curr_qty;
    uint64_t id;  // engine-assigned, the exchange Order Reference Number
    uint64_t key; // order_key(account, UserRefNum), used by cancel
    OrderBlock *prev;
    OrderBlock *next;
  };

  Allocator<OrderBlock, ALLOCATOR_SIZE> allocator{};

  struct OrderList {
    OrderBlock *head = nullptr;
    OrderBlock *tail = nullptr;
  };

  // An order is identified by (OUCH account, UserRefNum). UserRefNum is 32 bits,
  // so both fit in one 64-bit key.
  static uint64_t order_key(uint32_t account, uint32_t user_ref_num) {
    return (static_cast<uint64_t>(account) << 32) | user_ref_num;
  }

  uint64_t counter = 0;
  std::unordered_map<uint64_t, OrderBlock *> order_blocks_map;

  std::array<OrderList, BUFFER_SIZE> order_buffer{};
  uint64_t best_buy_idx = 0;
  uint64_t best_sell_idx = BUFFER_SIZE;

  void unlink(OrderBlock *block, OrderList *list);
  void append_list(OrderBlock *block, OrderList *list);

  uint64_t insert_buy_order(uint32_t account, EnterRequestView order);
  uint64_t insert_sell_order(uint32_t account, EnterRequestView order);

public:
  Engine(mpsc_queue<InboundMessage, QUEUE_SIZE> &incoming_queue_in,
         spsc_queue<OUCHMessageOut, QUEUE_SIZE> &outgoing_queue_in)
      : incoming_queue(incoming_queue_in), outgoing_queue(outgoing_queue_in) {}
  // Blocks live in allocator's mmap region, released when allocator dies.
  ~Engine() = default;

  Engine(const Engine &) = delete;
  Engine(Engine &&) = delete;
  Engine &operator=(const Engine &) = delete;

  uint64_t insert_order(uint32_t account, EnterRequestView order);
  uint32_t cancel_order(uint32_t account, CancelRequestView cancel);
};

template <size_t QUEUE_SIZE, size_t BUFFER_SIZE, size_t ALLOCATOR_SIZE>
void Engine<QUEUE_SIZE, BUFFER_SIZE, ALLOCATOR_SIZE>::unlink(
    OrderBlock *block, OrderList *list) {
  if (block->prev)
    block->prev->next = block->next;
  else
    list->head = block->next;
  if (block->next)
    block->next->prev = block->prev;
  else
    list->tail = block->prev;

  block->prev = nullptr;
  block->next = nullptr;
}

template <size_t QUEUE_SIZE, size_t BUFFER_SIZE, size_t ALLOCATOR_SIZE>
void Engine<QUEUE_SIZE, BUFFER_SIZE, ALLOCATOR_SIZE>::append_list(
    OrderBlock *block, OrderList *list) {
  block->next = nullptr;
  block->prev = list->tail;
  if (list->tail)
    list->tail->next = block;
  else
    list->head = block;
  list->tail = block;
}

template <size_t QUEUE_SIZE, size_t BUFFER_SIZE, size_t ALLOCATOR_SIZE>
uint64_t Engine<QUEUE_SIZE, BUFFER_SIZE, ALLOCATOR_SIZE>::insert_order(
    uint32_t account, EnterRequestView order) {
  switch (order.Side()) {
  case SideEnum::B:
    return insert_buy_order(account, order);
  case SideEnum::S:
    return insert_sell_order(account, order);
  default:
    throw std::invalid_argument("Invalid side taken from the order");
  }
}

template <size_t QUEUE_SIZE, size_t BUFFER_SIZE, size_t ALLOCATOR_SIZE>
uint64_t Engine<QUEUE_SIZE, BUFFER_SIZE, ALLOCATOR_SIZE>::insert_buy_order(
    uint32_t account, EnterRequestView order) {
  // TODO: broadcast out of the engine when there are fills/partial fills

  // No need to handle the market order, since that is already very high number
  uint32_t curr_qty = order.Quantity();
  while (best_sell_idx < BUFFER_SIZE && best_sell_idx <= order.Price() &&
         curr_qty > 0) {
    OrderList *list = &order_buffer[best_sell_idx];
    OrderBlock *block = list->head;
    if (block->curr_qty > curr_qty) {
      // Inserted order got filled
      block->curr_qty -= curr_qty;
      curr_qty = 0;
    } else {
      // Resting qty <= incoming: resting fully filled, remove it
      order_blocks_map.erase(block->key);
      curr_qty -= block->curr_qty;
      unlink(block, list);
      allocator.deallocate(block);
      while (best_sell_idx < BUFFER_SIZE &&
             order_buffer[best_sell_idx].head == nullptr) {
        best_sell_idx++;
      }
    }
  }
  if (curr_qty > 0) {
    if (order.Price() == MARKETPRICE)
      return 0; // unfilled rest is cancelled, never rests. TODO: send Cancelled
    auto new_block = allocator.allocate();
    if (new_block == nullptr)
      return 0; // book full: unfilled rest is cancelled. TODO: send Cancelled
    if (order.Price() > best_buy_idx)
      best_buy_idx = order.Price();
    new_block->id = ++counter;
    new_block->key = order_key(account, order.UserRefNum());
    new_block->curr_qty = curr_qty;
    std::memcpy(new_block->raw.data(), order.data(), EnterRequestView::WIRE_SIZE);
    append_list(new_block, &order_buffer[order.Price()]);
    order_blocks_map[new_block->key] = new_block;
    return new_block->id;
  }
  return 0; // Signals that was inserted OK and already filled
}

template <size_t QUEUE_SIZE, size_t BUFFER_SIZE, size_t ALLOCATOR_SIZE>
uint64_t Engine<QUEUE_SIZE, BUFFER_SIZE, ALLOCATOR_SIZE>::insert_sell_order(
    uint32_t account, EnterRequestView order) {
  // TODO: broadcast out of the engine when there are fills/partial fills

  // Market sell must hit any bid, so match it as the lowest possible limit
  uint64_t limit = order.Price() == MARKETPRICE ? 0 : order.Price();
  uint32_t curr_qty = order.Quantity();
  // best_buy_idx stops at 0 when bids run out, so check the level is non-empty
  while (order_buffer[best_buy_idx].head != nullptr &&
         best_buy_idx >= limit && curr_qty > 0) {
    OrderList *list = &order_buffer[best_buy_idx];
    OrderBlock *block = list->head;
    if (block->curr_qty > curr_qty) {
      // Inserted order got filled
      block->curr_qty -= curr_qty;
      curr_qty = 0;
    } else {
      // Resting qty <= incoming: resting fully filled, remove it
      order_blocks_map.erase(block->key);
      curr_qty -= block->curr_qty;
      unlink(block, list);
      allocator.deallocate(block);
      while (best_buy_idx > 0 && order_buffer[best_buy_idx].head == nullptr) {
        best_buy_idx--;
      }
    }
  }
  if (curr_qty > 0) {
    if (order.Price() == MARKETPRICE)
      return 0; // unfilled rest is cancelled, never rests. TODO: send Cancelled
    auto new_block = allocator.allocate();
    if (new_block == nullptr)
      return 0; // book full: unfilled rest is cancelled. TODO: send Cancelled
    if (order.Price() < best_sell_idx)
      best_sell_idx = order.Price();
    new_block->id = ++counter;
    new_block->key = order_key(account, order.UserRefNum());
    new_block->curr_qty = curr_qty;
    std::memcpy(new_block->raw.data(), order.data(), EnterRequestView::WIRE_SIZE);
    append_list(new_block, &order_buffer[order.Price()]);
    order_blocks_map[new_block->key] = new_block;
    return new_block->id;
  }
  return 0; // Signals that was inserted OK and already filled
}

// Returns the number of shares cancelled (0 if nothing changed). OUCH 5.0 2.3:
// Quantity is the new intended size of the open order; 0 cancels the rest.
template <size_t QUEUE_SIZE, size_t BUFFER_SIZE, size_t ALLOCATOR_SIZE>
uint32_t Engine<QUEUE_SIZE, BUFFER_SIZE, ALLOCATOR_SIZE>::cancel_order(
    uint32_t account, CancelRequestView cancel) {
  // TODO: send Cancelled with the returned (incremental) quantity
  auto it = order_blocks_map.find(order_key(account, cancel.UserRefNum()));
  if (it == order_blocks_map.end())
    return 0; // unknown or already gone: spec says silently ignore

  OrderBlock *block = it->second;
  uint32_t new_qty = cancel.Quantity();
  if (new_qty >= block->curr_qty)
    return 0; // a cancel can only reduce
  uint32_t cancelled = block->curr_qty - new_qty;
  if (new_qty > 0) {
    block->curr_qty = new_qty; // reduce in place, keeps time priority
    return cancelled;
  }

  // Read price/side before deallocate: the free list reuses the slot's bytes
  EnterRequestView order(block->raw.data());
  uint64_t idx = order.Price();
  bool is_buy = order.Side() == SideEnum::B;

  order_blocks_map.erase(it);
  unlink(block, &order_buffer[idx]);
  allocator.deallocate(block);

  if (order_buffer[idx].head == nullptr) {
    if (is_buy && idx == best_buy_idx) {
      while (best_buy_idx > 0 && order_buffer[best_buy_idx].head == nullptr)
        best_buy_idx--;
    } else if (!is_buy && idx == best_sell_idx) {
      while (best_sell_idx < BUFFER_SIZE &&
             order_buffer[best_sell_idx].head == nullptr)
        best_sell_idx++;
    }
  }
  return cancelled;
}
