#include "allocator.h"
#include <stdio.h>
#include <sys/mman.h>

Header _parse_header(void *ptr) {
  return (Header){.block_size = *(size_t *)(ptr - 2 * sizeof(size_t)),
                  .allocated = *(size_t *)(ptr - sizeof(size_t))};
}

int alloc_init(Allocator *allocator, size_t size) {
  if (allocator->start_ptr != 0) {
    return 1; // Invalid if the memory is already initialized. Need
              // de-initialization
  }
  allocator->start_ptr =
      mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (allocator->start_ptr == MAP_FAILED) {
    perror("mmap");
    return 1;
  }
  allocator->last_ptr = allocator->start_ptr;
  allocator->size = size;
  return 0;
}

int alloc_deinit(Allocator *allocator) {
  if (allocator->start_ptr == 0) {
    return 1; // Cannot de-init if never initialized
  }
  munmap(allocator->start_ptr, allocator->size);
  allocator->start_ptr = 0;
  allocator->last_ptr = 0;
  allocator->size = 0;
  return 0;
}

void *alloc_malloc(Allocator *allocator, size_t size) {
  // Round up so every block size is a multiple of ALIGN; header is 16 bytes,
  // so all headers and payloads stay 16-byte aligned.
  size = (size % ALIGN == 0) ? size : size + (ALIGN - (size % ALIGN));
  void *current = allocator->start_ptr;
  size_t found = 0;
  Header header;

  while (found == 0 && current < allocator->last_ptr) {
    // Header parsing
    current += 2 * sizeof(size_t);
    header = _parse_header(current);

    // Check if block is usable
    if (header.allocated == 0 && header.block_size >= size) {
      found = 1;
      current -= 2 * sizeof(size_t);
    } else {
      current += header.block_size;
    }
  }

  if (found == 0 && (current + size + 2 * sizeof(size_t)) >
                        (allocator->start_ptr + allocator->size)) {
    return 0; // Cannot allocate more than the space that is left
  }

  if (found == 0) {
    *(size_t *)current = size;
    current += sizeof(size_t);
    *(size_t *)current = 1;
    current += sizeof(size_t);
    allocator->last_ptr = current + size;
    return current;
  } else {
    current += sizeof(size_t);
    *(size_t *)current = 1;
    current += sizeof(size_t);
    return current;
  }
}

int alloc_free(Allocator *allocator, void *ptr) {
  if (ptr < allocator->start_ptr + 2 * sizeof(size_t) ||
      ptr >= allocator->last_ptr) {
    return 1;
  }
  Header header = _parse_header(ptr);
  if (header.allocated == 0) {
    return 1;
  } else {
    ptr -= sizeof(size_t);
    *(size_t *)ptr = 0;
    return 0;
  }
}

int alloc_reset(Allocator *allocator) {
  if (allocator->start_ptr == 0)
    return 1;
  allocator->last_ptr = allocator->start_ptr;
  return 0;
}
