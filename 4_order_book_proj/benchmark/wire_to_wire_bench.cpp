#include "engine.hpp"
#include "engine_flow_generator.hpp"
#include "messages.hpp"
#include "network_device.hpp"
#include "queues.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <memory>
#include <poll.h>
#include <stdexcept>
#include <string_view>
#include <sys/socket.h>
#include <thread>
#include <vector>

constexpr size_t INITIAL_WARMING_ORDER_NO = 3000;
constexpr size_t BENCH_ORDER_NO = 100000;

constexpr size_t MAX_QUEUE_SIZE = 32768;
constexpr size_t MAX_ORDER_BUFFER_SIZE = 2048 * 1024;
constexpr size_t ALLOCATOR_SIZE = 2048 * 1024;

enum class RequestKind : size_t {
  passive_limit,
  aggressive_limit,
  market,
  cancel,
};

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
  RequestKind kind;
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

constexpr std::array<std::string_view, 4> REQUEST_KIND_NAMES = {
    "limit_passive", "limit_aggressive", "market", "cancel"};

RequestKind classify(const InboundMessage &msg) {
  const char type = std::to_integer<char>(msg.bytes[0]);
  if (type == CancelRequestView::TYPE)
    return RequestKind::cancel;

  EnterRequestView enter(msg.bytes.data());
  if (enter.Price() == 0x7FFFFFFF)
    return RequestKind::market;

  const bool aggressive =
      (enter.Side() == SideEnum::B && enter.Price() >= SPREAD) ||
      (enter.Side() == SideEnum::S && enter.Price() <= SPREAD);
  return aggressive ? RequestKind::aggressive_limit
                    : RequestKind::passive_limit;
}

size_t percentile_index(size_t count, double p) {
  const size_t rank =
      std::max<size_t>(1, static_cast<size_t>(std::ceil(p * count)));
  return rank - 1;
}

void print_stats(std::string_view name, std::vector<LatencySample> samples) {
  if (samples.empty()) {
    std::cout << std::left << std::setw(19) << name << " no samples\n";
    return;
  }
  std::sort(samples.begin(), samples.end(),
            [](const auto &a, const auto &b) { return a.ns < b.ns; });
  const auto &p999 = samples[percentile_index(samples.size(), 0.999)];
  const auto &worst = samples.back();
  std::cout << std::left << std::setw(19) << name << std::right << std::setw(9)
            << samples.size() << std::setw(10)
            << samples[percentile_index(samples.size(), 0.50)].ns
            << std::setw(10)
            << samples[percentile_index(samples.size(), 0.99)].ns
            << std::setw(10) << p999.ns << std::setw(10) << worst.ns
            << std::setw(13) << p999.index << std::setw(13) << worst.index
            << '\n';
}

class ResponseReader {
  int socket_fd;
  std::vector<std::byte> buffered;

  static size_t response_size(char type) {
    switch (type) {
    case AcceptResponseView::TYPE:
      return AcceptResponseView::WIRE_SIZE;
    case CancelledResponseView::TYPE:
      return CancelledResponseView::WIRE_SIZE;
    case CancelRejectResponseView::TYPE:
      return CancelRejectResponseView::WIRE_SIZE;
    case ExecutedResponseView::TYPE:
      return ExecutedResponseView::WIRE_SIZE;
    default:
      throw std::runtime_error("Unknown OUCH response type");
    }
  }

  bool pop_response(char &type, uint32_t &user_ref_num) {
    if (buffered.empty())
      return false;
    type = std::to_integer<char>(buffered.front());
    const size_t size = response_size(type);
    if (buffered.size() < size)
      return false;
    user_ref_num = detail::load_be32(buffered.data() + 9);
    buffered.erase(buffered.begin(), buffered.begin() + size);
    return true;
  }

  bool receive(bool nonblocking) {
    std::array<std::byte, 1024> bytes{};
    const ssize_t count = recv(socket_fd, bytes.data(), bytes.size(),
                               nonblocking ? MSG_DONTWAIT : 0);
    if (count < 0) {
      if (nonblocking && (errno == EAGAIN || errno == EWOULDBLOCK))
        return false;
      perror("Receive");
      std::exit(1);
    }
    if (count == 0) {
      std::cerr << "Client connection closed while waiting for response\n";
      std::exit(1);
    }
    buffered.insert(buffered.end(), bytes.begin(), bytes.begin() + count);
    return true;
  }

public:
  explicit ResponseReader(int fd) : socket_fd(fd) {}

  void wait_for_ack(char expected_type, char alternative_type,
                    uint32_t expected_user_ref_num) {
    char type;
    uint32_t user_ref_num;
    while (true) {
      while (pop_response(type, user_ref_num)) {
        if ((type == expected_type || type == alternative_type) &&
            user_ref_num == expected_user_ref_num)
          return;
      }

      pollfd response_fd{socket_fd, POLLIN, 0};
      const int ready = poll(&response_fd, 1, 5000);
      if (ready == 0) {
        std::cerr << "Timed out waiting for response to UserRefNum "
                  << expected_user_ref_num << '\n';
        std::exit(1);
      }
      if (ready < 0) {
        perror("poll");
        std::exit(1);
      }
      receive(false);
    }
  }

  void drain_available() {
    char type;
    uint32_t user_ref_num;
    while (pop_response(type, user_ref_num)) {
    }
    while (receive(true)) {
      while (pop_response(type, user_ref_num)) {
      }
    }
  }
};

struct ExpectedAck {
  char type;
  char alternative_type;
  uint32_t user_ref_num;
};

ExpectedAck expected_ack(const InboundMessage &message) {
  if (std::to_integer<char>(message.bytes[0]) == CancelRequestView::TYPE) {
    CancelRequestView cancel(message.bytes.data());
    return {CancelledResponseView::TYPE, CancelRejectResponseView::TYPE,
            cancel.UserRefNum()};
  }
  EnterRequestView enter(message.bytes.data());
  return {AcceptResponseView::TYPE, '\0', enter.UserRefNum()};
}

void send_and_wait_for_ack(int socket_fd, ResponseReader &reader,
                           const InboundMessage &message) {
  const auto expected = expected_ack(message);
  const size_t message_size =
      std::to_integer<char>(message.bytes[0]) == CancelRequestView::TYPE
          ? CancelRequestView::WIRE_SIZE
          : EnterRequestView::WIRE_SIZE;
  const ssize_t sent = send(socket_fd, message.bytes.data(), message_size, 0);
  if (sent < 0) {
    perror("Send");
    std::exit(1);
  }
  if (static_cast<size_t>(sent) != message_size) {
    std::cerr << "Short request send\n";
    std::exit(1);
  }
  reader.wait_for_ack(expected.type, expected.alternative_type,
                      expected.user_ref_num);
}

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
  network_writer_th.detach();
  std::thread engine_th(engine_thread);
  engine_th.detach();
  std::thread network_reader_th(network_reader_thread);
  network_reader_th.detach();

  std::vector<int> sockets = {};
  std::vector<std::unique_ptr<ResponseReader>> response_readers;
  for (int i = START_PORT_RANGE; i <= END_PORT_RANGE; i++) {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(i);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    auto res = connect(clientSocket, (struct sockaddr *)&serverAddress,
                       sizeof(serverAddress));
    sockets.push_back(clientSocket);
    response_readers.push_back(std::make_unique<ResponseReader>(clientSocket));
    if (res != 0) {
      perror("Connect");
      exit(1);
    }
  }

  FlowGenerator fg(INITIAL_WARMING_ORDER_NO, BENCH_ORDER_NO);
  std::vector<LatencySample> samples;

  for (auto order : fg.get_warm_book()) {
    auto client = get<1>(order);
    const auto &message = get<0>(order);
    send_and_wait_for_ack(sockets[client], *response_readers[client], message);
    response_readers[client]->drain_available();
  }

  size_t counter = 0;
  const double tsc_hz = estimate_tsc_hz();
  for (auto order : fg.get_bench_orders()) {
    auto client = get<1>(order);
    const auto &message = get<0>(order);
    response_readers[client]->drain_available();

    const auto start = read_tsc();
    send_and_wait_for_ack(sockets[client], *response_readers[client], message);
    const auto end = read_tsc();
    if (start.cpu_tag == end.cpu_tag) {
      const RequestKind kind = classify(get<0>(order));
      samples.push_back({++counter, end.ticks - start.ticks, 0, kind});
      samples[samples.size() - 1].ns = static_cast<uint64_t>(
          std::llround(static_cast<double>(samples[samples.size() - 1].ticks) *
                       1'000'000'000.0 / tsc_hz));
    }
    response_readers[client]->drain_available();
  }

  for (auto clientSocket : sockets) {
    close(clientSocket);
  }

  std::array<std::vector<LatencySample>, REQUEST_KIND_NAMES.size()> by_kind;
  for (auto &sample : samples) {
    sample.ns = static_cast<uint64_t>(std::llround(
        static_cast<double>(sample.ticks) * 1'000'000'000.0 / tsc_hz));
    by_kind[static_cast<size_t>(sample.kind)].push_back(sample);
  }

  std::cout << "TSC frequency: " << std::fixed << std::setprecision(2)
            << (tsc_hz / 1'000'000.0) << " MHz" << "\n\n";
  std::cout << std::left << std::setw(19) << "Request" << std::right
            << std::setw(9) << "Samples" << std::setw(10) << "P50 ns"
            << std::setw(10) << "P99 ns" << std::setw(10) << "P99.9 ns"
            << std::setw(10) << "Max ns" << std::setw(13) << "P99.9 idx"
            << std::setw(13) << "Max idx" << '\n'
            << std::string(94, '-') << '\n';
  print_stats("all", samples);
  for (size_t i = 0; i < by_kind.size(); ++i)
    print_stats(REQUEST_KIND_NAMES[i], std::move(by_kind[i]));
  exit(0);
  return 0;
}
