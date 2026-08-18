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
    size_t t = tail.load();
    if ((t + 1) % S == head.load()) {
      return false;
    }
    data[t] = std::move(value);
    tail.store((t + 1) % S);
    return true;
  }

  std::shared_ptr<T> deque() {
    size_t curr = head.load();
    if (curr == tail.load()) {
      return nullptr;
    }
    auto res = std::make_shared<T>(std::move(data[curr]));
    head.store((curr + 1) % S);
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
