#include "allocator.h"
#include <cstdio>

#define ALIGN 16

// C test 1: init + deinit. With RAII this is construct + destruct: the
// constructor exits the process on mmap failure, so reaching the end of the
// scope without crashing is the success condition.
int test_1() {
  { Arena arena(4096); }
  printf("Test 1 - Success\n");
  return 0;
}


int test_2() {
  Arena arena(4096);
  int *memory = arena.alloc<int>(1);
  if (memory == nullptr) {
    printf("2. Unable to allocate valid memory\n");
    return 1;
  }
  *memory = 5;
  if (*memory != (int)5) {
    printf("2. Incorrect value in memory\n");
    return 1;
  }
  printf("Test 2 - Success\n");
  return 0;
}

int test_3() {
  Arena arena(4096);
  int *val1 = arena.alloc<int>(1);
  int *val2 = arena.alloc<int>(1);
  int *val3 = arena.alloc<int>(1);
  int diff = ALIGN / sizeof(int);

  if (val1 + diff != val2 || val2 + diff != val3) {
    printf("3. Invalid layout\n");
    return 1;
  }

  *val1 = 1;
  *val2 = 2;
  *val3 = 3;

  if (*val1 != 1 || *val2 != 2 || *val3 != 3) {
    printf("3. Invalid values in the address\n");
    return 1;
  }

  printf("Test 3 - Success\n");
  return 0;
}

int test_4() {
  Arena arena(1);
  int *val1 = arena.alloc<int>(1);

  if (val1 != nullptr) {
    printf("4. Should not allocate over dimension of buffer\n");
    return 1;
  }

  printf("Test 4 - Success\n");
  return 0;
}

int test_5() {
  Arena arena(ALIGN);
  char *val1 = arena.alloc<char>(1);

  if (val1 == nullptr) {
    printf("5. Should allocate correctly\n");
    return 1;
  }
  char *val2 = arena.alloc<char>(1);

  if (val2 != nullptr) {
    printf("5. Should not allocate over dimension of buffer\n");
    return 1;
  }
  arena.reset();
  int *val3 = arena.alloc<int>(1);
  if (val3 == nullptr) {
    printf("5. After reset should have available space\n");
    return 1;
  }

  printf("Test 5 - Success\n");
  return 0;
}

int main() {
  test_1();
  fflush(stdout);
  test_2();
  fflush(stdout);
  test_3();
  fflush(stdout);
  test_4();
  fflush(stdout);
  test_5();
  fflush(stdout);
}
