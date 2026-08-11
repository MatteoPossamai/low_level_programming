# Benchmark

## Bump allocator

Runs per workload: 5. An op is one malloc+free pair (interleaved: one malloc or free).

| Workload       | Iterations | Native ns/op | Impl ns/op | Impl/Native |
|----------------|-----------:|-------------:|-----------:|------------:|
| small 64B      |      10000 |         34.8 |       32.5 |        0.9x |
| mix 1KB        |      10000 |        616.9 |      426.6 |        0.7x |
| big 10MB       |         20 |       8026.8 |     2091.5 |        0.3x |
| interleaved    |      10000 |         14.2 |       15.5 |        1.1x |
| random 16B-4KB |      10000 |        229.9 |      629.1 |        2.7x |

## Implicit free list

Runs per workload: 5. An op is one malloc+free pair (interleaved: one malloc or free).

| Workload       | Iterations | Native ns/op | Impl ns/op | Impl/Native |
|----------------|-----------:|-------------:|-----------:|------------:|
| small 64B      |      10000 |         30.9 |    21387.0 |      691.2x |
| mix 1KB        |      10000 |        419.4 |    28536.2 |       68.0x |
| big 10MB       |         20 |       5002.9 |     1356.1 |        0.3x |
| interleaved    |      10000 |         13.0 |       41.4 |        3.2x |
| random 16B-4KB |      10000 |        235.9 |    47697.7 |      202.2x |

## Implicit free list IMPROVED

Runs per workload: 5. An op is one malloc+free pair (interleaved: one malloc or free).

| Workload       | Iterations | Native ns/op | Impl ns/op | Impl/Native |
|----------------|-----------:|-------------:|-----------:|------------:|
| small 64B      |      10000 |         26.0 |     3512.8 |      135.2x |
| mix 1KB        |      10000 |        414.5 |     3829.8 |        9.2x |
| big 10MB       |         20 |       4765.9 |     1320.0 |        0.3x |
| interleaved    |      10000 |         12.8 |       20.2 |        1.6x |
| random 16B-4KB |      10000 |        229.5 |     4252.5 |       18.5x |

## Implicit free list + coalesce

Runs per workload: 5. An op is one malloc+free pair (interleaved: one malloc or free).

| Workload       | Iterations | Native ns/op | Impl ns/op | Impl/Native |
|----------------|-----------:|-------------:|-----------:|------------:|
| small 64B      |      10000 |         32.9 |    21583.9 |      656.1x |
| mix 1KB        |      10000 |        430.2 |    29531.9 |       68.6x |
| big 10MB       |         20 |       3557.5 |     1410.8 |        0.4x |
| interleaved    |      10000 |         12.7 |       43.7 |        3.4x |
| random 16B-4KB |      10000 |        233.9 |    49515.0 |      211.7x |

## Explicit free list

Runs per workload: 5. An op is one malloc+free pair (interleaved: one malloc or free).

| Workload       | Iterations | Native ns/op | Impl ns/op | Impl/Native |
|----------------|-----------:|-------------:|-----------:|------------:|
| small 64B      |      10000 |         42.0 |       57.8 |        1.4x |
| mix 1KB        |      10000 |        491.2 |      372.5 |        0.8x |
| big 10MB       |         20 |       4723.8 |     1246.9 |        0.3x |
| interleaved    |      10000 |         12.8 |       11.8 |        0.9x |
| random 16B-4KB |      10000 |        223.0 |      653.4 |        2.9x |
