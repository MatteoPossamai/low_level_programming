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
    printf("5. Should not initialize over dimension of buffer");
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
  if (alloc_free(&allocator, allocator.start_ptr) != 1) {
    printf("9. Free of start_ptr (no header before it) accepted\n");
    alloc_deinit(&allocator);
    return 1;
  }
  if (alloc_free(&allocator, (char *)allocator.start_ptr + 4096 + 64) != 1) {
    printf("9. Free past end of region accepted\n");
    alloc_deinit(&allocator);
    return 1;
  }
  alloc_deinit(&allocator);
  printf("Test 9 - Success\n");
  return 0;
}

// Overhead must count against capacity: header (16) + footer (8) = 24
// bytes per block, so region 72 holds exactly one 48-byte payload.
int test_10() {
  Allocator allocator = {0};
  alloc_init(&allocator, 72);
  // 56 rounds to 64; 64 + 24 = 88 > 72, must fail.
  void *a = alloc_malloc(&allocator, 56);
  if (a != 0) {
    printf("10. Allocation ignoring header/footer overhead succeeded\n");
    alloc_deinit(&allocator);
    return 1;
  }
  // 48 + 24 = 72, exact fit.
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

// Forward coalescing: free B, then free A. A must absorb B, so a
// request bigger than either single block fits at A's address.
// Merged payload: 32 + 32 + 24 (B's header+footer become payload) = 88.
int test_13() {
  Allocator allocator = {0};
  alloc_init(&allocator, 4096);
  void *a = alloc_malloc(&allocator, 32);
  void *b = alloc_malloc(&allocator, 32);
  void *c = alloc_malloc(&allocator, 32); // guard: keeps B off last_ptr edge
  memset(c, 0x77, 32);
  alloc_free(&allocator, b);
  alloc_free(&allocator, a);
  void *big = alloc_malloc(&allocator, 64);
  if (big != a) {
    printf("13. Forward coalesce failed (a=%p big=%p)\n", a, big);
    alloc_deinit(&allocator);
    return 1;
  }
  if (((unsigned char *)c)[0] != 0x77 || ((unsigned char *)c)[31] != 0x77) {
    printf("13. Coalescing corrupted the guard block\n");
    alloc_deinit(&allocator);
    return 1;
  }
  alloc_deinit(&allocator);
  printf("Test 13 - Success\n");
  return 0;
}

// Backward coalescing: free A, then free B. B must merge into A.
int test_14() {
  Allocator allocator = {0};
  alloc_init(&allocator, 4096);
  void *a = alloc_malloc(&allocator, 32);
  void *b = alloc_malloc(&allocator, 32);
  void *c = alloc_malloc(&allocator, 32);
  memset(c, 0x77, 32);
  alloc_free(&allocator, a);
  alloc_free(&allocator, b);
  void *big = alloc_malloc(&allocator, 64);
  if (big != a) {
    printf("14. Backward coalesce failed (a=%p big=%p)\n", a, big);
    alloc_deinit(&allocator);
    return 1;
  }
  if (((unsigned char *)c)[0] != 0x77) {
    printf("14. Coalescing corrupted the guard block\n");
    alloc_deinit(&allocator);
    return 1;
  }
  alloc_deinit(&allocator);
  printf("Test 14 - Success\n");
  return 0;
}

// Both sides at once: A and C free, then freeing B must merge all three.
// Merged payload: 3*32 + 2*24 = 144.
int test_15() {
  Allocator allocator = {0};
  alloc_init(&allocator, 4096);
  void *a = alloc_malloc(&allocator, 32);
  void *b = alloc_malloc(&allocator, 32);
  void *c = alloc_malloc(&allocator, 32);
  void *d = alloc_malloc(&allocator, 32); // guard
  memset(d, 0x55, 32);
  alloc_free(&allocator, a);
  alloc_free(&allocator, c);
  alloc_free(&allocator, b);
  void *big = alloc_malloc(&allocator, 144);
  if (big != a) {
    printf("15. Two-sided coalesce failed (a=%p big=%p)\n", a, big);
    alloc_deinit(&allocator);
    return 1;
  }
  if (((unsigned char *)d)[0] != 0x55) {
    printf("15. Coalescing corrupted the guard block\n");
    alloc_deinit(&allocator);
    return 1;
  }
  alloc_deinit(&allocator);
  printf("Test 15 - Success\n");
  return 0;
}

// Splitting: a small alloc from a big free block must leave the
// remainder usable. Remainder of 256-block after 32: 256-32-24 = 200.
int test_16() {
  Allocator allocator = {0};
  alloc_init(&allocator, 4096);
  void *p = alloc_malloc(&allocator, 256);
  void *guard = alloc_malloc(&allocator, 32);
  memset(guard, 0x99, 32);
  alloc_free(&allocator, p);

  void *small = alloc_malloc(&allocator, 32);
  if (small != p) {
    printf("16. Split block not placed at freed address\n");
    alloc_deinit(&allocator);
    return 1;
  }
  // Remainder header starts at p + 32 + 8 (footer), payload 16 further.
  void *rest = alloc_malloc(&allocator, 160);
  if (rest != (char *)p + 32 + 3 * sizeof(size_t)) {
    printf("16. Remainder after split not reused (p=%p rest=%p)\n", p, rest);
    alloc_deinit(&allocator);
    return 1;
  }
  memset(small, 0x11, 32);
  memset(rest, 0x22, 160);
  if (((unsigned char *)guard)[0] != 0x99) {
    printf("16. Split corrupted the guard block\n");
    alloc_deinit(&allocator);
    return 1;
  }
  alloc_deinit(&allocator);
  printf("Test 16 - Success\n");
  return 0;
}

// README done-criterion: heap must not fragment into uselessness.
// 9 blocks of 48, freed in scattered order, must merge back into one
// block of 9*48 + 8*24 = 624 that a single big alloc can take.
int test_17() {
  Allocator allocator = {0};
  alloc_init(&allocator, 4096);
  void *p[9];
  for (int i = 0; i < 9; i++)
    p[i] = alloc_malloc(&allocator, 48);
  for (int i = 0; i < 9; i += 2)
    alloc_free(&allocator, p[i]);
  for (int i = 1; i < 9; i += 2)
    alloc_free(&allocator, p[i]);
  void *big = alloc_malloc(&allocator, 624);
  if (big != p[0]) {
    printf("17. Heap fragmented: scattered frees did not merge "
           "(p0=%p big=%p)\n",
           p[0], big);
    alloc_deinit(&allocator);
    return 1;
  }
  alloc_deinit(&allocator);
  printf("Test 17 - Success\n");
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
  failures += test_13();
  failures += test_14();
  failures += test_15();
  failures += test_16();
  failures += test_17();
  if (failures != 0) {
    printf("%d test(s) FAILED\n", failures);
    return 1;
  }
  printf("All tests passed\n");
  return 0;
}
