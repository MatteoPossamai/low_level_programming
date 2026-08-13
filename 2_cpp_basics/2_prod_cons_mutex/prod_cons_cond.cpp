#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

std::condition_variable data_cond;
std::pmr::vector<int> var;
std::mutex mut;

void producer() {
  std::lock_guard<std::mutex> guard(mut);
  var.push_back(10);
  data_cond.notify_one();
}
void consumer() {
  std::unique_lock<std::mutex> lk(mut);
  data_cond.wait(lk, [&] { return var.size() != 0; });
  std::cout << var[0] << std::endl;
}

int main() {
  std::thread producer_t(producer);
  std::thread consumer_t(consumer);
  producer_t.join();
  consumer_t.join();
  return 0;
}
