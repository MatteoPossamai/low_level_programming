# Order book / matching engine — Q4

Block 1: price-level data structure chosen by measurement (three
implementations, one workload benchmark). Blocks 2-3: OUCH 5.0 codec, pool
allocator, queues, and a price-time matching engine built on the winning
structure, with scenario tests and a baseline benchmark. Engine design in
`DESIGN_DOC.md`.

## Layout

```
4_order_book_proj/
├── data_structure_bench/        Block 1 structures
│   ├── base.hpp                 shared types (BuyOrder, SellOrder, BuyBlock, SellBlock, OrderBook interface)
│   ├── sorted_vector_str.{hpp,cpp}   O(n) shift-based sorted vector
│   ├── list_str.{hpp,cpp}       intrusive doubly-linked list + id→node hash map
│   └── tick_offset_str.{hpp,cpp}     sparse array indexed by price, FIFO list per level
├── src/                         engine
│   ├── messages.hpp, message.cpp     OUCH 5.0 views/builders, decode(), queue payload types
│   ├── queues.hpp, queues_impl.hpp   MPSC (inbound) and SPSC (outbound) sequence-number queues
│   ├── allocator.hpp            mmap-backed fixed-size pool (free list)
│   ├── engine.hpp               matching engine: insert/cancel, outbound messages, process()/run()
│   └── main.cpp                 wiring placeholder
├── tests/
│   ├── allocator_test.cpp       pool allocator contract tests
│   └── engine_test.cpp          input→output scenarios + random property test
├── benchmark/
│   ├── workload_bench.cpp       Block 1 harness, templated on Book type
│   ├── codec_bench.cpp          codec encode/decode + round-trip check
│   └── engine_bench.cpp         engine baseline on realistic traffic
├── CMakeLists.txt
├── DATA_STRUCTURE.md            Block 1 hypotheses + results + interpretation
├── DESIGN_DOC.md                engine components, threading, rules
└── README.md                    this file
```

## Block 1 contract

All implementations expose the same interface (`OrderBook` in `base.hpp`), with
`final` on each derived so the benchmark's static-dispatch calls devirtualise
under `-O2`. Interface exists for signature enforcement; the benchmark never
holds a base pointer.

Duck-typed methods:

```cpp
uint64_t insert_buy_order(BuyOrder);
uint64_t insert_sell_order(SellOrder);
uint64_t cancel_buy_order(uint64_t);
uint64_t cancel_sell_order(uint64_t);
uint64_t market_buy_order(BuyOrder);
uint64_t market_sell_order(SellOrder);
std::pair<BuyOrder, SellOrder> top_of_book();
```

## Conventions

- Price and size are `uint64_t`. Price `0` is a reserved sentinel; inserts at
  price 0 are rejected.
- id `0` is reserved as the "operation failed" return value. First real id is `1`.
- FIFO within a price level. head = oldest = highest priority; tail = newest.
- Best-price direction: buy = higher, sell = lower. On the sorted vector,
  best sits at `back()` so `pop_back()` is O(1).
- Market orders walk-and-consume: fill each opposing resting order (partial or
  full) until size satisfied or opposite side exhausted. Empty-book fallback
  reinserts the market as a marketable limit at the extreme legal price.
- `match()` runs after every limit insert. Uses `min(buy.size, sell.size)`
  per cross, decrements both, unlinks whichever hit zero.
- Book is non-copyable. Destructors free every allocated node.

## Build and run

Requires Google Benchmark and GoogleTest installed system-wide.

```
cmake -S . -B build && cmake --build build
./build/allocator_tests                               # ASan+UBSan
./build/engine_tests                                  # ASan+UBSan
./build/bench_engine                                  # engine baseline
./build/bench_codec
./build/bench_order_book                              # Block 1, everything
./build/bench_order_book --benchmark_filter=TickOff   # one structure
./build/bench_order_book --benchmark_format=json --benchmark_out=results.json
```

Compiler flags: `-O2 -g -Wall -Wextra -Wshadow`, C++20. Test targets add
`-fsanitize=address,undefined -fno-sanitize-recover=undefined`.

## Adding another structure

1. Header declaring `class OrderBook_Xxx final : public OrderBook`, implement
   every method in `.cpp` (or in the header for templated impls).
2. `CMakeLists.txt`: add an `add_library(xxx_book STATIC ...)` and append
   `xxx_book` to `target_link_libraries(bench_order_book ...)`.
3. `benchmark/workload_bench.cpp`: `#include "xxx_str.hpp"` and one
   `BENCHMARK_TEMPLATE(BM_Workload, OrderBook_Xxx)->Args(...)` line.

Same op script hits every impl → results are directly comparable.

## Block 1 workload

From `DATA_STRUCTURE.md`: insert 45%, cancel 43%, market 2%, top-of-book 10%.
Op script generated once per `(seed, size)` via deterministic RNG; replayed
against a warmed book each Google Benchmark iteration. Prices `[1, 1000]`,
sizes `[1, 100]`. Cancel picks uniformly from a live-id tracker maintained
across the replay.

Args are `{book_depth, ops_per_iteration}`. Depth = orders pre-loaded before
timing starts.

## Current state

**Block 1** — MVP complete. Winner by measurement at real-book scales:
**tick-offset (sparse array)**. Numbers in `DATA_STRUCTURE.md`.

**Engine** (`src/engine.hpp`) — tick-offset book, intrusive FIFO per level,
blocks from the pool allocator, orders keyed by `account << 32 | UserRefNum`.
Price-time matching for limit and market orders, cancel as reduce-to-new-size.
Emits OUCH Accepted / Executed (one per side, shared match number) / Canceled
with the destination account. All tests pass under ASan+UBSan.

**Baseline** (`bench_engine`, enter 50% / cancel 48% / market 2%, 100k
messages per iteration, -O2, threads unpinned, CPU scaling on):

| Resting depth | Time / 100k msgs | Throughput |
|---|---|---|
| 1,000 | 8.10 ms | 12.35 M msg/s |
| 10,000 | 8.18 ms | 12.22 M msg/s |
| 50,000 | 8.69 ms | 11.51 M msg/s |

## Known limits (accepted for the POC)

- Price is used raw as the level index: inbound must reject price 0 and
  price >= `BUFFER_SIZE`.
- `0x7FFFFFFF` is treated as market on both sides.
- Sides `T`/`E` (short sell) throw.
- Cancel quantity read as the new open size; spec wording ("executed in
  total") is ambiguous.
- Codec has no variable-length optional appendage support.
- Outbound `enqueue` blocks when full: a stalled fan-out stalls the engine
  (Block 5 policy).

## Open items

- **Profile the baseline** — throughput is flat with depth, so per-message
  fixed costs dominate. Candidates: `unordered_map`, `clock_gettime`, message
  encoding. Write the hypothesis before running `perf`.
- **`std::unordered_map` → flat index** for cancel lookup.
- **Price-level bitset** for best-idx walks after a level empties.
- **Inbound validation stage** (price range, side, quantity) before the queue.
- **Codec fuzzing.**
- **Vary the op mix** via `state.range()`.
