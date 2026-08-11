#ifndef ALLOCATOR_H
#define ALLOCATOR_H
#include <stddef.h>
#define ALIGN 16
typedef struct {
  char *buffer;
  char *ptr;
  size_t buff_size;
} Allocator;

int alloc_init(Allocator *arena, size_t size);
int alloc_deinit(Allocator *arena);
void *alloc_malloc(Allocator *arena, size_t size);
int alloc_free(Allocator *arena, void *ptr);
int alloc_reset(Allocator *arena);

#endif
