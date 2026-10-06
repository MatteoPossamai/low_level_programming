#include "messages.hpp"

#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <span>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace {

class Client {
  int fd_;
  const char *label_;

public:
  Client(int port, const char *label) : fd_(socket(AF_INET, SOCK_STREAM, 0)),
                                       label_(label) {
    if (fd_ < 0)
      throw std::runtime_error("socket failed");

    timeval timeout{5, 0};
    setsockopt(fd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port);
    if (connect(fd_, reinterpret_cast<sockaddr *>(&address), sizeof(address)) <
        0) {
      close(fd_);
      throw std::runtime_error("connect failed; is src/main.cpp running?");
    }
  }

  ~Client() { close(fd_); }
  Client(const Client &) = delete;
  Client &operator=(const Client &) = delete;

  int fd() const { return fd_; }
  const char *label() const { return label_; }

  bool send_all(std::span<const std::byte> bytes) const {
    std::size_t total = 0;
    const char *data = reinterpret_cast<const char *>(bytes.data());
    while (total < bytes.size()) {
      const ssize_t sent = send(fd_, data + total, bytes.size() - total,
                                MSG_NOSIGNAL);
      if (sent < 0 && errno == EINTR)
        continue;
      if (sent <= 0)
        return false;
      total += static_cast<std::size_t>(sent);
    }
    return true;
  }

  template <std::size_t SIZE> std::array<std::byte, SIZE> receive() const {
    std::array<std::byte, SIZE> bytes{};
    std::size_t total = 0;
    char *data = reinterpret_cast<char *>(bytes.data());
    while (total < bytes.size()) {
      const ssize_t received = recv(fd_, data + total, bytes.size() - total, 0);
      if (received < 0 && errno == EINTR)
        continue;
      if (received <= 0)
        throw std::runtime_error(std::string(label_) +
                                 " timed out waiting for a response");
      total += static_cast<std::size_t>(received);
    }
    return bytes;
  }
};

void require(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}

auto make_enter(uint32_t user_ref, SideEnum side, uint32_t quantity,
                const char *client_order_id) {
  return EnterRequestBuilder()
      .UserRefNum(user_ref)
      .Side(side)
      .Quantity(quantity)
      .Symbol("AAPL")
      .Price(100)
      .TimeInForce(TimeInForceEnum::Day)
      .Display(DisplayEnum::Y)
      .Capacity(CapacityEnum::P)
      .InterMarketSweepElig(InterMarketSweepEligEnum::N)
      .CrossType(CrossTypeEnum::N)
      .ClOrdID(client_order_id);
}

void send_order(Client &client, const EnterRequestBuilder &order) {
  require(client.send_all(order.bytes()),
          std::string(client.label()) + " failed to send order");
}

void expect_accepted(Client &client, uint32_t user_ref, SideEnum side,
                     uint32_t quantity) {
  const auto bytes = client.receive<AcceptResponseView::WIRE_SIZE>();
  require(std::to_integer<char>(bytes[0]) == AcceptResponseView::TYPE,
          std::string(client.label()) + " expected Accepted response");
  const AcceptResponseView response(bytes.data());
  require(response.UserRefNum() == user_ref, "Accepted UserRefNum mismatch");
  require(response.Side() == side, "Accepted side mismatch");
  require(response.Quantity() == quantity, "Accepted quantity mismatch");
  require(response.Price() == 100, "Accepted price mismatch");
  require(response.Symbol().substr(0, 4) == "AAPL",
          "Accepted symbol mismatch");
  std::printf("%s <- Accepted %s %u AAPL @ 100 (ref %u)\n", client.label(),
              side == SideEnum::B ? "BUY" : "SELL", quantity, user_ref);
}

uint64_t expect_fill(Client &client, uint32_t user_ref, uint32_t quantity,
                     char liquidity) {
  const auto bytes = client.receive<ExecutedResponseView::WIRE_SIZE>();
  require(std::to_integer<char>(bytes[0]) == ExecutedResponseView::TYPE,
          std::string(client.label()) + " expected Executed response");
  const ExecutedResponseView response(bytes.data());
  require(response.UserRefNum() == user_ref, "Executed UserRefNum mismatch");
  require(response.Quantity() == quantity, "Executed quantity mismatch");
  require(response.Price() == 100, "Executed price mismatch");
  require(response.LiquidityFlag() == liquidity,
          "Executed liquidity flag mismatch");
  require(response.MatchNumber() != 0, "match number must be nonzero");
  std::printf("%s <- Executed %u AAPL @ 100 (%c, match %llu)\n",
              client.label(), quantity, liquidity,
              static_cast<unsigned long long>(response.MatchNumber()));
  return response.MatchNumber();
}

} // namespace

int main() {
  try {
    std::puts("Connect this client to a running main on ports 49152 and 49153.");
    std::puts("AAPL order flow: buy 10, sell 6, then sell 4\n");

    Client buyer(49152, "client 1 / port 49152");
    Client seller(49153, "client 2 / port 49153");

    constexpr uint32_t buy_ref = 1001;
    send_order(buyer, make_enter(buy_ref, SideEnum::B, 10, "buy-1001"));
    expect_accepted(buyer, buy_ref, SideEnum::B, 10);

    constexpr uint32_t first_sell_ref = 2001;
    send_order(seller,
               make_enter(first_sell_ref, SideEnum::S, 6, "sell-2001"));
    expect_accepted(seller, first_sell_ref, SideEnum::S, 6);
    const uint64_t first_match_seller =
        expect_fill(seller, first_sell_ref, 6, 'R');
    const uint64_t first_match_buyer = expect_fill(buyer, buy_ref, 6, 'A');
    require(first_match_seller == first_match_buyer,
            "clients received different match numbers");

    constexpr uint32_t second_sell_ref = 2002;
    send_order(seller,
               make_enter(second_sell_ref, SideEnum::S, 4, "sell-2002"));
    expect_accepted(seller, second_sell_ref, SideEnum::S, 4);
    const uint64_t second_match_seller =
        expect_fill(seller, second_sell_ref, 4, 'R');
    const uint64_t second_match_buyer = expect_fill(buyer, buy_ref, 4, 'A');
    require(second_match_seller == second_match_buyer,
            "clients received different match numbers");

    std::puts("\nFlow completed: the 10-lot buy was filled by 6 + 4.");
    return EXIT_SUCCESS;
  } catch (const std::exception &error) {
    std::fprintf(stderr, "Flow failed: %s\n", error.what());
    return EXIT_FAILURE;
  }
}
