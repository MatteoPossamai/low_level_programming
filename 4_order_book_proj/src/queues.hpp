#pragma once
#include <atomic>
#include <cstddef>

template <typename T, size_t S> class mpsc_queue {
  static_assert(S && (S & (S - 1)) == 0, "S must be a power of 2");
  struct cell {
    std::atomic<size_t> sequence;
    T data;
  };

  static constexpr size_t mask_ = S - 1;
  cell buffer_[S];

  alignas(64) std::atomic<size_t> enqueue_ptr_;
  alignas(64) size_t dequeue_ptr_;

public:
  mpsc_queue();
  mpsc_queue(const mpsc_queue &) = delete;
  mpsc_queue &operator=(const mpsc_queue &) = delete;
  mpsc_queue(mpsc_queue &&) = delete;
  mpsc_queue &operator=(mpsc_queue &&) = delete;
  void enqueue(T data);
  void dequeue(T &data);
};

template <typename T, size_t S> class spsc_queue {
  static_assert(S && (S & (S - 1)) == 0, "S must be a power of 2");
  struct cell {
    std::atomic<size_t> sequence;
    T data;
  };

  static constexpr size_t mask_ = S - 1;
  cell buffer_[S];

  alignas(64) size_t enqueue_ptr_;
  alignas(64) size_t dequeue_ptr_;

public:
  spsc_queue();
  spsc_queue(const spsc_queue &) = delete;
  spsc_queue &operator=(const spsc_queue &) = delete;
  spsc_queue(spsc_queue &&) = delete;
  spsc_queue &operator=(spsc_queue &&) = delete;
  void enqueue(T data);
  void dequeue(T &data);
};

#include "queues_impl.hpp"
