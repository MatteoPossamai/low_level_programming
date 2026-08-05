# Notes

Small afterthoughts of the benchmark.

In the case of small allocations, the bump is by far the faster, just needs
to move ahead a pointer. No harder logic to implement. On the other hand,
the implicit need to scan, and with a big number of blocks takes long time.

For bump allocator, dimension does not really matter, is more or less stable.
For the implicit, the more they become bigger, the less allocations, the faster
they are, which makes sense, since small number of blocks to parse is almost
linear time.

The explicit is way faster than the implicit in most of the cases. Which makes
sense since the list should decrease the number of visited blocks significantly,
to only the free one.

In case there is random allocation, the number of blocks start to grow again,
so all take a toll, but the bump allocator overall, but that gets slower anyway.
