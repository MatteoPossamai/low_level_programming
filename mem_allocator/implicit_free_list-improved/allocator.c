#include "allocator.h"
#include <stdio.h>
#include <sys/mman.h>

int alloc_init(Allocator *allocator, size_t size) {
  if (allocator->meta_start_ptr != 0) {
    return 1; // Invalid if the memory is already initialized. Need
              // de-initialization
  }
  allocator->meta_start_ptr =
      mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (allocator->meta_start_ptr == MAP_FAILED) {
    perror("mmap");
    return 1;
  }
  allocator->data_start_ptr =
      mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (allocator->data_start_ptr == MAP_FAILED) {
    perror("mmap");
    munmap(allocator->meta_start_ptr, size);
    allocator->meta_start_ptr = 0;
    return 1;
  }
  allocator->meta_last_ptr = allocator->meta_start_ptr;
  allocator->size = size;
  return 0;
}

int alloc_deinit(Allocator *allocator) {
  if (allocator->meta_start_ptr == 0) {
    return 1; // Cannot de-init if never initialized
  }
  munmap(allocator->meta_start_ptr, allocator->size);
  munmap(allocator->data_start_ptr, allocator->size);
  allocator->meta_start_ptr = 0;
  allocator->data_start_ptr = 0;
  allocator->meta_last_ptr = 0;
  allocator->size = 0;
  return 0;
}

// Meta region entry (3 words): [block_size, allocated, payload_ptr].
// Data region block: 16-byte header [padding, meta_entry_ptr] then payload,
// so payloads stay 16-byte aligned and free() can find the meta entry.
void *alloc_malloc(Allocator *allocator, size_t size) {
  if (allocator->meta_start_ptr == 0)
    return 0;
  // Round up so every block size is a multiple of ALIGN; header is 16 bytes,
  // so all headers and payloads stay 16-byte aligned.
  size = (size % ALIGN == 0) ? size : size + (ALIGN - (size % ALIGN));
  void *current = allocator->meta_start_ptr;
  while (current < allocator->meta_last_ptr) {
    size_t buffer_size = *(size_t *)current;
    size_t allocated = *(size_t *)(current + sizeof(size_t));

    if (allocated == 0 && buffer_size >= size) {
      *(size_t *)(current + sizeof(size_t)) = 1; // Mark as allocated
      return *(void **)(current + 2 * sizeof(size_t));
    }
    current += 3 * sizeof(size_t);
  }
  // No reusable block: append after the last block in the data region.
  void *block_start = NULL;
  if (allocator->meta_start_ptr == allocator->meta_last_ptr) {
    block_start = allocator->data_start_ptr;
  } else {
    void *prev = allocator->meta_last_ptr - 3 * sizeof(size_t);
    block_start = *(void **)(prev + 2 * sizeof(size_t)) + *(size_t *)prev;
  }
  if (block_start + 2 * sizeof(size_t) + size >
      allocator->data_start_ptr + allocator->size) {
    return 0; // Data region full
  }
  if (allocator->meta_last_ptr + 3 * sizeof(size_t) >
      allocator->meta_start_ptr + allocator->size) {
    return 0; // Meta region full
  }
  void *payload = block_start + 2 * sizeof(size_t);
  *(size_t *)allocator->meta_last_ptr = size;
  *(size_t *)(allocator->meta_last_ptr + sizeof(size_t)) = 1;
  *(void **)(allocator->meta_last_ptr + 2 * sizeof(size_t)) = payload;
  *(void **)(payload - sizeof(size_t)) = allocator->meta_last_ptr;
  allocator->meta_last_ptr += 3 * sizeof(size_t);
  return payload;
}

int alloc_free(Allocator *allocator, void *ptr) {
  if (allocator->meta_start_ptr == 0)
    return 1;
  if (ptr < allocator->data_start_ptr + 2 * sizeof(size_t) ||
      ptr >= allocator->data_start_ptr + allocator->size)
    return 1;
  void *header = *(void **)(ptr - sizeof(size_t));
  if (header < allocator->meta_start_ptr ||
      header >= allocator->meta_last_ptr ||
      (header - allocator->meta_start_ptr) % (3 * sizeof(size_t)) != 0)
    return 1;
  if (*(void **)(header + 2 * sizeof(size_t)) != ptr)
    return 1; // Header does not describe this pointer
  if (*(size_t *)(header + sizeof(size_t)) == 0)
    return 1; // Double free
  *(size_t *)(header + sizeof(size_t)) = 0;
  return 0;
}

int alloc_reset(Allocator *allocator) {
  if (allocator->meta_start_ptr == 0)
    return 1;
  allocator->meta_last_ptr = allocator->meta_start_ptr;
  return 0;
}
