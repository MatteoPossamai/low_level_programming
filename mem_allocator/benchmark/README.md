# Benchmark

## Bump allocator

Runs per workload: 5. An op is one malloc+free pair (interleaved: one malloc or free).

| Workload       | Iterations | Native ns/op | Impl ns/op | Impl/Native |
|----------------|-----------:|-------------:|-----------:|------------:|
| small 64B      |      10000 |         25.5 |       15.8 |        0.6x |
| mix 1KB        |      10000 |        244.5 |      203.5 |        0.8x |
| big 10MB       |         20 |       2029.3 |      932.0 |        0.5x |
| interleaved    |      10000 |          5.9 |        8.1 |        1.4x |
| random 16B-4KB |      10000 |        127.2 |      370.9 |        2.9x |

## Implicit free list

Runs per workload: 5. An op is one malloc+free pair (interleaved: one malloc or free).

| Workload       | Iterations | Native ns/op | Impl ns/op | Impl/Native |
|----------------|-----------:|-------------:|-----------:|------------:|
| small 64B      |      10000 |         22.6 |     6547.1 |      289.1x |
| mix 1KB        |      10000 |        224.1 |     8348.5 |       37.3x |
| big 10MB       |         20 |       2249.7 |     1020.3 |        0.5x |
| interleaved    |      10000 |          5.8 |       24.4 |        4.2x |
| random 16B-4KB |      10000 |        147.1 |    28739.2 |      195.4x |

## Implicit free list + coalesce

Runs per workload: 5. An op is one malloc+free pair (interleaved: one malloc or free).

| Workload       | Iterations | Native ns/op | Impl ns/op | Impl/Native |
|----------------|-----------:|-------------:|-----------:|------------:|
| small 64B      |      10000 |         22.5 |     6202.3 |      275.6x |
| mix 1KB        |      10000 |        210.5 |     8250.4 |       39.2x |
| big 10MB       |         20 |       2209.6 |     1203.5 |        0.5x |
| interleaved    |      10000 |          5.9 |       25.7 |        4.4x |
| random 16B-4KB |      10000 |        154.3 |    28807.7 |      186.7x |

## Explicit free list

Runs per workload: 5. An op is one malloc+free pair (interleaved: one malloc or free).

| Workload       | Iterations | Native ns/op | Impl ns/op | Impl/Native |
|----------------|-----------:|-------------:|-----------:|------------:|
| small 64B      |      10000 |         30.7 |       27.9 |        0.9x |
| mix 1KB        |      10000 |        197.8 |      167.2 |        0.8x |
| big 10MB       |         20 |       1957.4 |      986.2 |        0.5x |
| interleaved    |      10000 |          5.8 |        6.5 |        1.1x |
| random 16B-4KB |      10000 |        119.9 |      389.5 |        3.2x |
