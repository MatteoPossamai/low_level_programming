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
