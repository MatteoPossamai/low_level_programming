#include "allocator.h"
#include <stdio.h>
#include <sys/mman.h>

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
  *(size_t *)allocator->start_ptr = (size_t)0;
  return 0;
}

int alloc_reset(Allocator *allocator) {
  if (allocator->start_ptr == 0)
    return 1;
  allocator->last_ptr = allocator->start_ptr;
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
  size = (size % ALIGN == 0) ? size : size + (ALIGN - (size % ALIGN));
  Header *header;
  if (HEAD != NULL) {
    void *current = HEAD;
    size_t found = 0;
    do {
      header = (Header *)current;
      if (header->block_size >= size) {
        found = 1;
      } else {
        // Go to next
        *(size_t *)current = *(size_t *)(current + 3 * sizeof(size_t));
      }
    } while (current != HEAD && found == 0);

    if (found != 0) {
      void *prev = current + 2 * sizeof(size_t);
      void *next = current + 3 * sizeof(size_t);
      header = (Header *)current;

      header->allocated = 1;
      if (header->block_size >= size + 5 * sizeof(size_t)) {
        size_t original_size = header->block_size;
        header->block_size = size;                               // header
        *(size_t *)(current + size + 2 * sizeof(size_t)) = size; // footer

        void *other_chunk = current + size + 3 * sizeof(size_t);
        Header *new_header = (Header *)other_chunk;
        new_header->allocated = 0;
        new_header->block_size = original_size - size - 3 * sizeof(size_t);
        *(size_t *)(other_chunk + new_header->block_size + 2 * sizeof(size_t)) =
            new_header->block_size;
        void *n_prev = other_chunk + 2 * sizeof(size_t);
        void *n_next = other_chunk + 3 * sizeof(size_t);
        *(size_t *)n_prev = *(size_t *)prev;
        *(size_t *)n_next = *(size_t *)next;
      } else if (*(size_t *)prev == *(size_t *)next) {
        HEAD = NULL;
      } else {
        size_t *prev_block_next = *(size_t **)prev;
        size_t *next_block_prev = *(size_t **)next;
        *prev_block_next = *(size_t *)next;
        *next_block_prev = *(size_t *)prev;
      }
      return current + 2 * sizeof(size_t);
    }
  }
  if (allocator->last_ptr + 3 * sizeof(size_t) + size >
      allocator->start_ptr + allocator->size) {
    return 0; // Not more space left
  }
  header = (Header *)allocator->last_ptr;
  header->allocated = 1;
  header->block_size = size;
  void *result = allocator->last_ptr + 2 * sizeof(size_t);
  *(size_t *)(allocator->last_ptr + 2 * sizeof(size_t) + size) = size;
  allocator->last_ptr += size + 3 * sizeof(size_t);
  return result;
}

int alloc_free(Allocator *allocator, void *ptr) {
  if (ptr < allocator->start_ptr + 2 * sizeof(size_t) ||
      ptr >= allocator->last_ptr) {
    return 1;
  }
  Header *header = (Header *)(ptr - 2 * sizeof(size_t));
  if (header->allocated == 0) {
    return 1;
  }

  size_t size = header->block_size;

  // Coal front: absorb next block, unlink it from the free list
  void *next_hdr = ptr + size + sizeof(size_t);
  if (next_hdr < allocator->last_ptr && ((Header *)next_hdr)->allocated == 0) {
    void *n_prev = *(void **)(next_hdr + 2 * sizeof(size_t));
    void *n_next = *(void **)(next_hdr + 3 * sizeof(size_t));
    if (n_next == next_hdr) {
      HEAD = NULL; // was the only node in the list
    } else {
      *(void **)(n_prev + 3 * sizeof(size_t)) = n_next; // prev block's next
      *(void **)(n_next + 2 * sizeof(size_t)) = n_prev; // next block's prev
      if (HEAD == next_hdr)
        HEAD = n_next;
    }
    size += ((Header *)next_hdr)->block_size + 3 * sizeof(size_t);
  }

  // Coal back: previous block's footer sits right before our header
  if ((void *)header > allocator->start_ptr) {
    size_t prev_size = *(size_t *)(ptr - 3 * sizeof(size_t));
    void *prev_hdr = ptr - 5 * sizeof(size_t) - prev_size;
    if (((Header *)prev_hdr)->allocated == 0) {
      void *p_prev = *(void **)(prev_hdr + 2 * sizeof(size_t));
      void *p_next = *(void **)(prev_hdr + 3 * sizeof(size_t));
      if (p_next == prev_hdr) {
        HEAD = NULL;
      } else {
        *(void **)(p_prev + 3 * sizeof(size_t)) = p_next;
        *(void **)(p_next + 2 * sizeof(size_t)) = p_prev;
        if (HEAD == prev_hdr)
          HEAD = p_next;
      }
      size += prev_size + 3 * sizeof(size_t);
      header = (Header *)prev_hdr;
    }
  }

  header->allocated = 0;
  header->block_size = size;
  void *hdr = (void *)header;
  *(size_t *)(hdr + 2 * sizeof(size_t) + size) = size; // merged footer

  // Push merged block at the head of the circular free list
  if (HEAD == NULL) {
    *(void **)(hdr + 2 * sizeof(size_t)) = hdr;
    *(void **)(hdr + 3 * sizeof(size_t)) = hdr;
  } else {
    void *tail = *(void **)(HEAD + 2 * sizeof(size_t));
    *(void **)(hdr + 2 * sizeof(size_t)) = tail;
    *(void **)(hdr + 3 * sizeof(size_t)) = HEAD;
    *(void **)(tail + 3 * sizeof(size_t)) = hdr;
    *(void **)(HEAD + 2 * sizeof(size_t)) = hdr;
  }
  HEAD = hdr;
  return 0;
}
