#include <atomic>
#include <condition_variable>
#include <iostream>
#include <thread>
#include <vector>

std::atomic_flag flag;
std::condition_variable data_cond;
std::vector<int> var;

void producer() {
  var.push_back(10);
  flag.test_and_set();
  flag.notify_all();
}
void consumer() {
  flag.wait(false, std::memory_order_seq_cst);
  std::cout << var[0] << std::endl;
}

int main() {
  std::thread producer_t(producer);
  std::thread consumer_t(consumer);
  producer_t.join();
  consumer_t.join();

  return 0;
}
