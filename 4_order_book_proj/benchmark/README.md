# Benchmark results

Machine, `-O2`, DEBUG-mode `libbenchmark`, CPU scaling enabled. Numbers are
representative, not p99.

## `bench_order_book` — order book workload

Op mix 45/43/2/10 (insert/cancel/market/top). Prices `[1, 1000]`,
sizes `[1, 100]`. Args `{book_depth, ops_per_iteration}`.

```
Benchmark                                                   Time             CPU   Iterations UserCounters...
-------------------------------------------------------------------------------------------------------------
BM_Workload<OrderBook_SortedVector>/100/10000          195132 ns       195150 ns         2128 items_per_second=51.2426M/s
BM_Workload<OrderBook_SortedVector>/1000/10000         272249 ns       272264 ns         1539 items_per_second=36.7291M/s
BM_Workload<OrderBook_SortedVector>/10000/5000         488972 ns       488963 ns          857 items_per_second=10.2257M/s
BM_Workload<OrderBook_SortedVector>/50000/1000         469641 ns       469588 ns          894 items_per_second=2.12953M/s
BM_Workload<OrderBook_List>/100/10000                  282544 ns       282566 ns         1493 items_per_second=35.39M/s
BM_Workload<OrderBook_List>/1000/10000                 361793 ns       361792 ns         1163 items_per_second=27.6402M/s
BM_Workload<OrderBook_List>/10000/5000                1122167 ns      1122151 ns          373 items_per_second=4.45573M/s
BM_Workload<OrderBook_List>/50000/1000                1509794 ns      1509244 ns          280 items_per_second=662.583k/s
BM_Workload<OrderBook_TickOffset<1024>>/100/10000      520681 ns       520691 ns          803 items_per_second=19.2053M/s
BM_Workload<OrderBook_TickOffset<1024>>/1000/10000     570120 ns       570146 ns          737 items_per_second=17.5394M/s
BM_Workload<OrderBook_TickOffset<1024>>/10000/5000     364264 ns       364201 ns         1146 items_per_second=13.7287M/s
BM_Workload<OrderBook_TickOffset<1024>>/50000/1000     346190 ns       346026 ns         1201 items_per_second=2.88996M/s
```

## `bench_codec` — OUCH 5.0 codec

Single `EnterRequest` message. Builder held in named local across the loop.
Roundtrip assertion runs before benchmarks; failure aborts.

```
roundtrip: OK
----------------------------------------------------------------
Benchmark                      Time             CPU   Iterations
----------------------------------------------------------------
BM_EncodeEnterRequest       1.38 ns         1.38 ns    303439890
BM_DecodeDispatch          0.714 ns        0.714 ns    588711920
BM_DecodePartialRead        1.11 ns         1.11 ns    378536388
BM_DecodeFullRead           1.28 ns         1.28 ns    326948873
```
