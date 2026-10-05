#include "network_device.hpp"

#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <cstdio>
#include <set>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace {

constexpr int kMessagesPerPort = 3;
constexpr std::size_t kDemoMessageSize = 16;

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
    const ssize_t sent = send(fd, data + sent_total, size - sent_total,
                              MSG_NOSIGNAL);
    if (sent < 0 && errno == EINTR)
      continue;
    if (sent <= 0)
      return false;
    sent_total += static_cast<std::size_t>(sent);
  }
  return true;
}

std::array<char, kDemoMessageSize> make_message(int port, int number) {
  std::array<char, kDemoMessageSize> message{};
  std::snprintf(message.data(), message.size(), "port=%d msg=%d", port,
                number);
  return message;
}

// This is the shared inbound-backend contract. An io_uring backend can be
// checked by calling this same helper with its backend type.
template <typename Backend> void receives_messages_from_every_port() {
  Backend device;
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
    const InboundMessage message = device.receive_msg();
    EXPECT_GE(message.client_fd, 0);
    received.emplace(reinterpret_cast<const char *>(message.bytes.data()),
                     kDemoMessageSize);
  }

  EXPECT_EQ(received, expected);
}

template <typename Backend> void receives_repeated_messages_on_one_connection() {
  Backend device;
  ClientSockets client;
  const int fd = connect_to_port(START_PORT_RANGE);
  ASSERT_GE(fd, 0);
  client.fds.push_back(fd);

  for (int number = 1; number <= kMessagesPerPort; ++number) {
    const auto payload = make_message(START_PORT_RANGE, number);
    ASSERT_TRUE(send_all(fd, payload.data(), payload.size()));

    const InboundMessage received = device.receive_msg();
    EXPECT_EQ(std::string(reinterpret_cast<const char *>(received.bytes.data()),
                          payload.size()),
              std::string(payload.data(), payload.size()));
  }
}

template <typename Backend> void ignores_a_client_that_disconnects_without_data() {
  Backend device;
  const int empty_client = connect_to_port(START_PORT_RANGE);
  ASSERT_GE(empty_client, 0);
  close(empty_client);

  ClientSockets client;
  const int fd = connect_to_port(START_PORT_RANGE);
  ASSERT_GE(fd, 0);
  client.fds.push_back(fd);
  const auto payload = make_message(START_PORT_RANGE, 1);
  ASSERT_TRUE(send_all(fd, payload.data(), payload.size()));

  const InboundMessage received = device.receive_msg();
  EXPECT_EQ(std::string(reinterpret_cast<const char *>(received.bytes.data()),
                        payload.size()),
            std::string(payload.data(), payload.size()));
}

TEST(NetworkDeviceEpoll, ReceivesThreeMessagesFromEachListeningPort) {
  receives_messages_from_every_port<NetworkDeviceEpoll<32>>();
}

TEST(NetworkDeviceEpoll, ReceivesRepeatedMessagesOnOneConnection) {
  receives_repeated_messages_on_one_connection<NetworkDeviceEpoll<32>>();
}

TEST(NetworkDeviceEpoll, IgnoresAClientThatDisconnectsWithoutData) {
  ignores_a_client_that_disconnects_without_data<NetworkDeviceEpoll<32>>();
}

} // namespace
