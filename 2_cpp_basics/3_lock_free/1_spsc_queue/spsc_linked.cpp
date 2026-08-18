#include <atomic>
#include <cassert>
#include <memory>
#include <thread>

#ifndef ITER
#define ITER 1000
#endif

template <typename T> class spsc_queue {

  struct node {
    std::shared_ptr<T> data;
    node *next;
    node() : next(nullptr) {}
  };
  std::atomic<node *> head;
  std::atomic<node *> tail;

public:
  spsc_queue() : head(new node), tail(head.load()) {};
  spsc_queue(const spsc_queue &other) = delete;
  spsc_queue &operator=(const spsc_queue &other) = delete;
  ~spsc_queue() {
    while (node *const old_head = head.load()) {
      head.store(old_head->next);
      delete old_head;
    }
  }

  std::shared_ptr<int> deque() {
    // relaxed: only the consumer ever writes head
    auto h = head.load(std::memory_order_relaxed);
    // acquire: pairs with the producer's release store of tail, so h->data
    // and h->next are fully written before we touch them
    if (h == tail.load(std::memory_order_acquire)) {
      return nullptr;
    }
    auto res = std::move(h->data);
    auto nxt = h->next;
    h->next = nullptr;
    // relaxed: the producer never reads head in this design, so nothing to
    // synchronize with
    head.store(nxt, std::memory_order_relaxed);
    delete h;
    return res;
  }

  void enqueue(T data) {
    std::shared_ptr<T> ptr = std::make_shared<T>(std::move(data));
    // relaxed: only the producer ever writes tail
    node *t = tail.load(std::memory_order_relaxed);
    auto new_node = new node;
    t->data = ptr;
    t->next = new_node;
    // release: publishes t->data and t->next before the consumer can see the
    // new tail
    tail.store(new_node, std::memory_order_release);
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
