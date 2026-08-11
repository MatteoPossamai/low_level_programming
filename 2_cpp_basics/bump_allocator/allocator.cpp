#include "allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

Arena::Arena(size_t size) {
  start_ptr =
      mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (start_ptr == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
  this->size = size;
  this->curr_ptr = this->start_ptr;
}

Arena::~Arena() {
  munmap(start_ptr, size);
  start_ptr = nullptr;
  curr_ptr = nullptr;
}

auto Arena::dealloc(void *ptr) -> size_t {
  (void)ptr;
  return 0;
}

auto Arena::reset() -> size_t {
  curr_ptr = start_ptr;
  return 0;
}
