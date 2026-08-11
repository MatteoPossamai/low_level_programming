#define _POSIX_C_SOURCE 200809L
#ifndef ALLOC_HEADER
#define ALLOC_HEADER "../list_explicit_free/allocator.h"
#endif
#include ALLOC_HEADER
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define RUNS 5
#define MAX_ITER 10000
#define WINDOW 100
#define SEED 42

static inline uint64_t now_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

/* allocator == NULL means native malloc/free */
static inline void *do_malloc(Allocator *allocator, size_t size) {
  void *ptr = allocator ? alloc_malloc(allocator, size) : malloc(size);
  /* touch the memory so the compiler cannot elide the native malloc/free
   * pair; applied to both paths so the comparison stays fair */
  ((volatile char *)ptr)[0] = 1;
  return ptr;
}

static inline void do_free(Allocator *allocator, void *ptr) {
  if (allocator)
    alloc_free(allocator, ptr);
  else
    free(ptr);
}

/* alloc everything, then free everything, one fixed size */
static uint64_t bench_fixed(Allocator *allocator, size_t size, size_t iters) {
  static void *ptrs[MAX_ITER];
  uint64_t start = now_ns();
  for (size_t i = 0; i < iters; i++)
    ptrs[i] = do_malloc(allocator, size);
  for (size_t i = 0; i < iters; i++)
    do_free(allocator, ptrs[i]);
  return now_ns() - start;
}

/* alloc everything, then free everything, random sizes (fixed seed so
 * every allocator sees the identical sequence) */
static uint64_t bench_random(Allocator *allocator, size_t iters) {
  static void *ptrs[MAX_ITER];
  static size_t sizes[MAX_ITER];
  srand(SEED);
  for (size_t i = 0; i < iters; i++)
    sizes[i] = 16 + (size_t)rand() % (4096 - 16 + 1);
  uint64_t start = now_ns();
  for (size_t i = 0; i < iters; i++)
    ptrs[i] = do_malloc(allocator, sizes[i]);
  for (size_t i = 0; i < iters; i++)
    do_free(allocator, ptrs[i]);
  return now_ns() - start;
}

/* window of live pointers, each op hits a random slot: free it if
 * occupied, allocate into it otherwise - stresses free-block reuse */
static uint64_t bench_interleaved(Allocator *allocator, size_t ops) {
  static int slot_of[MAX_ITER];
  void *slots[WINDOW] = {0};
  srand(SEED);
  for (size_t i = 0; i < ops; i++)
    slot_of[i] = rand() % WINDOW;
  uint64_t start = now_ns();
  for (size_t i = 0; i < ops; i++) {
    int s = slot_of[i];
    if (slots[s]) {
      do_free(allocator, slots[s]);
      slots[s] = NULL;
    } else {
      slots[s] = do_malloc(allocator, 64);
    }
  }
  uint64_t elapsed = now_ns() - start;
  for (int s = 0; s < WINDOW; s++)
    if (slots[s])
      do_free(allocator, slots[s]);
  return elapsed;
}

typedef enum { FIXED, RANDOM, INTERLEAVED } Kind;

typedef struct {
  const char *name;
  Kind kind;
  size_t size;   /* only for FIXED */
  size_t iters;
  size_t arena;  /* worst case incl. bump allocator, whose free is a no-op */
} Workload;

static uint64_t run_workload(const Workload *w, Allocator *allocator) {
  switch (w->kind) {
  case FIXED:
    return bench_fixed(allocator, w->size, w->iters);
  case RANDOM:
    return bench_random(allocator, w->iters);
  case INTERLEAVED:
  default:
    return bench_interleaved(allocator, w->iters);
  }
}

int main(void) {
  const Workload workloads[] = {
      {"small 64B", FIXED, 64, 10000, 16ull << 20},
      {"mix 1KB", FIXED, 1024, 10000, 32ull << 20},
      {"big 10MB", FIXED, 10ull << 20, 20, 512ull << 20},
      {"interleaved", INTERLEAVED, 0, 10000, 16ull << 20},
      {"random 16B-4KB", RANDOM, 0, 10000, 64ull << 20},
  };
  const size_t n = sizeof(workloads) / sizeof(workloads[0]);

  printf("Runs per workload: %d. An op is one malloc+free pair "
         "(interleaved: one malloc or free).\n\n",
         RUNS);
  printf("| Workload       | Iterations | Native ns/op | Impl ns/op | "
         "Impl/Native |\n");
  printf("|----------------|-----------:|-------------:|-----------:|"
         "------------:|\n");

  for (size_t i = 0; i < n; i++) {
    const Workload *w = &workloads[i];
    uint64_t native = 0, impl = 0;

    for (int r = 0; r < RUNS; r++) {
      native += run_workload(w, NULL);

      Allocator allocator = {0};
      if (alloc_init(&allocator, w->arena) != 0) {
        fprintf(stderr, "alloc_init failed for %s\n", w->name);
        return 1;
      }
      impl += run_workload(w, &allocator);
      alloc_deinit(&allocator);
    }

    uint64_t ops = w->iters * RUNS;
    double native_op = (double)native / (double)ops;
    double impl_op = (double)impl / (double)ops;
    printf("| %-14s | %10zu | %12.1f | %10.1f | %10.1fx |\n", w->name,
           w->iters, native_op, impl_op, impl_op / native_op);
  }

  return 0;
}
