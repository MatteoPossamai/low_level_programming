#ifndef ALLOCATOR_H
#define ALLOCATOR_H
#include <stddef.h>

#define ALIGN 16

typedef struct {
  void *meta_start_ptr;
  void *meta_last_ptr;
  void *data_start_ptr;
  size_t size;
} Allocator;

int alloc_init(Allocator *allocator, size_t size);
int alloc_deinit(Allocator *allocator);
void *alloc_malloc(Allocator *allocator, size_t size);
int alloc_free(Allocator *allocator, void *ptr);
int alloc_reset(Allocator *allocator);

#endif
