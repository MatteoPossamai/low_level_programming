#include <atomic>
#include <cassert>
#include <thread>

#define ITERS 100000

class counter {
  std::atomic<int> counter;

public:
  void increment() { counter++; };
  int read() { return counter.load(); }
};

counter c;

void increment() {
  for (int i = 0; i < ITERS; i++) {
    c.increment();
  }
}

int main() {
  std::thread t1(increment);
  std::thread t2(increment);
  t1.join();
  t2.join();
  assert(ITERS * 2 == c.read());
}
