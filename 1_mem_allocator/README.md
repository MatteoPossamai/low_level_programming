# Memory Allocators

Exercises of building different types of memory allorators
and benchmarking those. Then trying to run `perf stat` to find
the bottleneck in one of those, to then given an optimized version
for runtime.

## Different allocators

- [Bump allocator](./bump_allocator/)
- [Implicit free list Allocator](./implicit_free_list/)
- [Implicit free list Allocator - Improved](./implicit_free_list-improved/)
- [Implicit free list with coalasce](./implicit_free_list_coalasce/)
- [Explicit free list](./list_explicit_free/)

## Benchmarks and Perf

- [Benchmarks](./benchmark/)
- [Perf](./perf/)

## Story

Initially created all the allocators from the simplest (bump)
to the most complex (Explicit list).

The initial allocator
was very fast to allocate, but did not deallocate memory, causing
leaks if not de-initialized often, making it not desirable.

Implicit list allocators resolved that problem, giving up though
on way more expensive allocation algorithms, that required linear
scan of the cells in the allocated memory. The coalasce tried to
recompact memory so that the cells were less on average and more
space could be reused, making it better in the long run.

Explicit free list on the other hand was more powerful since on an
alternative data structure kept another linked list with all the free
blocks, making the scan much more efficient to perform, while still
maintaining the ability to free up memory, making it by far the best
compared to implicit list allocators, and not that bad compared to
the bump allocator according to benchmarks.

Then choose one at random (implicit list) and tried to profile it using
`perf stat`. Found out that the main cost there was parsing every time
again and again the header, that given the random access caused high
cache misses. So we tried to swap from Array of Structure (AoS) to
Structure of Array (SoA) where we have a second array linking to the data
array to be faster and more predictable for CPU to cache what is required.

This means that memory consumed to perform the same increased almost double,
but at the same time, runtime improved significantly across the board by
a relevant factor, especially with smaller to medium blocks.

## Learning

- Touched first hand how much cache locality can mean in terms for performance
- Seen how to trace and profile a bit the code to find bottlenecks
- Mental model to read performance is always relative, never absolute (divide)
- Iteratively find the bottleneck and try to improve to get out better and better perf
- Is very easy to mess up pointer arithmetics
