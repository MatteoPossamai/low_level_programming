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
