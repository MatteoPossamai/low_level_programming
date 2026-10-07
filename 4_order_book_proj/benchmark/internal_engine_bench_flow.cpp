#include "engine.hpp"
#include "engine_flow_generator.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

constexpr size_t INITIAL_WARMING_ORDER_NO = 3000;
constexpr size_t BENCH_ORDER_NO = 100000;

constexpr size_t MAX_QUEUE_SIZE = 2048 * 1024;
constexpr size_t MAX_ORDER_BUFFER_SIZE = 2048 * 1024;
constexpr size_t ALLOCATOR_SIZE = 2048 * 1024;

struct TscStamp {
  uint64_t ticks;
  uint32_t cpu_tag;
};

enum class RequestKind : size_t {
  passive_limit,
  aggressive_limit,
  market,
  cancel,
};

constexpr std::array<std::string_view, 4> REQUEST_KIND_NAMES = {
    "limit_passive", "limit_aggressive", "market", "cancel"};

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

int main() {
  const double tsc_hz = estimate_tsc_hz();
  std::vector<LatencySample> samples;
  samples.reserve(BENCH_ORDER_NO);
  size_t migrated_samples = 0;
  FlowGenerator fg(INITIAL_WARMING_ORDER_NO, BENCH_ORDER_NO);
  auto incoming_queue =
      std::make_unique<spsc_queue<InboundMessage, MAX_QUEUE_SIZE>>();
  auto outgoing_queue =
      std::make_unique<spsc_queue<OutboundMessage, MAX_QUEUE_SIZE>>();

  auto engine = std::make_unique<
      Engine<MAX_QUEUE_SIZE, MAX_ORDER_BUFFER_SIZE, ALLOCATOR_SIZE>>(
      *incoming_queue, *outgoing_queue);

  for (const auto &msg : fg.get_warm_book()) {
    engine->process(get<0>(msg));
  }

  size_t request_index = 0;
  for (const auto &msg : fg.get_bench_orders()) {
    const RequestKind kind = classify(get<0>(msg));
    const auto start = read_tsc();
    engine->process(get<0>(msg));
    const auto end = read_tsc();
    if (start.cpu_tag == end.cpu_tag)
      samples.push_back({request_index, end.ticks - start.ticks, 0, kind});
    else
      ++migrated_samples;
    ++request_index;
  }

  std::array<std::vector<LatencySample>, REQUEST_KIND_NAMES.size()> by_kind;
  for (auto &sample : samples) {
    sample.ns = static_cast<uint64_t>(std::llround(
        static_cast<double>(sample.ticks) * 1'000'000'000.0 / tsc_hz));
    by_kind[static_cast<size_t>(sample.kind)].push_back(sample);
  }

  std::cout << "TSC frequency: " << std::fixed << std::setprecision(2)
            << (tsc_hz / 1'000'000.0) << " MHz"
            << " | discarded migrated samples: " << migrated_samples << "\n\n";
  std::cout << std::left << std::setw(19) << "Request" << std::right
            << std::setw(9) << "Samples" << std::setw(10) << "P50 ns"
            << std::setw(10) << "P99 ns" << std::setw(10) << "P99.9 ns"
            << std::setw(10) << "Max ns" << std::setw(13) << "P99.9 idx"
            << std::setw(13) << "Max idx" << '\n'
            << std::string(94, '-') << '\n';
  print_stats("all", samples);
  for (size_t i = 0; i < by_kind.size(); ++i)
    print_stats(REQUEST_KIND_NAMES[i], std::move(by_kind[i]));
}
