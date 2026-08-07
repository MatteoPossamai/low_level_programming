#include "allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test_1() {
  Allocator allocator = {0};
  int res = alloc_init(&allocator, 4096);
  if (res != 0) {
    printf("1. Allocation valid does not work properly\n");
    return 1;
  }
  res = alloc_deinit(&allocator);
  if (res != 0) {
    printf("1. De-llocation valid does not work properly\n");
    return 1;
  }
  printf("Test 1 - Success\n");
  return 0;
}

int test_2() {
  Allocator allocator = {0};
  int res = alloc_deinit(&allocator);
  if (res != 1) {
    printf("2. De-llocation invalid unallocated is not working\n");
    return 1;
  }
  printf("Test 2 - Success\n");
  return 0;
}

int test_3() {
  Allocator allocator = {0};
  if (alloc_init(&allocator, 4096) != 0) {
    printf("3. Init failed\n");
    return 1;
  }
  int *memory = (int *)alloc_malloc(&allocator, sizeof(int));
  if (memory == 0) {
    printf("3. Unable to allocate valid memory\n");
    return 1;
  }
  *memory = 5;
  if (*memory != (int)5) {
    printf("3. Incorrect value in memory\n");
    return 1;
  }
  alloc_deinit(&allocator);
  printf("Test 3 - Success\n");
  return 0;
}

int test_5() {
  Allocator allocator = {0};
  if (alloc_init(&allocator, 1) != 0) {
    printf("5. Init failed\n");
    return 1;
  }
  int *val1 = (int *)alloc_malloc(&allocator, sizeof(int));

  if (val1 != 0) {
    printf("5. Should not initialize over dimension of buffer\n");
    return 1;
  }

  alloc_deinit(&allocator);
  printf("Test 5 - Success\n");
  return 0;
}
// Two consecutive mallocs must both succeed and not overlap.
int test_6() {
  Allocator allocator = {0};
  alloc_init(&allocator, 4096);
  int *a = (int *)alloc_malloc(&allocator, sizeof(int));
  int *b = (int *)alloc_malloc(&allocator, sizeof(int));
  if (a == 0 || b == 0) {
    printf("6. Second malloc failed (a=%p b=%p)\n", (void *)a, (void *)b);
    alloc_deinit(&allocator);
    return 1;
  }
  *a = 111;
  *b = 222;
  if (*a != 111 || *b != 222) {
    printf("6. Blocks overlap or corrupt each other\n");
    alloc_deinit(&allocator);
    return 1;
  }
  alloc_deinit(&allocator);
  printf("Test 6 - Success\n");
  return 0;
}

// First-fit: freeing a block then allocating same size must reuse it.
int test_7() {
  Allocator allocator = {0};
  alloc_init(&allocator, 4096);
  void *a = alloc_malloc(&allocator, 32);
  void *b = alloc_malloc(&allocator, 32);
  alloc_free(&allocator, a);
  void *c = alloc_malloc(&allocator, 32);
  if (c != a) {
    printf("7. Freed block not reused (a=%p c=%p)\n", a, c);
    alloc_deinit(&allocator);
    return 1;
  }
  // b must still hold its data untouched by the reuse
  memset(b, 0xAB, 32);
  memset(c, 0xCD, 32);
  if (((unsigned char *)b)[0] != 0xAB) {
    printf("7. Reused block clobbered neighbor\n");
    alloc_deinit(&allocator);
    return 1;
  }
  alloc_deinit(&allocator);
  printf("Test 7 - Success\n");
  return 0;
}

// Double free must be rejected.
int test_8() {
  Allocator allocator = {0};
  alloc_init(&allocator, 4096);
  void *a = alloc_malloc(&allocator, 16);
  alloc_malloc(&allocator, 16); // keep last_ptr past a
  if (alloc_free(&allocator, a) != 0) {
    printf("8. First free rejected\n");
    alloc_deinit(&allocator);
    return 1;
  }
  if (alloc_free(&allocator, a) != 1) {
    printf("8. Double free not detected\n");
    alloc_deinit(&allocator);
    return 1;
  }
  alloc_deinit(&allocator);
  printf("Test 8 - Success\n");
  return 0;
}

// Freeing pointers outside the valid range must be rejected.
int test_9() {
  Allocator allocator = {0};
  alloc_init(&allocator, 4096);
  alloc_malloc(&allocator, 16);
  if (alloc_free(&allocator, allocator.meta_start_ptr) != 1) {
    printf("9. Free of start_ptr (no header before it) accepted\n");
    alloc_deinit(&allocator);
    return 1;
  }
  if (alloc_free(&allocator, (char *)allocator.meta_start_ptr + 4096 + 64) != 1) {
    printf("9. Free past end of region accepted\n");
    alloc_deinit(&allocator);
    return 1;
  }
  alloc_deinit(&allocator);
  printf("Test 9 - Success\n");
  return 0;
}

// Header overhead must count against capacity: payload alone fitting
// is not enough, header + payload must fit.
int test_10() {
  Allocator allocator = {0};
  alloc_init(&allocator, 64);
  // 64 - 16 header = 48 max payload. 60 fits region but not with header.
  void *a = alloc_malloc(&allocator, 60);
  if (a != 0) {
    printf("10. Allocation ignoring header overhead succeeded\n");
    alloc_deinit(&allocator);
    return 1;
  }
  void *b = alloc_malloc(&allocator, 48);
  if (b == 0) {
    printf("10. Exact-fit allocation failed\n");
    alloc_deinit(&allocator);
    return 1;
  }
  alloc_deinit(&allocator);
  printf("Test 10 - Success\n");
  return 0;
}

// Reset makes the whole region reusable.
int test_11() {
  Allocator allocator = {0};
  alloc_init(&allocator, 4096);
  void *a = alloc_malloc(&allocator, 128);
  alloc_reset(&allocator);
  void *b = alloc_malloc(&allocator, 128);
  if (b != a) {
    printf("11. After reset, malloc did not restart from beginning\n");
    alloc_deinit(&allocator);
    return 1;
  }
  alloc_deinit(&allocator);
  printf("Test 11 - Success\n");
  return 0;
}

// Stress: random malloc/free, each block filled with a unique byte,
// verified before free. Catches overlapping blocks and header corruption.
int test_12() {
  Allocator allocator = {0};
  alloc_init(&allocator, 1 << 20);
  enum { SLOTS = 64 };
  unsigned char *ptrs[SLOTS] = {0};
  size_t sizes[SLOTS] = {0};
  srand(42);

  for (int iter = 0; iter < 10000; iter++) {
    int i = rand() % SLOTS;
    if (ptrs[i] == 0) {
      size_t sz = 8 + (size_t)(rand() % 256);
      unsigned char *p = (unsigned char *)alloc_malloc(&allocator, sz);
      if (p == 0)
        continue; // out of memory is fine, corruption is not
      memset(p, i & 0xFF, sz);
      ptrs[i] = p;
      sizes[i] = sz;
    } else {
      for (size_t j = 0; j < sizes[i]; j++) {
        if (ptrs[i][j] != (unsigned char)(i & 0xFF)) {
          printf("12. Corruption in slot %d at byte %zu\n", i, j);
          alloc_deinit(&allocator);
          return 1;
        }
      }
      if (alloc_free(&allocator, ptrs[i]) != 0) {
        printf("12. Free of valid block %d failed\n", i);
        alloc_deinit(&allocator);
        return 1;
      }
      ptrs[i] = 0;
    }
  }
  alloc_deinit(&allocator);
  printf("Test 12 - Success\n");
  return 0;
}

int main() {
  int failures = 0;
  failures += test_1();
  failures += test_2();
  failures += test_3();
  failures += test_5();
  failures += test_6();
  failures += test_7();
  failures += test_8();
  failures += test_9();
  failures += test_10();
  failures += test_11();
  failures += test_12();
  if (failures != 0) {
    printf("%d test(s) FAILED\n", failures);
    return 1;
  }
  printf("All tests passed\n");
  return 0;
}
