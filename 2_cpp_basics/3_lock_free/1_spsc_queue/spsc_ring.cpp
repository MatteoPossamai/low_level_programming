#include <atomic>
#include <cassert>
#include <memory>
#include <thread>
#ifndef ITER
#define ITER 1000
#endif

template <typename T, int S> class spsc_queue {
  T *data;
  std::atomic<size_t> head, tail;

public:
  spsc_queue() : data(new T[S]), head(0), tail(0) {};
  spsc_queue(const spsc_queue &other) = delete;
  spsc_queue &operator=(const spsc_queue &other) = delete;
  ~spsc_queue() { delete[] data; }

  bool enqueue(T value) {
    // relaxed: only the producer ever writes tail, so it always sees its own
    // latest value
    size_t t = tail.load(std::memory_order_relaxed);
    // acquire: pairs with the consumer's release store of head, so the slot
    // we are about to overwrite has really been read out already
    if ((t + 1) % S == head.load(std::memory_order_acquire)) {
      return false;
    }
    data[t] = std::move(value);
    // release: publishes the write to data[t] before the new tail is visible
    tail.store((t + 1) % S, std::memory_order_release);
    return true;
  }

  std::shared_ptr<T> deque() {
    // relaxed: only the consumer ever writes head
    size_t curr = head.load(std::memory_order_relaxed);
    // acquire: pairs with the producer's release store of tail, so data[curr]
    // is fully written before we read it
    if (curr == tail.load(std::memory_order_acquire)) {
      return nullptr;
    }
    auto res = std::make_shared<T>(std::move(data[curr]));
    // release: the producer must not reuse the slot until we are done with it
    head.store((curr + 1) % S, std::memory_order_release);
    return res;
  }
};

auto queue = spsc_queue<int, 1000>();

void producer() {
  for (int i = 0; i < ITER; i++) {
    while (!queue.enqueue(i)) {
    }
  }
}

void consumer() {
  int counter = 0;
  while (counter < ITER) {
    std::shared_ptr<int> data = queue.deque();

    if (data != nullptr) {
      assert(*data.get() == counter);
      counter++;
    }
  }
}

int main() {
  std::thread t1(producer);
  std::thread t2(consumer);
  t1.join();
  t2.join();
  return 0;
}
