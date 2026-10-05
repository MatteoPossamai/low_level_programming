#include "engine.hpp"
#include "network_device.hpp"

#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <span>
#include <sys/socket.h>
#include <unistd.h>

namespace {

constexpr std::size_t kQueueSize = 2048;
constexpr std::size_t kPriceBufferSize = 2048;
constexpr std::size_t kAllocatorSize = 2048;

using InQueue = spsc_queue<InboundMessage, kQueueSize>;
using OutQueue = spsc_queue<OutboundMessage, kQueueSize>;
using TestEngine = Engine<kQueueSize, kPriceBufferSize, kAllocatorSize>;
using TestNetworkDevice = NetworkDeviceEpoll<kQueueSize>;

class ClientSocket {
  int fd_ = -1;

public:
  explicit ClientSocket(int fd) : fd_(fd) {}
  ~ClientSocket() {
    if (fd_ >= 0)
      close(fd_);
  }
  ClientSocket(const ClientSocket &) = delete;
  ClientSocket &operator=(const ClientSocket &) = delete;
  int get() const { return fd_; }
};

bool send_all(int fd, std::span<const std::byte> bytes) {
  std::size_t sent_total = 0;
  const char *data = reinterpret_cast<const char *>(bytes.data());
  while (sent_total < bytes.size()) {
    const ssize_t sent = send(fd, data + sent_total, bytes.size() - sent_total,
                              MSG_NOSIGNAL);
    if (sent < 0 && errno == EINTR)
      continue;
    if (sent <= 0)
      return false;
    sent_total += static_cast<std::size_t>(sent);
  }
  return true;
}

bool receive_all(int fd, std::span<std::byte> bytes) {
  std::size_t received_total = 0;
  char *data = reinterpret_cast<char *>(bytes.data());
  while (received_total < bytes.size()) {
    const ssize_t received =
        recv(fd, data + received_total, bytes.size() - received_total, 0);
    if (received < 0 && errno == EINTR)
      continue;
    if (received <= 0)
      return false;
    received_total += static_cast<std::size_t>(received);
  }
  return true;
}

} // namespace

TEST(CompleteFlow, AcceptThenCancelReachesTheClient) {
  InQueue incoming;
  OutQueue outgoing;
  TestEngine engine(incoming, outgoing);
  TestNetworkDevice network(incoming);

  const int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_GE(client_fd, 0);
  ClientSocket client(client_fd);

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = htons(START_PORT_RANGE);
  ASSERT_EQ(connect(client.get(), reinterpret_cast<sockaddr *>(&address),
                    sizeof(address)),
            0);

  constexpr uint32_t user_ref_num = 1001;
  const auto enter = EnterRequestBuilder()
                         .UserRefNum(user_ref_num)
                         .Side(SideEnum::B)
                         .Quantity(10)
                         .Symbol("AAPL")
                         .Price(100)
                         .TimeInForce(TimeInForceEnum::Day)
                         .Display(DisplayEnum::Y)
                         .Capacity(CapacityEnum::P)
                         .InterMarketSweepElig(InterMarketSweepEligEnum::N)
                         .CrossType(CrossTypeEnum::N)
                         .ClOrdID("flow-test");

  ASSERT_TRUE(send_all(client.get(), enter.bytes()));
  network.receive_msg();
  InboundMessage inbound{};
  incoming.dequeue(inbound);
  engine.process(inbound);

  OutboundMessage outbound{};
  outgoing.dequeue(outbound);
  ASSERT_EQ(outbound.account, inbound.account);
  ASSERT_EQ(network.send_msg(outbound), 0u);

  std::array<std::byte, AcceptResponseView::WIRE_SIZE> accepted_bytes{};
  ASSERT_TRUE(receive_all(client.get(), accepted_bytes));
  ASSERT_EQ(std::to_integer<char>(accepted_bytes[0]), AcceptResponseView::TYPE);
  const AcceptResponseView accepted(accepted_bytes.data());
  EXPECT_EQ(accepted.UserRefNum(), user_ref_num);
  EXPECT_EQ(accepted.Side(), SideEnum::B);
  EXPECT_EQ(accepted.Quantity(), 10u);
  EXPECT_EQ(accepted.Symbol().substr(0, 4), "AAPL");
  EXPECT_EQ(accepted.Price(), 100u);

  const auto cancel = CancelRequestBuilder()
                          .UserRefNum(user_ref_num)
                          .Quantity(0);
  ASSERT_TRUE(send_all(client.get(), cancel.bytes()));
  network.receive_msg();
  incoming.dequeue(inbound);
  engine.process(inbound);

  outgoing.dequeue(outbound);
  ASSERT_EQ(outbound.account, inbound.account);
  ASSERT_EQ(network.send_msg(outbound), 0u);

  std::array<std::byte, CancelledResponseView::WIRE_SIZE> canceled_bytes{};
  ASSERT_TRUE(receive_all(client.get(), canceled_bytes));
  ASSERT_EQ(std::to_integer<char>(canceled_bytes[0]),
            CancelledResponseView::TYPE);
  const CancelledResponseView canceled(canceled_bytes.data());
  EXPECT_EQ(canceled.UserRefNum(), user_ref_num);
  EXPECT_EQ(canceled.Quantity(), 10u);
  EXPECT_EQ(canceled.Reason(), 'U');
}

TEST(CompleteFlow, MatchingAaplOrdersSendsFillToBothClients) {
  InQueue incoming;
  OutQueue outgoing;
  TestEngine engine(incoming, outgoing);
  TestNetworkDevice network(incoming);

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = htons(START_PORT_RANGE);

  const int buyer_fd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_GE(buyer_fd, 0);
  ClientSocket buyer(buyer_fd);
  ASSERT_EQ(connect(buyer.get(), reinterpret_cast<sockaddr *>(&address),
                    sizeof(address)),
            0);

  constexpr uint32_t buy_ref = 2001;
  const auto buy = EnterRequestBuilder()
                       .UserRefNum(buy_ref)
                       .Side(SideEnum::B)
                       .Quantity(7)
                       .Symbol("AAPL")
                       .Price(100)
                       .TimeInForce(TimeInForceEnum::Day)
                       .Display(DisplayEnum::Y)
                       .Capacity(CapacityEnum::P)
                       .InterMarketSweepElig(InterMarketSweepEligEnum::N)
                       .CrossType(CrossTypeEnum::N)
                       .ClOrdID("buyer");

  ASSERT_TRUE(send_all(buyer.get(), buy.bytes()));
  network.receive_msg();
  InboundMessage buyer_inbound{};
  incoming.dequeue(buyer_inbound);
  engine.process(buyer_inbound);

  OutboundMessage outbound{};
  outgoing.dequeue(outbound);
  ASSERT_EQ(outbound.account, buyer_inbound.account);
  ASSERT_EQ(network.send_msg(outbound), 0u);

  std::array<std::byte, AcceptResponseView::WIRE_SIZE> buyer_accepted_bytes{};
  ASSERT_TRUE(receive_all(buyer.get(), buyer_accepted_bytes));
  ASSERT_EQ(std::to_integer<char>(buyer_accepted_bytes[0]),
            AcceptResponseView::TYPE);
  const AcceptResponseView buyer_accepted(buyer_accepted_bytes.data());
  EXPECT_EQ(buyer_accepted.UserRefNum(), buy_ref);

  const int seller_fd = socket(AF_INET, SOCK_STREAM, 0);
  ASSERT_GE(seller_fd, 0);
  ClientSocket seller(seller_fd);
  ASSERT_EQ(connect(seller.get(), reinterpret_cast<sockaddr *>(&address),
                    sizeof(address)),
            0);

  constexpr uint32_t sell_ref = 3001;
  const auto sell = EnterRequestBuilder()
                        .UserRefNum(sell_ref)
                        .Side(SideEnum::S)
                        .Quantity(7)
                        .Symbol("AAPL")
                        .Price(100)
                        .TimeInForce(TimeInForceEnum::Day)
                        .Display(DisplayEnum::Y)
                        .Capacity(CapacityEnum::P)
                        .InterMarketSweepElig(InterMarketSweepEligEnum::N)
                        .CrossType(CrossTypeEnum::N)
                        .ClOrdID("seller");

  ASSERT_TRUE(send_all(seller.get(), sell.bytes()));
  network.receive_msg();
  InboundMessage seller_inbound{};
  incoming.dequeue(seller_inbound);
  engine.process(seller_inbound);

  outgoing.dequeue(outbound);
  ASSERT_EQ(outbound.account, seller_inbound.account);
  ASSERT_EQ(network.send_msg(outbound), 0u);

  std::array<std::byte, AcceptResponseView::WIRE_SIZE> seller_accepted_bytes{};
  ASSERT_TRUE(receive_all(seller.get(), seller_accepted_bytes));
  ASSERT_EQ(std::to_integer<char>(seller_accepted_bytes[0]),
            AcceptResponseView::TYPE);
  const AcceptResponseView seller_accepted(seller_accepted_bytes.data());
  EXPECT_EQ(seller_accepted.UserRefNum(), sell_ref);

  outgoing.dequeue(outbound);
  ASSERT_EQ(outbound.account, seller_inbound.account);
  ASSERT_EQ(network.send_msg(outbound), 0u);
  outgoing.dequeue(outbound);
  ASSERT_EQ(outbound.account, buyer_inbound.account);
  ASSERT_EQ(network.send_msg(outbound), 0u);

  std::array<std::byte, ExecutedResponseView::WIRE_SIZE> seller_fill_bytes{};
  std::array<std::byte, ExecutedResponseView::WIRE_SIZE> buyer_fill_bytes{};
  ASSERT_TRUE(receive_all(seller.get(), seller_fill_bytes));
  ASSERT_TRUE(receive_all(buyer.get(), buyer_fill_bytes));
  ASSERT_EQ(std::to_integer<char>(seller_fill_bytes[0]),
            ExecutedResponseView::TYPE);
  ASSERT_EQ(std::to_integer<char>(buyer_fill_bytes[0]),
            ExecutedResponseView::TYPE);

  const ExecutedResponseView seller_fill(seller_fill_bytes.data());
  const ExecutedResponseView buyer_fill(buyer_fill_bytes.data());
  EXPECT_EQ(seller_fill.UserRefNum(), sell_ref);
  EXPECT_EQ(buyer_fill.UserRefNum(), buy_ref);
  EXPECT_EQ(seller_fill.Quantity(), 7u);
  EXPECT_EQ(buyer_fill.Quantity(), 7u);
  EXPECT_EQ(seller_fill.Price(), 100u);
  EXPECT_EQ(buyer_fill.Price(), 100u);
  EXPECT_EQ(seller_fill.LiquidityFlag(), 'R');
  EXPECT_EQ(buyer_fill.LiquidityFlag(), 'A');
  EXPECT_EQ(seller_fill.MatchNumber(), buyer_fill.MatchNumber());
}
