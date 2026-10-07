# Perf

## Count whole-program events

```shell
perf stat ./EXEC
```

Shows all hardware and OS counters.

```shell
perf stat --topdown ./EXEC
```

Top down groups CPU slots into **retiring**, **bad speculation**, **backend bound**, and **frontend bound**. These point to broad areas to investigate; they don’t identify the exact slow line.

```shell
perf stat -e cycles,instructions,branches,branch-misses,page-faults,context-switches,cpu-migrations -- ./EXEC
```

IPC is instructions divided by cycles. It gives a rough sense of how much work the CPU completes per cycle, but by itself doesn’t say whether performance is good

## Profile CPU

```shell
perf record -g -e cycles:u -- ./EXEC
perf report --stdio
```

Record samples CPU cycles in `perf.data`, that is read from report.
`-g` to leave debug symbols and see the code.

To inspect a function’s annotated instructions:

```
perf annotate --stdio --symbol ANNOTATION (function name)
```
