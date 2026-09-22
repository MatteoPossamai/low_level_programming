# Data Structures

## Possible Operations

- Insert order: 45%
- Cancel by id: 43%
- Market order: 2%
- Levels: 10%

## Events

- Execute: 10%

## Empirical structure comparison

### `map`

Not optimal since there is no order. To find the top of the book you need to
look up all data points, with `O(n)` complexity. Not desirable.

Given complexity and clearly inferior algorithm, this will not be tested.

### Sorted `vector`

Better than `map` on average but still requires to keep the array in
order, which means linearly scan and moving all of the data on top every
time. Amortized might not be a lot, but in general is `O(n)`.

To test to verify if the hypothesis hold.

### Linked list (intrusive)

Easy to find top of book in `0(1)` and match stuff up. But the insert is again
linear and depending on the depth it might be `O(n)` and walking a list is
slow due to caches.

To test to verify if the hypothesis hold.

### Sparse arrays with tick size slots as index

Expensive in terms of memory, but gives for free the levels (just get last X
cells, if you have pointer), and inserting is also constant. Might need to
keep some reference to given orders if they were to be cancelled. That might
be a bit more tricky, but sorting a vector with only one tick prize might be
less work instead.

Each level is a linked list, so adding and removing is constant.

## TODO: pool allocator

Both linked list and sparse array use `new`/`delete` per node. Swap in a slab
pool later for both. Rerun benchmarks to isolate allocator cost from
structure cost.

## Results

```shell
-------------------------------------------------------------------------------------------------------------
Benchmark                                                   Time             CPU   Iterations UserCounters...
-------------------------------------------------------------------------------------------------------------
BM_Workload<OrderBook_SortedVector>/100/10000          195910 ns       195869 ns         3572 items_per_second=51.0545M/s
BM_Workload<OrderBook_SortedVector>/1000/10000         270064 ns       270024 ns         2588 items_per_second=37.0338M/s
BM_Workload<OrderBook_SortedVector>/10000/5000         487430 ns       487373 ns         1429 items_per_second=10.2591M/s
BM_Workload<OrderBook_SortedVector>/50000/1000         465668 ns       465596 ns         1502 items_per_second=2.14778M/s
BM_Workload<OrderBook_List>/100/10000                  280008 ns       280033 ns         2500 items_per_second=35.71M/s
BM_Workload<OrderBook_List>/1000/10000                 362788 ns       362799 ns         1928 items_per_second=27.5635M/s
BM_Workload<OrderBook_List>/10000/5000                1120371 ns      1120330 ns          623 items_per_second=4.46297M/s
BM_Workload<OrderBook_List>/50000/1000                1516649 ns      1516255 ns          454 items_per_second=659.52k/s
BM_Workload<OrderBook_TickOffset<1024>>/100/10000      512028 ns       511965 ns         1357 items_per_second=19.5326M/s
BM_Workload<OrderBook_TickOffset<1024>>/1000/10000     554573 ns       554516 ns         1240 items_per_second=18.0338M/s
BM_Workload<OrderBook_TickOffset<1024>>/10000/5000     352603 ns       352576 ns         1985 items_per_second=14.1813M/s
BM_Workload<OrderBook_TickOffset<1024>>/50000/1000     336825 ns       336753 ns         2078 items_per_second=2.96953M/s
low_level_programming/4_order_book_proj/build on  main [!] took 1m25s ❯
```

### Explanation

- List is worst, cache hostile and slow across the board
- Sorted vector is simple with no setup costs, so for small workflows wins
- Tick offset has some overhead, and for smaller workflows it can be seen, but then
  it stays relatively flat, and is the one that suffers less from growth.

Since our order book must scale and have large number of orders, the baseline of
the Tick offset is the choice that we are doing. This benchmark is pre optimizations
like pool allocation and similar, that might bring more performance improvements
and will arrive just later.

## Notes

- Was expecting to find the tick offset the best across the board.
  Seeing it was not an obvious choice was unexpected.
