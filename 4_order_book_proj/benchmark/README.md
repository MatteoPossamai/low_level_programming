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

## `bench_network_device` — epoll vs io_uring

One loopback TCP request/response per iteration. The timed round trip includes
client send/receive, the inbound SPSC queue, and the backend's response send.
It is an end-to-end transport comparison, not an isolated syscall benchmark.
The benchmark starts one backend reader thread and keeps one client connection
open for each run.

```bash
./build/bench_network_device
./build/bench_network_device --benchmark_min_time=1s
./build/bench_network_device --benchmark_filter=IOUring
```

Build and run the socket tests with:

```bash
cmake --build build --target network_device_tests bench_network_device
ctest --test-dir build -R '^network_device_tests$' --output-on-failure
```

### Hypothesis and one run

Hypothesis: io_uring may be faster because submitting asynchronous work should
return without blocking for each socket operation. In one run, the loopback
round trip was slower with io_uring:

```
Benchmark                                          Time             CPU   Iterations UserCounters...
Network/Epoll/LoopbackRoundTrip/real_time       6350 ns         4447 ns       105433 items_per_second=157.469k/s
Network/IOUring/LoopbackRoundTrip/real_time     9866 ns         6471 ns        69781 items_per_second=101.355k/s
```

This is one result for this workload and setup; it does not establish which
backend is generally faster. The benchmark measures a full request/response,
not just the cost of submitting an asynchronous operation.

`io_uring` shines when the kernel calls can be spread across different calls,
but in this specific benchmark, due to the TCP nature of it, (as documented
[online](https://www.man7.org/linux/man-pages/man7/io_uring.7.html)) batching
does not really happen, and hence the extra machinery and operations put into
place to perform the `io_uring` way make it overall quite slower.

## Benchmark of all flow

### Optimizations applied

#### Optimization 1

```c++
+  sell_orders -= !is_buy;
+  buy_orders -= is_buy;
+
   if (order_buffer[idx].head == nullptr) {
     if (is_buy && idx == best_buy_idx) {
       while (best_buy_idx > 0 && order_buffer[best_buy_idx].head == nullptr)
         best_buy_idx--;
     } else if (!is_buy && idx == best_sell_idx) {
-      while (best_sell_idx < BUFFER_SIZE &&
+      while (sell_orders > 0 && best_sell_idx < BUFFER_SIZE &&
              order_buffer[best_sell_idx].head == nullptr)
         best_sell_idx++;
+      if (sell_orders == 0)
+        best_sell_idx = BUFFER_SIZE;
     }
```

Why: avoids walking the rest of the empty vector in case the offer is empty
(extended to ask as well).

### Internal engine benchmark

Tells the time that it takes to go from call to `process` to the result
being enqueued into the outbound queue, that is what the engine thread
would take care to do.

#### Result pre any optimization

```shell
TSC frequency: 2803.13 MHz | discarded migrated samples: 0

Request              Samples    P50 ns    P99 ns  P99.9 ns    Max ns    P99.9 idx      Max idx
----------------------------------------------------------------------------------------------
all                   100000       168      1093   4077585   4552976        91076        90387
limit_passive          47778       164       703      1258   3734680        93540        72752
limit_aggressive        2532       317      1459   3999870   4415889        80127        87987
market                  1946       457   3709327   4173513   4471113        83891        92218
cancel                 47744       169   3767791   4173910   4552976        81058        90387

```

#### Result post optimization 1

```shell
TSC frequency: 2803.13 MHz | discarded migrated samples: 0

Request              Samples    P50 ns    P99 ns  P99.9 ns    Max ns    P99.9 idx      Max idx
----------------------------------------------------------------------------------------------
all                   100000        59       243       511   2900828        83271        94571
limit_passive          47778        58       149       333   2332512        85801        72752
limit_aggressive        2532       110       534   2323351   2652857        99934        93605
market                  1946       153   2289224   2834116   2900828        97495        94571
cancel                 47744        60       151       341      3658        34628        53046
```

### Wire to wire benchmark

Computes the time it takes from the message sent onto the wire until
the ACK is coming back. Note that the threads and sender are on the same
machine anyway.

#### Result pre any optimization

```shell
TSC frequency: 2803.11 MHz

Request              Samples    P50 ns    P99 ns  P99.9 ns    Max ns    P99.9 idx      Max idx
----------------------------------------------------------------------------------------------
all                    98730    105999  40953104  41727266  49468511        59917        74508
limit_passive          47178    106653  40934864  41710547  42350755        60620        95470
limit_aggressive        2506    108874  40920910  41814377  49468511        59515        74508
market                  1929    109481  40817425  41598434  41931194        49597        91128
cancel                 47117    104814  40980999  41772178  42689551        85062        60195
```

#### Result post optimization 1

```shell
TSC frequency: 2803.15 MHz

Request              Samples    P50 ns    P99 ns  P99.9 ns    Max ns    P99.9 idx      Max idx
----------------------------------------------------------------------------------------------
all                    99642     17851  40914443  41736708  42159736        85604        66029
limit_passive          47589     18416  40912311  41724426  42159736         8861        66029
limit_aggressive        2521     18864  40941898  41751910  42084937        12422        83058
market                  1942     18496  40695851  41626332  41782064        63860        50617
cancel                 47590     17268  40919374  41747626  42154596        65238        87157
```
