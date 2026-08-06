#include "../implicit_free_list/allocator.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_ITER 1000
#define SEED 42

void bench_fixed(Allocator *allocator, size_t size, size_t iters) {
  void *ptrs[MAX_ITER];
  for (size_t i = 0; i < iters; i++)
    ptrs[i] = alloc_malloc(allocator, size);
  for (size_t i = 0; i < iters; i++)
    alloc_free(allocator, ptrs[i]);
}

void bench_random(Allocator *allocator, size_t iters) {
  void *ptrs[MAX_ITER];
  size_t sizes[MAX_ITER];
  srand(SEED);
  for (size_t i = 0; i < iters; i++)
    sizes[i] = 16 + (size_t)rand() % (4096 - 16 + 1);
  for (size_t i = 0; i < iters; i++)
    ptrs[i] = alloc_malloc(allocator, sizes[i]);
  for (size_t i = 0; i < iters; i++)
    alloc_free(allocator, ptrs[i]);
}

int main() {
  Allocator allocator = {0};
  size_t size = 1024 * 1024 * 10;

  if (alloc_init(&allocator, size) != 0) {
    fprintf(stderr, "alloc_init failed");
    return 1;
  }

  for (int i = 0; i < MAX_ITER * 5; i++) {
    // bench_fixed(&allocator, size, MAX_ITER);
    bench_random(&allocator, MAX_ITER);
    alloc_reset(&allocator);
  }
  alloc_deinit(&allocator);

  return 0;
}
