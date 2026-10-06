#include "engine.hpp"
#include "engine_flow_generator.hpp"
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

constexpr size_t INITIAL_WARMING_ORDER_NO = 30;
constexpr size_t BENCH_ORDER_NO = 100000;

constexpr size_t MAX_QUEUE_SIZE = 2048 * 1024;
constexpr size_t MAX_ORDER_BUFFER_SIZE = 2048 * 1024;
constexpr size_t ALLOCATOR_SIZE = 2048 * 1024;

struct TscStamp {
  uint64_t ticks;
  uint32_t cpu_tag;
};

inline TscStamp read_tsc() {
  uint32_t lo, hi, aux;
  asm volatile("rdtscp\n\tlfence" : "=a"(lo), "=d"(hi), "=c"(aux) : : "memory");
  return {(uint64_t{hi} << 32) | lo, aux};
}

int main() {
  std::vector<uint64_t> times;
  FlowGenerator fg(INITIAL_WARMING_ORDER_NO, BENCH_ORDER_NO);
  auto incoming_queue =
      std::make_unique<spsc_queue<InboundMessage, MAX_QUEUE_SIZE>>();
  auto outgoing_queue =
      std::make_unique<spsc_queue<OutboundMessage, MAX_QUEUE_SIZE>>();

  auto engine = std::make_unique<
      Engine<MAX_QUEUE_SIZE, MAX_ORDER_BUFFER_SIZE, ALLOCATOR_SIZE>>(
      *incoming_queue, *outgoing_queue);

  for (auto msg : fg.get_warm_book()) {
    engine->process(msg);
  }

  for (auto msg : fg.get_bench_orders()) {
    const auto start = read_tsc();
    engine->process(msg);
    const auto end = read_tsc();
    if (start.cpu_tag == end.cpu_tag)
      times.push_back(end.ticks - start.ticks);
  }

  std::sort(times.begin(), times.end());

  auto percentile = [&](double p) {
    const size_t rank = static_cast<size_t>(std::ceil(p * times.size()));
    return times[rank - 1];
  };

  auto p50 = percentile(0.50);
  auto p99 = percentile(0.99);
  auto p999 = percentile(0.999);
  auto worst = times.back();

  std::cout << "P50: " << p50 << std::endl
            << "P99: " << p99 << std::endl
            << "P99.9: " << p999 << std::endl
            << "Worst: " << worst << std::endl;
}
