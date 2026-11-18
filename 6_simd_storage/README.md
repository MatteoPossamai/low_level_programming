# Quarter 6 - Plan

| Track | Deliverable |
| --- | --- |
| SIMD | Tested image-processing kernels, compared across scalar, compiler-vectorised, and handwritten SIMD implementations on the current computer |
| Storage | A persistent key-value store with recovery and compaction |

Sequence: SIMD 1 → Storage 1 → SIMD 2 → Storage 2 → SIMD 3 → Storage 3 → Write-up.

## Working method

- Read the assigned theory and explain the mechanism in your own words.
- Write a hypothesis before measuring.
- Establish correctness, inspect assembly or syscalls, then benchmark.
- Explain the difference between prediction and measurement.
- Finish the current block before switching tracks.
- Write the core algorithms yourself.
- Use C++20, CMake, GoogleTest, Google Benchmark, sanitizers, and perf.

## Block 1 - SIMD foundations and image baseline

Goal: understand vector lanes and establish a trustworthy reference.

- [x] Read [Algorithmica](https://en.algorithmica.org/hpc/simd/): SIMD introduction, vector types, data movement, and auto-vectorisation.
- [ ] Watch [Computer, Enhance!](https://www.computerenhance.com/p/table-of-contents) Part 3, lessons 5–7 and 9: assembly, latency/throughput, dependencies, and the CPU front end.
- [x] Define image layout, row stride, integer ranges, rounding, and border behavior. `stb_image.h`
- [x] Implement scalar grayscale conversion and pixel thresholding. Test tiny images, odd widths, padded rows, and boundary values.
- [x] Compare a verified non-vectorised build with compiler auto-vectorisation. Predict, inspect assembly, and measure.

Done when: outputs match the documented rules, tests pass, and you can explain the generated loop and its throughput.

## Block 2 - Storage layout and replay

Goal: build the smallest complete persistent store.

- [ ] Read [Database Internals](https://www.oreilly.com/library/view/database-internals/9781492040330/) chapters 1–3 and the Bitcask discussion in chapter 7.
- [ ] Write a short design: single process/writer, put/get, overwrites, record format, and an in-memory key-to-file-offset index.
- [ ] Implement append/read and rebuild the index by replaying the file on reopening. Handle partial I/O and report errors.
- [ ] Compare random operation sequences with a standard-library map. Test overwritten values and clean restarts.

Done when: the latest values survive a clean close/reopen and the file format is documented. Failure durability comes in Block 4.

## Block 3 - Handwritten SIMD on the current computer

Goal: control vector operations and explain their costs on the available CPU.

- [ ] Read [Algorithmica’s](https://en.algorithmica.org/hpc/simd/masking/) masking and shuffle sections, plus the intrinsic documentation for the SIMD instruction set supported by the current machine.
- [ ] Watch [Computer, Enhance!](https://www.computerenhance.com/p/table-of-contents) Part 3, lessons 12–15 and 17–18; Part 5, lessons 13 and 15–20.
- [ ] Identify the current CPU's SIMD features and select a concrete target (for example, x86-64 AVX2 where available).
- [ ] Implement handwritten SIMD versions of the first kernels. Handle remainders, padded rows, widening arithmetic, and unaligned input safely.
- [ ] Compare scalar, compiler-vectorised, and handwritten SIMD implementations on the same machine. Record CPU, compiler, flags, and relevant system conditions.

Done when: handwritten SIMD matches the reference and measured speedups or regressions are explained through instructions, dependencies, or data movement.

## Block 4 - Durability and failure recovery

Goal: define exactly what a successful write promises.

- [ ] Read [Database Internals](https://www.oreilly.com/library/view/database-internals/9781492040330/) chapter 5, SQLite’s atomic-commit/WAL explanations, and Linux fsync/fdatasync documentation.
- [ ] Define when a write is acknowledged and what may be lost after process failure, OS crash, or power loss.
- [ ] Add record integrity checks and explicit recovery rules. Test incomplete suffixes and corruption.
- [ ] Exercise interrupted writes using controlled file fixtures and child-process termination.
- [ ] Compare per-write and batched syncing, documenting the guarantee behind each measurement.

Done when: recovery tests pass, sync errors reach the caller, and write latency is tied to a clear durability contract.

Testing rule: killing a process does not simulate power loss. Document filesystem/device assumptions and the limits of the tests.

## Block 5 - Image filtering and CPU limits

Goal: handle neighbouring pixels and reductions.

- [ ] Finish Algorithmica’s SIMD chapter.
- [ ] Watch Computer, Enhance! Part 5, lessons 5, 9–12, and 21.
- [ ] Read OpenCV’s filtering and resizing theory.
- [ ] Implement scalar and handwritten SIMD versions of a 3×3 box blur. Define borders, accumulation width, and rounding; compare with equivalent OpenCV settings.
- [ ] Implement scalar/vector dot products with explicit floating-point tolerances.
- [ ] Predict computation versus memory limits, then measure across working-set sizes.

Done when: the stencil and reduction are correct, their assembly is understood, and at least one predicted bottleneck is measured and explained.

## Block 6 - Compaction and storage behaviour

Goal: reclaim obsolete data without breaking recovery.

- [ ] Read Database Internals chapters 4 and 6; finish chapter 7.
- [ ] Explain B-tree versus append/log-structured tradeoffs, including read/write amplification.
- [ ] Design and implement compaction retaining current values. Define safe file publication and required file/directory syncing first.
- [ ] Interrupt compaction at publication boundaries. Verify recovery and subsequent writes.
- [ ] Benchmark lookup/write latency, replay time, file growth, and compaction cost. Record device, filesystem, cache state, workload, and sync policy.

Done when: compaction preserves values, recovery tests pass, all Part I reading is complete, and measurements explain the storage costs.

## Block 7 - Write-up

- [ ] Write an image-processing report: design, correctness, hypotheses, assembly, measurements, and limits.
- [ ] Write a storage report: format, durability contract, recovery, compaction, measurements, and limits.
- [ ] Document reproducible build/run instructions and a demonstration for each project.

Done when: both projects can be reviewed and reproduced independently.

## Scope boundaries

- SIMD and image processing run on the current computer; no Raspberry Pi, camera, or hardware purchase is required.
- Use the CPU's available SIMD instruction set and its official documentation. Results describe this machine and should not be treated as measurements for other architectures.
- SQL, replication, concurrent transactions, and additional B-tree/LSM implementations are deferred.
- Read all of Database Internals Part I; implementing every structure discussed is unnecessary.
- Keep both projects independently testable and benchmarkable.

## Resources

- [Algorithmica SIMD](https://en.algorithmica.org/hpc/simd/)
- [Computer, Enhance!](https://www.computerenhance.com/p/table-of-contents)
- [Database Internals](https://www.databass.dev/book)
- [SQLite - atomic commit](https://sqlite.org/atomiccommit.html)
- [SQLite - write-ahead logging](https://sqlite.org/wal.html)
- [Linux - fsync/fdatasync](https://man7.org/linux/man-pages/man2/fsync.2.html)
- [Linux - rename](https://man7.org/linux/man-pages/man2/rename.2.html)
- [OpenCV - filtering](https://docs.opencv.org/4.x/d4/d13/tutorial_py_filtering.html)
- [OpenCV - resizing](https://docs.opencv.org/4.x/da/d6e/tutorial_py_geometric_transformations.html)
