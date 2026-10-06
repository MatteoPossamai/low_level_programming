#include "network_device.hpp"

#include <benchmark/benchmark.h>

#include <array>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <memory>
#include <thread>
#include <unistd.h>

namespace {

constexpr std::size_t kQueueSize = 1024;
constexpr std::size_t kRequestSize = 16;
using InQueue = spsc_queue<InboundMessage, kQueueSize>;
std::atomic<int> next_port{20000 + (getpid() % 7000) * 4};

std::array<char, kRequestSize> make_request(uint64_t sequence) {
  std::array<char, kRequestSize> bytes{};
  std::memcpy(bytes.data(), &sequence, sizeof(sequence));
  return bytes;
}

bool send_all(int fd, const char *data, std::size_t size) {
  std::size_t sent_total = 0;
  while (sent_total < size) {
    const ssize_t sent =
        send(fd, data + sent_total, size - sent_total, MSG_NOSIGNAL);
    if (sent < 0 && errno == EINTR)
      continue;
    if (sent <= 0)
      return false;
    sent_total += static_cast<std::size_t>(sent);
  }
  return true;
}

bool receive_all(int fd, char *data, std::size_t size) {
  std::size_t received_total = 0;
  while (received_total < size) {
    const ssize_t received = recv(fd, data + received_total,
                                  size - received_total, 0);
    if (received < 0 && errno == EINTR)
      continue;
    if (received <= 0)
      return false;
    received_total += static_cast<std::size_t>(received);
  }
  return true;
}

auto accepted_response() {
  return AcceptResponseBuilder()
      .Timestamp(1)
      .UserRefNum(2)
      .Side(SideEnum::B)
      .Quantity(10)
      .Symbol("TEST")
      .Price(12345)
      .TimeInForce(TimeInForceEnum::Day)
      .Display(DisplayEnum::Y)
      .OrderReferenceNumber(3)
      .Capacity('P')
      .InterMarketSweepElig(InterMarketSweepEligEnum::N)
      .CrossType(CrossTypeEnum::N)
      .OrderState(OrderStateEnum::L)
      .ClOrdID("bench");
}

template <typename Backend> void BM_NetworkRoundTrip(benchmark::State &state) {
  auto queue = std::make_unique<InQueue>();
  const int port = next_port.fetch_add(4, std::memory_order_relaxed);
  auto device = std::make_unique<Backend>(*queue, port);

  const int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client_fd < 0) {
    device.reset();
    queue.reset();
    state.SkipWithError("socket() failed");
    return;
  }

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = htons(port);

  std::atomic<bool> stop_reader{false};
  std::thread reader;

  auto stop = [&] {
    stop_reader.store(true, std::memory_order_release);
    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
    reader.join();
  };

  if (connect(client_fd, reinterpret_cast<sockaddr *>(&address),
              sizeof(address)) < 0) {
    close(client_fd);
    device.reset();
    queue.reset();
    state.SkipWithError("connect() failed");
    return;
  }

  reader = std::thread([&] {
    while (!stop_reader.load(std::memory_order_acquire))
      device->receive_msg();
  });

  const auto response = accepted_response();
  auto round_trip = [&](uint64_t sequence) {
    const auto request = make_request(sequence);
    if (!send_all(client_fd, request.data(), request.size()))
      return false;

    InboundMessage inbound{};
    queue->dequeue(inbound);

    OutboundMessage outbound{};
    outbound.account = inbound.account;
    std::copy(response.bytes().begin(), response.bytes().end(),
              outbound.bytes.begin());
    if (device->send_msg(outbound) != 0)
      return false;

    std::array<char, AcceptResponseView::WIRE_SIZE> received{};
    return receive_all(client_fd, received.data(), received.size()) &&
           std::memcmp(received.data(), response.bytes().data(),
                       received.size()) == 0;
  };

  // Establish the connection and warm both directions outside the timed loop.
  if (!round_trip(0)) {
    stop();
    device.reset();
    queue.reset();
    state.SkipWithError("warm-up round trip failed");
    return;
  }

  uint64_t sequence = 1;
  for (auto _ : state) {
    if (!round_trip(sequence++)) {
      state.SkipWithError("round trip failed");
      break;
    }
  }

  stop();
  device.reset();
  queue.reset();
  state.SetItemsProcessed(state.iterations());
}

} // namespace

BENCHMARK(BM_NetworkRoundTrip<NetworkDeviceEpoll<kQueueSize>>)
    ->Name("Network/Epoll/LoopbackRoundTrip")
    ->Unit(benchmark::kNanosecond)
    ->UseRealTime();
BENCHMARK(BM_NetworkRoundTrip<NetworkDeviceIOUring<kQueueSize>>)
    ->Name("Network/IOUring/LoopbackRoundTrip")
    ->Unit(benchmark::kNanosecond)
    ->UseRealTime();

BENCHMARK_MAIN();
