#include <atomic>
#include <cassert>
#include <memory>
#include <thread>

#define ITER 1000

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
    auto h = head.load();
    if (h == tail.load()) {
      return nullptr;
    }
    auto res = std::move(h->data);
    auto nxt = h->next;
    h->next = nullptr;
    head.store(nxt);
    delete h;
    return res;
  }

  void enqueue(T data) {
    std::shared_ptr<T> ptr = std::make_shared<T>(std::move(data));
    node *t = tail.load();
    auto new_node = new node;
    t->data = ptr;
    t->next = new_node;
    tail.store(t);
    tail.store(new_node);
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
