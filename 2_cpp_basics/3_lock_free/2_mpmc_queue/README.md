# MPMC Queue

Bounded multi-producer multi-consumer queue, based on the Vyukov design.

## Source

- [Vyukov 1024 cores](https://sites.google.com/site/1024cores/home/lock-free-algorithms/queues/bounded-mpmc-queue)

## Design

Every cell carries a sequence number. It encodes both whose turn it is
(producer or consumer) and which lap around the ring we are on. The
invariant, for a thread whose ticket is `pos`:

- `seq == pos` -> cell is free, a producer at `pos` may claim it
- `seq == pos + 1` -> cell is full, a consumer at `pos` may claim it
- after consuming, the consumer sets `seq = pos + capacity`, which marks
  the cell free for the producer that will arrive on the next lap

The `diff` check in the code compares the cell's sequence with the
expected value. `diff == 0` means it is our turn. `diff < 0` means the
cell has not been recycled yet: for a producer that means the queue is
full, for a consumer that the queue is empty, and we return false.
`diff > 0` means another thread of our group already claimed this
ticket, so we reload the pointer and retry.

Producers do not race among each other: they read the enqueue pointer
and CAS it to claim a slot; only the winner writes the cell. Consumers
do the same on the dequeue pointer. Producers and consumers never race
on the same cell because the sequence number gives the cell to only one
side at a time.

The enqueue and dequeue pointers are padded to separate cache lines.
Both are contended atomics, and without padding they would sit on the
same line and every CAS from either group would ping-pong that line
between cores (false sharing). The benchmarks below show this is the
single biggest win.

## Memory model notes - General

General examples, not related to file.

### `seq_cst`

This is the strongest guarantee that you can find. It puts all in a
global unique order, amongst thread. This way reordering does not
happen and has less room for weird behavior

### `acquire`

Used on loads. No memory operation that comes after the load (in this
thread, on any memory location) can be reordered before it. When the
load sees a value written with `release`, everything the writing thread
did before its release store is visible here. "After I see the flag,
I see everything they did."

### `release`

Used on stores. No memory operation that comes before the store (in
this thread, on any memory location) can be reordered after it.
"Everything I did is published before the flag." Note the scope: it
covers all prior writes, not just the atomic variable itself - that is
what makes publishing plain data through an atomic flag work.

### `acq_rel`

`acquire` + `release`, for read-modify-write operations. Still weaker
than `seq_cst`: there is no single total order that all threads agree
on, so two observers can disagree about which of two independent writes
happened first.

### `relaxed`

No ordering guarantees. The operation is still atomic (no torn reads or
writes) and each variable still has a single modification order, but
the compiler and CPU are free to reorder it against everything else.
Fastest option.

## Benchmarks

Setup:

- CPU: 13th Gen Intel(R) Core(TM) i7-13700 (24 cores)
- Compiler: g++ 15.2.0, `-O2 -std=c++20 -pthread`
- 5 producers, 5 consumers, buffer size 1024
- ITER = 2M per producer -> 10M enqueue/dequeue pairs per run
- Median of 10 runs, whole-process wall clock
- Reproduce: `./bench.sh`

"Default ordering" is `seq_cst` (the strongest, what you get when you
do not specify anything), not "no ordering".

| Variant                       | Min (ms) | Median (ms) | Mean (ms) | M ops/s |
| ----------------------------- | -------- | ----------- | --------- | ------- |
| No padding, seq_cst           | 1163.4   | 1286.3      | 1266.9    | 3.9     |
| No padding, acq/rel + relaxed | 1054.8   | 1098.9      | 1131.4    | 4.6     |
| Padding, seq_cst              | 587.0    | 601.7       | 614.3     | 8.3     |
| Padding, acq/rel + relaxed    | 520.5    | 546.5       | 544.3     | 9.1     |

Padding roughly doubles throughput. Relaxing the memory ordering on top
adds ~10%.
