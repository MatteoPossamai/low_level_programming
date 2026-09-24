#pragma once
#include "allocator.hpp"
#include "messages.hpp"
#include "queues.hpp"
#include <cstdint>
#include <unordered_map>

template <size_t QUEUE_SIZE, size_t BUFFER_SIZE, size_t ALLOCATOR_SIZE>
class Engine {
  Allocator<EnterRequestView, ALLOCATOR_SIZE> allocator{};

  mpsc_queue<OUCHMessageIn, QUEUE_SIZE> &incoming_queue;
  spsc_queue<OUCHMessageOut, QUEUE_SIZE> &outgoing_queue;

  struct OrderBlock {
    EnterRequestView Order;
    uint32_t qty_open;
    OrderBlock *prev;
    OrderBlock *next;
  };

  struct OrderList {
    OrderBlock *head = nullptr;
    OrderBlock *tail = nullptr;
  };

  uint64_t counter = 0;
  std::unordered_map<uint64_t, OrderBlock *> order_blocks_map;

  std::array<OrderList *, BUFFER_SIZE> order_buffer{};
  uint64_t best_buy_idx = 0;
  uint64_t best_sell_idx = BUFFER_SIZE;
  void unlink(OrderBlock *block);

public:
  Engine(mpsc_queue<OUCHMessageIn, QUEUE_SIZE> &incoming_queue_in,
         spsc_queue<OUCHMessageOut, QUEUE_SIZE> &outgoing_queue_in)
      : incoming_queue(incoming_queue_in), outgoing_queue(outgoing_queue_in) {
    for (int i = 0; i < BUFFER_SIZE; i++) {
      // Initialize all lists as empty
      order_buffer[i] = new OrderList;
    }
  }
  ~Engine() {
    for (int i = 0; i < BUFFER_SIZE; i++) {
      auto current = order_buffer[i]->head;
      while (current != nullptr) {
        auto nxt = current->nxt;
        allocator.deallocate(&current);
        current = nxt;
      }
      delete order_buffer[i];
    }
  }

  Engine(const Engine &) = delete;
  Engine(Engine &&) = delete;
  Engine &operator=(const Engine &) = delete;

  uint64_t insert_order(EnterRequestView order);
  uint64_t cancel_order(EnterRequestView order);
};

template <size_t QUEUE_SIZE, size_t BUFFER_SIZE, size_t ALLOCATOR_SIZE>
void Engine<QUEUE_SIZE, BUFFER_SIZE, ALLOCATOR_SIZE>::unlink(
    OrderBlock *block) {
  if (block->prev)
    block->prev->next = block->next;
  if (block->next)
    block->next->prev = block->prev;

  block->prev = nullptr;
  block->next = nullptr;
}

template <size_t QUEUE_SIZE, size_t BUFFER_SIZE, size_t ALLOCATOR_SIZE>
uint64_t Engine<QUEUE_SIZE, BUFFER_SIZE, ALLOCATOR_SIZE>::insert_order(
    EnterRequestView order) {}

template <size_t QUEUE_SIZE, size_t BUFFER_SIZE, size_t ALLOCATOR_SIZE>
uint64_t Engine<QUEUE_SIZE, BUFFER_SIZE, ALLOCATOR_SIZE>::cancel_order(
    EnterRequestView order) {}
