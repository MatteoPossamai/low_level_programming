# Order book — Q4 Block 1

Price-level data structure chosen by measurement. Three implementations, one
workload benchmark, structural pick informed by the numbers rather than
folklore.

## Layout

```
4_order_book_proj/
├── data_structure/
│   ├── base.hpp                 shared types (BuyOrder, SellOrder, BuyBlock, SellBlock, OrderBook interface)
│   ├── sorted_vector_str.{hpp,cpp}   O(n) shift-based sorted vector
│   ├── list_str.{hpp,cpp}       intrusive doubly-linked list + id→node hash map
│   └── tick_offset_str.{hpp,cpp}     sparse array indexed by price, FIFO list per level
├── benchmark/
│   └── workload_bench.cpp       Google Benchmark harness, templated on Book type
├── CMakeLists.txt
├── DATA_STRUCTURE.md            hypotheses + results + interpretation
└── README.md                    this file
```

## Contract

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

Requires Google Benchmark installed system-wide.

```
cmake -S . -B build && cmake --build build
./build/bench_order_book                              # run everything
./build/bench_order_book --benchmark_filter=TickOff   # one structure
./build/bench_order_book --benchmark_format=json --benchmark_out=results.json
```

Compiler flags: `-O2 -g -Wall -Wextra -Wshadow`, C++20.

## Adding another structure

1. Header declaring `class OrderBook_Xxx final : public OrderBook`, implement
   every method in `.cpp` (or in the header for templated impls).
2. `CMakeLists.txt`: add an `add_library(xxx_book STATIC ...)` and append
   `xxx_book` to `target_link_libraries(bench_order_book ...)`.
3. `benchmark/workload_bench.cpp`: `#include "xxx_str.hpp"` and one
   `BENCHMARK_TEMPLATE(BM_Workload, OrderBook_Xxx)->Args(...)` line.

Same op script hits every impl → results are directly comparable.

## Workload

From `DATA_STRUCTURE.md`: insert 45%, cancel 43%, market 2%, top-of-book 10%.
Op script generated once per `(seed, size)` via deterministic RNG; replayed
against a warmed book each Google Benchmark iteration. Prices `[1, 1000]`,
sizes `[1, 100]`. Cancel picks uniformly from a live-id tracker maintained
across the replay.

Args are `{book_depth, ops_per_iteration}`. Depth = orders pre-loaded before
timing starts.

## Current state (Block 1)

MVP complete. All three structures pass the workload without crashes and with
matched semantics (walk-and-consume market, partial fills in match).

Winner by measurement at real-book scales: **tick-offset (sparse array)**.
Full numbers and interpretation in `DATA_STRUCTURE.md`. Tick-offset is roughly
flat with depth; sorted-vector and list both decay linearly.

## Open items

- **Pool allocator** — highest-leverage optimisation. All three structures use
  `new`/`delete` per node. A slab pool per node type kills the malloc tax and
  clusters nodes in one region for cache locality. Rerun benchmark to isolate
  allocator cost from structure cost.
- **Price-level bitset for best-idx maintenance** in tick-offset. Walk-down or
  walk-up after emptying a level is currently a linear scan across `uint64_t`
  slots; a bitset plus `__builtin_ctzll` / `_bit_scan_reverse` makes it
  O(1) in the sparse case.
- **`std::unordered_map` → flat vector index**. Ids are monotonic; a
  `std::vector<Block*>` sized to the peak-id window replaces the hash on cancel
  with a single load.
- **Correctness harness**. Currently trusting the shape of the numbers.
  A tiny hand-driven driver (~30 lines) inserting a fixed sequence and
  asserting invariants after each op would guard against regressions when
  swapping allocators or changing internal layout.
- **Vary the op mix**. What if cancel dominates (70%)? List's O(1) cancel
  should finally start paying. Parametrise via `state.range()`.
