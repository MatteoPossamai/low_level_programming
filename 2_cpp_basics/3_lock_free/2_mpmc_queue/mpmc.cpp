#include <atomic>
#include <cassert>
#include <thread>
#include <vector>

#ifndef ITER
#define ITER 1000
#endif

template <typename T> class mpmc_queue {
  typedef struct {
    std::atomic<size_t> sequence;
    T data;
  } cell;
  static const size_t cache_line_size = 64;
  typedef char cacheline_pad_t[cache_line_size];

  // Pointers padded to avoid false sharing / memory ping-pong
  cacheline_pad_t pad1_;
  std::atomic<size_t> enqueue_ptr;
  cacheline_pad_t pad2_;
  std::atomic<size_t> dequeue_ptr;
  cacheline_pad_t pad3_;

  cell *buffer_;
  size_t buffer_mask;

public:
  mpmc_queue(size_t size) : buffer_(new cell[size]), buffer_mask(size - 1) {
    assert(size > 2 && (size & buffer_mask) == 0);

    for (size_t i = 0; i != size; i++) {
      buffer_[i].sequence.store(i, std::memory_order_relaxed);
    }
    enqueue_ptr.store(0, std::memory_order_relaxed);
    dequeue_ptr.store(0, std::memory_order_relaxed);
  };
  ~mpmc_queue() { delete[] buffer_; };

  bool enqueue(T data) {
    cell *curr_cell;
    size_t pos = enqueue_ptr.load(std::memory_order_relaxed);

    for (;;) {
      curr_cell = &buffer_[pos & buffer_mask];
      size_t cell_position = curr_cell->sequence.load(
          std::memory_order_acquire); // Get location to avoid race with
                                      // consumers
      int64_t diff = (int64_t)cell_position - (int64_t)pos;
      if (diff == 0) {
        if (enqueue_ptr.compare_exchange_weak(pos, pos + 1,
                                              std::memory_order_relaxed)) {
          break;
        }
      } else if (diff < 0)
        return false;
      else
        pos = enqueue_ptr.load(std::memory_order_relaxed);
    }
    curr_cell->data = data;
    curr_cell->sequence.store(
        pos + 1,
        std::memory_order_release); // Now on data is visible to be consumed
    return true;
  }

  bool dequeue(T &data) {
    cell *curr_cell;
    size_t pos = dequeue_ptr.load(std::memory_order_relaxed);

    for (;;) {
      curr_cell = &buffer_[pos & buffer_mask];
      size_t cell_position = curr_cell->sequence.load(
          std::memory_order_acquire); // Get location to avoid race with
                                      // producers
      int64_t diff = (int64_t)cell_position - (int64_t)(pos + 1);
      if (diff == 0) {
        if (dequeue_ptr.compare_exchange_weak(pos, pos + 1,
                                              std::memory_order_relaxed)) {
          break;
        }
      } else if (diff < 0)
        return false;
      else
        pos = dequeue_ptr.load(std::memory_order_relaxed);
    }

    data = curr_cell->data;
    curr_cell->sequence.store(
        pos + buffer_mask + 1,
        std::memory_order_release); // Now cell is available to enqueue again
    return true;
  }
};

#ifndef PRODUCERS
#define PRODUCERS 5
#endif
#ifndef CONSUMERS
#define CONSUMERS 5
#endif
static_assert(PRODUCERS == CONSUMERS,
              "each consumer pops exactly ITER items, so counts must match");

auto queue = mpmc_queue<int>(1024);
std::atomic<long long> total_sum{0};

void producer() {
  for (int i = 0; i < ITER; i++) {
    while (!queue.enqueue(i)) {
    }
  }
}

void consumer() {
  long long local_sum = 0;
  for (int i = 0; i < ITER; i++) {
    int data;
    while (!queue.dequeue(data)) {
    }
    local_sum += data;
  }
  total_sum.fetch_add(local_sum);
}

int main() {
  std::vector<std::thread> threads;
  threads.reserve(PRODUCERS + CONSUMERS);
  for (int i = 0; i < PRODUCERS; i++)
    threads.emplace_back(producer);
  for (int i = 0; i < CONSUMERS; i++)
    threads.emplace_back(consumer);
  for (auto &t : threads)
    t.join();

  long long expected = (long long)PRODUCERS * ITER * (ITER - 1LL) / 2;
  assert(total_sum.load() == expected);
  return 0;
}
