#pragma once

#include "messages.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <random>
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
constexpr uint64_t MARKET_PRICE = 0x7FFFFFFF;
constexpr size_t CLIENT_NO = 10;

class FlowGenerator {
  struct SimOrder {
    uint32_t account;
    uint32_t user_ref_num;
    SideEnum side;
    uint64_t price;
    uint32_t remaining_qty;
    uint64_t arrival_sequence;
  };

  std::mt19937 rng{SEED};
  std::uniform_int_distribution<size_t> coin_toss{0, 1};
  std::uniform_int_distribution<size_t> price_getter{0, 4};
  std::uniform_int_distribution<size_t> order_type_getter{0, TOTAL_ORDERS - 1};
  std::uniform_int_distribution<size_t> client{0, CLIENT_NO - 1};
  std::uniform_int_distribution<size_t> qty_getter{0, 4};
  std::uniform_int_distribution<size_t> cross_gen{0, 99};

  std::vector<std::tuple<InboundMessage, size_t>> warm_book{};
  std::vector<std::tuple<InboundMessage, size_t>> bench_orders{};
  std::array<uint32_t, CLIENT_NO> seq_counter{};
  std::vector<SimOrder> open_orders{};
  uint64_t next_arrival_sequence = 0;

  void apply_enter(const SimOrder &incoming) {
    uint32_t remaining_qty = incoming.remaining_qty;

    while (remaining_qty > 0) {
      size_t best_index = open_orders.size();
      for (size_t i = 0; i < open_orders.size(); ++i) {
        const SimOrder &resting = open_orders[i];
        const bool opposite_side = resting.side != incoming.side;
        if (!opposite_side)
          continue;

        const bool crosses =
            incoming.price == MARKET_PRICE ||
            (incoming.side == SideEnum::B ? resting.price <= incoming.price
                                          : resting.price >= incoming.price);
        if (!crosses)
          continue;

        if (best_index == open_orders.size()) {
          best_index = i;
          continue;
        }

        const SimOrder &best = open_orders[best_index];
        const bool better_price = incoming.side == SideEnum::B
                                      ? resting.price < best.price
                                      : resting.price > best.price;
        const bool same_price_older =
            resting.price == best.price &&
            resting.arrival_sequence < best.arrival_sequence;
        if (better_price || same_price_older)
          best_index = i;
      }

      if (best_index == open_orders.size())
        break;

      SimOrder &resting = open_orders[best_index];
      const uint32_t fill_qty = std::min(remaining_qty, resting.remaining_qty);
      remaining_qty -= fill_qty;
      resting.remaining_qty -= fill_qty;
      if (resting.remaining_qty == 0) {
        open_orders[best_index] = open_orders.back();
        open_orders.pop_back();
      }
    }

    // Market orders never rest; the engine cancels any unfilled remainder.
    if (remaining_qty > 0 && incoming.price != MARKET_PRICE) {
      SimOrder resting = incoming;
      resting.remaining_qty = remaining_qty;
      open_orders.push_back(resting);
    }
  }

  std::tuple<InboundMessage, uint32_t>
  create_enter_request(size_t client_idx, SideEnum side, bool warm, bool market,
                       bool crosses) {
    InboundMessage msg;
    msg.account = static_cast<uint32_t>(client_idx);
    const uint32_t qty = static_cast<uint32_t>(QTYS[qty_getter(rng)]);
    uint64_t price = MARKET_PRICE;

    if (!market && !warm && crosses) {
      price = side == SideEnum::B ? SELL_PRICES[price_getter(rng)]
                                  : BUY_PRICES[price_getter(rng)];
    } else if (!market) {
      do {
        price = side == SideEnum::B ? BUY_PRICES[price_getter(rng)]
                                    : SELL_PRICES[price_getter(rng)];
      } while (price == SPREAD);
    }

    const uint32_t user_ref_num = seq_counter[client_idx]++;
    EnterRequestBuilder builder;
    builder.Side(side)
        .UserRefNum(user_ref_num)
        .Symbol("AAPL")
        .Price(price)
        .Quantity(qty);
    std::memcpy(msg.bytes.data(), builder.bytes().data(),
                builder.bytes().size());

    apply_enter({static_cast<uint32_t>(client_idx), user_ref_num, side, price,
                 qty, next_arrival_sequence++});
    return {msg, msg.account};
  }

  std::tuple<InboundMessage, size_t> create_cancel_order() {
    std::uniform_int_distribution<size_t> choose_order(0,
                                                       open_orders.size() - 1);
    const size_t index = choose_order(rng);
    const SimOrder order = open_orders[index];
    open_orders[index] = open_orders.back();
    open_orders.pop_back();

    InboundMessage msg;
    msg.account = order.account;
    CancelRequestBuilder builder;
    builder.UserRefNum(order.user_ref_num).Quantity(0);
    std::memcpy(msg.bytes.data(), builder.bytes().data(),
                builder.bytes().size());
    return {msg, order.account};
  }

public:
  FlowGenerator(size_t initial_warm_orders, size_t bench_orders_no) {
    warm_book.reserve(initial_warm_orders);
    bench_orders.reserve(bench_orders_no);

    // Warm the book with non-crossing limit orders.
    for (size_t i = 0; i < initial_warm_orders; ++i) {
      const size_t side = coin_toss(rng);
      const size_t client_idx = client(rng);
      warm_book.push_back(create_enter_request(
          client_idx, side == 0 ? SideEnum::B : SideEnum::S, true, false,
          false));
    }

    for (size_t i = 0; i < bench_orders_no;) {
      const size_t order_type = order_type_getter(rng);

      if (order_type < ENTER_ORDER_PERC) {
        const size_t side = coin_toss(rng);
        const size_t client_idx = client(rng);
        const bool crosses = cross_gen(rng) < CROSS_PERCENTAGE;
        bench_orders.push_back(create_enter_request(
            client_idx, side == 0 ? SideEnum::B : SideEnum::S, false, false,
            crosses));
      } else if (order_type >= ENTER_ORDER_PERC + CANCEL_ORDER_PERC) {
        const size_t side = coin_toss(rng);
        const size_t client_idx = client(rng);
        bench_orders.push_back(create_enter_request(
            client_idx, side == 0 ? SideEnum::B : SideEnum::S, true, true,
            false));
      } else {
        // Retry the operation draw if no resting order can be canceled.
        if (open_orders.empty())
          continue;
        bench_orders.push_back(create_cancel_order());
      }
      ++i;
    }
  }

  const std::vector<std::tuple<InboundMessage, size_t>> &get_warm_book() const {
    return warm_book;
  }
  const std::vector<std::tuple<InboundMessage, size_t>> &
  get_bench_orders() const {
    return bench_orders;
  }
};
