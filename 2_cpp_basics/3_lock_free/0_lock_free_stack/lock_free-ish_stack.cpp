#include <atomic>
#include <iostream>
#include <memory>
#include <thread>

template <typename T> class lf_stack {
  struct Node {
    std::shared_ptr<T> data;
    std::shared_ptr<Node> prev; // Element before in the stack
  };

  std::atomic<std::shared_ptr<Node>> head;

public:
  lf_stack() : head(nullptr) { std::cout << head.is_lock_free() << std::endl; };

  void push(T val) {
    std::shared_ptr<T> ptr = std::make_shared<T>(val);
    std::shared_ptr<Node> n =
        std::make_shared<Node>(Node{ptr, head.load(std::memory_order_relaxed)});

    while (!head.compare_exchange_weak(n->prev, n, std::memory_order_release,
                                       std::memory_order_relaxed)) {
    }
  }

  std::shared_ptr<T> pop() {
    auto curr_head = head.load();
    while (curr_head &&
           !head.compare_exchange_weak(curr_head, curr_head->prev,
                                       std::memory_order_acquire,
                                       std::memory_order_relaxed)) {
    }
    return curr_head->data;
  }
};

lf_stack<int> stack{};

void th1() {
  stack.push(10);
  stack.pop();
}
void th2() {}

int main() {
  std::thread t1(th1);
  std::thread t2(th2);

  t1.join();
  t2.join();
  return 0;
}
