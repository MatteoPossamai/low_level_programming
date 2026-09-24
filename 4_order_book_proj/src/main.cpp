#include "engine.hpp"
#include "messages.hpp"
#include "queues.hpp"

#define MAX_QUEUE_SIZE 2048
#define MAX_ORDER_BUFFER_SIZE 2048
#define ALLOCATOR_SIZE 2048

int main() {
  mpsc_queue<OUCHMessageIn, MAX_QUEUE_SIZE> incoming_queue;
  spsc_queue<OUCHMessageOut, MAX_QUEUE_SIZE> outgoing_queue;
  auto engine = Engine<MAX_QUEUE_SIZE, MAX_ORDER_BUFFER_SIZE, ALLOCATOR_SIZE>(
      incoming_queue, outgoing_queue);
  return 0;
}
