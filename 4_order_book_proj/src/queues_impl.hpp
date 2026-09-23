#pragma once
#include "queues.hpp"
#include <immintrin.h>
#include <thread>
#include <utility>

// Constructors
template <typename T, size_t S> mpsc_queue<T, S>::mpsc_queue() {
  enqueue_ptr_.store(0, std::memory_order_relaxed);
  dequeue_ptr_ = 0;

  for (size_t i = 0; i < S; i++) {
    buffer_[i].sequence.store(i, std::memory_order_relaxed);
  }
}

template <typename T, size_t S> spsc_queue<T, S>::spsc_queue() {
  enqueue_ptr_ = 0;
  dequeue_ptr_ = 0;
  for (size_t i = 0; i < S; i++) {
    buffer_[i].sequence.store(i, std::memory_order_relaxed);
  }
}

// Enqueue Logic
template <typename T, size_t S> void mpsc_queue<T, S>::enqueue(T data) {
  cell *curr_cell;
  size_t pos = enqueue_ptr_.load(std::memory_order_relaxed);

  for (;;) {
    curr_cell = &buffer_[pos & mask_];
    size_t cell_pos = curr_cell->sequence.load(std::memory_order_acquire);
    intptr_t diff = static_cast<intptr_t>(cell_pos - pos);

    if (diff == 0) {
      if (enqueue_ptr_.compare_exchange_weak(pos, pos + 1,
                                             std::memory_order_relaxed)) {
        break;
      }
    } else if (diff > 0) {
      pos = enqueue_ptr_.load(std::memory_order_relaxed);
    } else {
      std::this_thread::yield();
    }
  }

  curr_cell->data = std::move(data);
  curr_cell->sequence.store(pos + 1, std::memory_order_release);
}

template <typename T, size_t S> void spsc_queue<T, S>::enqueue(T data) {
  size_t pos = enqueue_ptr_;
  cell &c = buffer_[pos & mask_];
  while (c.sequence.load(std::memory_order_acquire) != pos) {
    _mm_pause();
  }
  c.data = std::move(data);
  c.sequence.store(pos + 1, std::memory_order_release);
  enqueue_ptr_ = pos + 1;
}

// Dequeue Logic
template <typename T, size_t S> void mpsc_queue<T, S>::dequeue(T &data) {
  size_t pos = dequeue_ptr_;
  cell &c = buffer_[pos & mask_];

  while (c.sequence.load(std::memory_order_acquire) != pos + 1) {
    _mm_pause();
  }

  data = std::move(c.data);
  c.sequence.store(pos + S, std::memory_order_release);
  dequeue_ptr_ = pos + 1;
}

template <typename T, size_t S> void spsc_queue<T, S>::dequeue(T &data) {
  size_t pos = dequeue_ptr_;
  cell &c = buffer_[pos & mask_];

  while (c.sequence.load(std::memory_order_acquire) != pos + 1) {
    _mm_pause();
  }

  data = std::move(c.data);
  c.sequence.store(pos + S, std::memory_order_release);
  dequeue_ptr_ = pos + 1;
}
