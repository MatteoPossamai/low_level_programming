#ifndef ALLOCATOR_H
#define ALLOCATOR_H
#include <stddef.h>
#define ALIGN 16
typedef struct {
    char* buffer;
    char* ptr;
    size_t buff_size;
} Arena;

int   alloc_init(Arena* arena, size_t size);
int   alloc_deinit(Arena* arena);
void* alloc_malloc(Arena* arena, size_t size);
int   alloc_free(Arena* arena, void* ptr);
int   alloc_reset(Arena* arena);

#endif