#include "engine.hpp"
#include "engine_flow_generator.hpp"
#include "messages.hpp"
#include "network_device.hpp"
#include "queues.hpp"
#include <thread>

constexpr size_t INITIAL_WARMING_ORDER_NO = 3000;
constexpr size_t BENCH_ORDER_NO = 100000;

constexpr size_t MAX_QUEUE_SIZE = 2048 * 1024;
constexpr size_t MAX_ORDER_BUFFER_SIZE = 2048 * 1024;
constexpr size_t ALLOCATOR_SIZE = 2048 * 1024;

// Time stuff
// ---
struct TscStamp {
  uint64_t ticks;
  uint32_t cpu_tag;
};

struct LatencySample {
  size_t index;
  uint64_t ticks;
  uint64_t ns;
};

inline TscStamp read_tsc() {
  uint32_t lo, hi, aux;
  asm volatile("rdtscp\n\tlfence" : "=a"(lo), "=d"(hi), "=c"(aux) : : "memory");
  return {(uint64_t{hi} << 32) | lo, aux};
}

double estimate_tsc_hz() {
  using Clock = std::chrono::steady_clock;
  const auto wall_start = Clock::now();
  const auto tsc_start = read_tsc();
  std::this_thread::sleep_for(std::chrono::milliseconds(250));
  const auto tsc_end = read_tsc();
  const auto wall_end = Clock::now();

  const double elapsed_seconds =
      std::chrono::duration<double>(wall_end - wall_start).count();
  return static_cast<double>(tsc_end.ticks - tsc_start.ticks) / elapsed_seconds;
}

// ---

spsc_queue<InboundMessage, MAX_QUEUE_SIZE> incoming_queue;
spsc_queue<OutboundMessage, MAX_QUEUE_SIZE> outgoing_queue;
auto engine = Engine<MAX_QUEUE_SIZE, MAX_ORDER_BUFFER_SIZE, ALLOCATOR_SIZE>(
    incoming_queue, outgoing_queue);
auto network_device = NetworkDeviceIOUring<MAX_QUEUE_SIZE>(incoming_queue);

void engine_thread() { engine.run(); }

void network_reader_thread() {

  while (1) {
    network_device.receive_msg();
  }
}

void network_writer_thread() {
  while (1) {
    OutboundMessage msg;
    outgoing_queue.dequeue(msg);
    network_device.send_msg(msg);
  }
}

int main() {
  std::thread network_writer_th(network_writer_thread);
  std::thread engine_th(engine_thread);
  std::thread network_reader_th(network_reader_thread);

  FlowGenerator fg(INITIAL_WARMING_ORDER_NO, BENCH_ORDER_NO);

  network_writer_th.join();
  engine_th.join();
  network_reader_th.join();

  return 0;
}
