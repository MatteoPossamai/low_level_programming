#include "network_device.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <set>
#include <span>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

constexpr int kMessagesPerPort = 3;
constexpr std::size_t kDemoMessageSize = 24;
constexpr std::size_t kQueueSize = 32;
using TestInboundQueue = spsc_queue<InboundMessage, kQueueSize>;

int connect_to_port(int port) {
  const int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0)
    return -1;

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = htons(port);
  if (connect(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) <
      0) {
    close(fd);
    return -1;
  }
  return fd;
}

template <typename Backend> class DeviceReader {
  Backend &device;
  std::atomic<bool> stop{false};
  std::thread worker;

public:
  explicit DeviceReader(Backend &device_in)
      : device(device_in), worker([this] {
          while (!stop.load(std::memory_order_acquire))
            device.receive_msg();
        }) {}

  ~DeviceReader() {
    stop.store(true, std::memory_order_release);
    const int wake_fd = connect_to_port(START_PORT_RANGE);
    if (wake_fd >= 0)
      close(wake_fd);
    worker.join();
  }
};

struct ClientSockets {
  std::vector<int> fds;
  ~ClientSockets() {
    for (const int fd : fds)
      close(fd);
  }
};

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
    const ssize_t received =
        recv(fd, data + received_total, size - received_total, 0);
    if (received < 0 && errno == EINTR)
      continue;
    if (received <= 0)
      return false;
    received_total += static_cast<std::size_t>(received);
  }
  return true;
}

std::array<char, kDemoMessageSize> make_message(int port, int number) {
  std::array<char, kDemoMessageSize> message{};
  std::snprintf(message.data(), message.size(), "port=%d msg=%d", port, number);
  return message;
}

template <typename Backend>
InboundMessage receive_one(Backend &device, TestInboundQueue &queue) {
  (void)device;
  InboundMessage message{};
  queue.dequeue(message);
  return message;
}

template <typename Backend>
void sends_response_to_the_client(std::span<const std::byte> response,
                                  std::size_t expected_size) {
  TestInboundQueue queue;
  Backend device(queue);
  DeviceReader reader(device);
  ClientSockets client;
  const int client_fd = connect_to_port(START_PORT_RANGE);
  ASSERT_GE(client_fd, 0);
  client.fds.push_back(client_fd);

  const auto request_bytes = make_message(START_PORT_RANGE, 1);
  ASSERT_TRUE(send_all(client_fd, request_bytes.data(), request_bytes.size()));
  const InboundMessage request = receive_one(device, queue);
  ASSERT_GT(request.account, 0u);

  OutboundMessage outbound{};
  outbound.account = request.account;
  std::copy(response.begin(), response.end(), outbound.bytes.begin());
  ASSERT_EQ(device.send_msg(outbound), 0u);

  std::vector<char> actual(expected_size);
  ASSERT_TRUE(receive_all(client_fd, actual.data(), actual.size()));
  EXPECT_EQ(std::memcmp(actual.data(), response.data(), expected_size), 0);

  char extra_byte;
  const ssize_t extra = recv(client_fd, &extra_byte, 1, MSG_DONTWAIT);
  EXPECT_EQ(extra, -1) << "response contained bytes past its wire length";
  EXPECT_TRUE(errno == EAGAIN || errno == EWOULDBLOCK);
}

// This is the shared inbound-backend contract. An io_uring backend can be
// checked by calling this same helper with its backend type.
template <typename Backend> void receives_messages_from_every_port() {
  TestInboundQueue queue;
  Backend device(queue);
  DeviceReader reader(device);
  ClientSockets clients;
  std::set<std::string> expected;

  for (int port = START_PORT_RANGE; port <= END_PORT_RANGE; ++port) {
    for (int number = 1; number <= kMessagesPerPort; ++number) {
      const auto payload = make_message(port, number);
      expected.emplace(payload.data(), payload.size());

      const int fd = connect_to_port(port);
      ASSERT_GE(fd, 0) << "Could not connect to test port " << port;
      clients.fds.push_back(fd);

      ASSERT_TRUE(send_all(fd, payload.data(), payload.size()));
    }
  }

  // Keep the connections open while receiving, so EOF events do not add
  // noise to this test of successful inbound payload delivery.
  std::set<std::string> received;
  for (std::size_t i = 0; i < expected.size(); ++i) {
    const InboundMessage message = receive_one(device, queue);
    EXPECT_GE(message.account, 0);
    received.emplace(reinterpret_cast<const char *>(message.bytes.data()),
                     kDemoMessageSize);
  }

  EXPECT_EQ(received, expected);
}

template <typename Backend>
void receives_repeated_messages_on_one_connection() {
  TestInboundQueue queue;
  Backend device(queue);
  DeviceReader reader(device);
  ClientSockets client;
  const int fd = connect_to_port(START_PORT_RANGE);
  ASSERT_GE(fd, 0);
  client.fds.push_back(fd);

  for (int number = 1; number <= kMessagesPerPort; ++number) {
    const auto payload = make_message(START_PORT_RANGE, number);
    ASSERT_TRUE(send_all(fd, payload.data(), payload.size()));

    const InboundMessage received = receive_one(device, queue);
    EXPECT_EQ(std::string(reinterpret_cast<const char *>(received.bytes.data()),
                          payload.size()),
              std::string(payload.data(), payload.size()));
  }
}

template <typename Backend>
void ignores_a_client_that_disconnects_without_data() {
  TestInboundQueue queue;
  Backend device(queue);
  DeviceReader reader(device);
  const int empty_client = connect_to_port(START_PORT_RANGE);
  ASSERT_GE(empty_client, 0);
  close(empty_client);

  ClientSockets client;
  const int fd = connect_to_port(START_PORT_RANGE);
  ASSERT_GE(fd, 0);
  client.fds.push_back(fd);
  const auto payload = make_message(START_PORT_RANGE, 1);
  ASSERT_TRUE(send_all(fd, payload.data(), payload.size()));

  const InboundMessage received = receive_one(device, queue);
  EXPECT_EQ(std::string(reinterpret_cast<const char *>(received.bytes.data()),
                        payload.size()),
            std::string(payload.data(), payload.size()));
}

TEST(NetworkDeviceEpoll, ReceivesThreeMessagesFromEachListeningPort) {
  receives_messages_from_every_port<NetworkDeviceEpoll<kQueueSize>>();
}

TEST(NetworkDeviceEpoll, ReceivesRepeatedMessagesOnOneConnection) {
  receives_repeated_messages_on_one_connection<NetworkDeviceEpoll<kQueueSize>>();
}

TEST(NetworkDeviceEpoll, IgnoresAClientThatDisconnectsWithoutData) {
  ignores_a_client_that_disconnects_without_data<NetworkDeviceEpoll<kQueueSize>>();
}

TEST(NetworkDeviceEpoll, SendsAcceptedResponse) {
  const auto response = AcceptResponseBuilder()
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
                            .ClOrdID("test");
  sends_response_to_the_client<NetworkDeviceEpoll<32>>(
      response.bytes(), AcceptResponseView::WIRE_SIZE);
}

TEST(NetworkDeviceEpoll, SendsCanceledResponse) {
  const auto response =
      CancelledResponseBuilder().Timestamp(1).UserRefNum(2).Quantity(10).Reason(
          'U');
  sends_response_to_the_client<NetworkDeviceEpoll<32>>(
      response.bytes(), CancelledResponseView::WIRE_SIZE);
}

TEST(NetworkDeviceEpoll, SendsExecutedResponse) {
  const auto response = ExecutedResponseBuilder()
                            .Timestamp(1)
                            .UserRefNum(2)
                            .Quantity(10)
                            .Price(12345)
                            .LiquidityFlag('A')
                            .MatchNumber(3);
  sends_response_to_the_client<NetworkDeviceEpoll<32>>(
      response.bytes(), ExecutedResponseView::WIRE_SIZE);
}

TEST(NetworkDeviceIOUring, InboundAndAsyncOutboundFlow) {
  TestInboundQueue queue;
  NetworkDeviceIOUring<kQueueSize> device(queue);
  DeviceReader reader(device);
  ClientSockets client;
  std::set<std::string> expected;
  std::set<std::string> received;

  for (int port = START_PORT_RANGE; port <= END_PORT_RANGE; ++port) {
    for (int number = 1; number <= kMessagesPerPort; ++number) {
      const auto payload = make_message(port, number);
      expected.emplace(payload.data(), payload.size());
      const int fd = connect_to_port(port);
      ASSERT_GE(fd, 0);
      client.fds.push_back(fd);
      ASSERT_TRUE(send_all(fd, payload.data(), payload.size()));
    }
  }

  for (std::size_t i = 0; i < expected.size(); ++i) {
    const InboundMessage message = receive_one(device, queue);
    received.emplace(reinterpret_cast<const char *>(message.bytes.data()),
                     kDemoMessageSize);
  }
  EXPECT_EQ(received, expected);

  const int fd = client.fds.front();
  const auto repeated = make_message(START_PORT_RANGE, 99);
  ASSERT_TRUE(send_all(fd, repeated.data(), repeated.size()));
  InboundMessage request = receive_one(device, queue);
  EXPECT_EQ(std::string(reinterpret_cast<const char *>(request.bytes.data()),
                        repeated.size()),
            std::string(repeated.data(), repeated.size()));

  const auto response = AcceptResponseBuilder()
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
                            .ClOrdID("test");

  OutboundMessage outbound{};
  outbound.account = request.account;
  std::copy(response.bytes().begin(), response.bytes().end(),
            outbound.bytes.begin());
  ASSERT_EQ(device.send_msg(outbound), 0u);
  std::array<char, AcceptResponseView::WIRE_SIZE> actual{};
  ASSERT_TRUE(receive_all(fd, actual.data(), actual.size()));
  EXPECT_EQ(std::memcmp(actual.data(), response.bytes().data(), actual.size()),
            0);

  const int empty_client = connect_to_port(START_PORT_RANGE);
  ASSERT_GE(empty_client, 0);
  close(empty_client);
  const int replacement = connect_to_port(START_PORT_RANGE);
  ASSERT_GE(replacement, 0);
  client.fds.push_back(replacement);
  const auto after_disconnect = make_message(START_PORT_RANGE, 100);
  ASSERT_TRUE(send_all(replacement, after_disconnect.data(),
                       after_disconnect.size()));
  const InboundMessage replacement_message = receive_one(device, queue);
  EXPECT_EQ(
      std::string(reinterpret_cast<const char *>(replacement_message.bytes.data()),
                  after_disconnect.size()),
      std::string(after_disconnect.data(), after_disconnect.size()));
}

} // namespace
