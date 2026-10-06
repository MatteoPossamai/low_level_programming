#include "messages.hpp"
#include <algorithm>
#include <cstdint>
#include <random>
#include <tuple>
#include <vector>

constexpr size_t SEED = 42;
constexpr size_t ENTER_ORDER_PERC = 50;
constexpr size_t CANCEL_ORDER_PERC = 48;
constexpr size_t MARKET_ORDER_PERC = 2;
constexpr size_t TOTAL_ORDERS =
    ENTER_ORDER_PERC + CANCEL_ORDER_PERC + MARKET_ORDER_PERC;
constexpr size_t CROSS_PERCENTAGE = 5;
constexpr uint64_t BUY_PRICES[5] = {96, 97, 98, 99, 100};
constexpr uint64_t SPREAD = 100;
constexpr uint64_t SELL_PRICES[5] = {100, 101, 102, 103, 104};
constexpr uint64_t QTYS[5] = {5, 20, 100, 150, 200};
constexpr size_t CLIENT_NO = 10;

class FlowGenerator {
  std::mt19937 rng{SEED};
  std::uniform_int_distribution<size_t> coin_toss{0, 1};
  std::uniform_int_distribution<size_t> price_getter{0, 4};
  std::uniform_int_distribution<size_t> order_type_getter{0, TOTAL_ORDERS - 1};
  std::uniform_int_distribution<size_t> client{0, CLIENT_NO - 1};
  std::uniform_int_distribution<size_t> qty_getter{0, 4};
  std::uniform_int_distribution<size_t> cross_gen{0, 99};

  std::vector<InboundMessage> warm_book{};
  std::vector<InboundMessage> bench_orders{};

  std::array<uint32_t, CLIENT_NO> seq_counter{};

  std::vector<std::tuple<uint32_t, uint32_t>>
      live_orders; // client_id, user_ref_num

  InboundMessage create_enter_request(size_t client_idx, SideEnum side,
                                      bool warm, bool market, bool crosses) {
    InboundMessage msg;
    msg.account = static_cast<uint32_t>(client_idx);
    uint64_t qty = qty_getter(rng);
    uint64_t price = 100;

    if (market) {
      price = 0x7FFFFFFF;
    } else if (!warm && crosses) {
      if (side == SideEnum::B) {
        price = SELL_PRICES[price_getter(rng)];
      } else {
        price = BUY_PRICES[price_getter(rng)];
      }
    } else {
      do {
        switch (side) {
        case SideEnum::B:
          price = BUY_PRICES[price_getter(rng)];
          break;
        default:
          price = SELL_PRICES[price_getter(rng)];
          break;
        }
      } while (price == 100);
    }

    auto numb = seq_counter[client_idx]++;
    EnterRequestBuilder builder;
    builder.Side(side).UserRefNum(numb).Symbol("AAPL").Price(price).Quantity(
        QTYS[qty]);

    live_orders.push_back({client_idx, numb});
    std::memcpy(msg.bytes.data(), builder.bytes().data(),
                builder.bytes().size());
    return msg;
  }

  InboundMessage create_cancel_order() {
    InboundMessage msg;
    std::shuffle(live_orders.begin(), live_orders.end(), rng);
    auto order_to_cancel = live_orders[live_orders.size() - 1];
    live_orders.pop_back();
    CancelRequestBuilder builder;
    builder.UserRefNum(get<1>(order_to_cancel)).Quantity(0);
    msg.account = get<0>(order_to_cancel);
    std::memcpy(msg.bytes.data(), builder.bytes().data(),
                builder.bytes().size());
    return msg;
  }

public:
  FlowGenerator(size_t initial_warm_orders, size_t bench_orders_no) {
    // Warm the book stream of messagess -- NOT PART OF BENCHMARKS
    for (size_t i = 0; i < initial_warm_orders; i++) {
      size_t type = coin_toss(rng);
      size_t client_idx = client(rng);
      InboundMessage msg = create_enter_request(
          client_idx, type == 0 ? SideEnum::B : SideEnum::S, true, false,
          false);
      warm_book.push_back(std::move(msg));
    }

    for (size_t i = 0; i < bench_orders_no;) {
      size_t order_type = order_type_getter(rng);

      // Create a order
      if (order_type < ENTER_ORDER_PERC) {
        size_t type = coin_toss(rng);
        size_t client_idx = client(rng);
        bool crosses = cross_gen(rng) > CROSS_PERCENTAGE - 1 ? false : true;
        InboundMessage msg = create_enter_request(
            client_idx, type == 0 ? SideEnum::B : SideEnum::S, false, false,
            crosses);
        bench_orders.push_back(msg);
      } else if (order_type >= (ENTER_ORDER_PERC + CANCEL_ORDER_PERC)) {
        size_t type = coin_toss(rng);
        size_t client_idx = client(rng);
        InboundMessage msg = create_enter_request(
            client_idx, type == 0 ? SideEnum::B : SideEnum::S, true, true,
            false);
        bench_orders.push_back(msg);

      } else {
        if (live_orders.empty())
          continue;
        InboundMessage msg = create_cancel_order();
        bench_orders.push_back(msg);
      }
      ++i;
    }
  }

  const std::vector<InboundMessage> get_warm_book() { return warm_book; }
  const std::vector<InboundMessage> get_bench_orders() { return bench_orders; }
};
