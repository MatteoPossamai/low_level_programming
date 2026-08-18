#include <cassert>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#define ITER 1000

template <typename T> class spsc_queue {
  std::queue<T> queue;
  std::mutex mutex;

public:
  spsc_queue() = default;
  spsc_queue(const spsc_queue &other) = delete;
  spsc_queue &operator=(const spsc_queue &other) = delete;
  ~spsc_queue() = default;

  void enqueue(T value) {
    std::lock_guard<std::mutex> lk(mutex);
    queue.push(value);
  }

  std::shared_ptr<T> deque() {
    std::lock_guard<std::mutex> lk(mutex);
    if (queue.size() == 0) {
      return nullptr;
    }
    auto res = std::make_shared<T>(queue.front());
    queue.pop();
    return res;
  }
};

auto queue = spsc_queue<int>();

void producer() {
  for (int i = 0; i < ITER; i++) {
    queue.enqueue(i);
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
