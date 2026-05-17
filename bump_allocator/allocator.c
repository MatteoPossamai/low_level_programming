#include <sys/mman.h>
#include <stdio.h>
#include "allocator.h"

int alloc_init(Arena* arena, size_t size){
    if (arena->ptr != 0){
        return 1; // Invalid if the memory is already initialized. Need de-initialization
    }
    arena->buffer= mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (arena->buffer == MAP_FAILED) { perror("mmap"); return 1; }
    arena->ptr = arena->buffer;
    arena->buff_size = size;
    return 0;
}
 
int alloc_deinit(Arena* arena){
    if (arena->ptr == 0){
        return 1; // Cannot de-init if never initialized
    }
    munmap(arena->buffer, arena->buff_size);
    arena->ptr = 0;
    arena->buff_size = 0;
    return 0;
}

void * alloc_malloc(Arena* arena, size_t size){
    if (arena->ptr == 0){
        return 0; // Need initialized memory
    }
    size_t aligned = (size % ALIGN == 0) ? size : size + (ALIGN - (size % ALIGN));
    if (aligned > (size_t)(arena->buffer + arena->buff_size - arena->ptr)){
        return 0; // Out of memory
    }
    char* res = arena->ptr;
    arena->ptr += aligned;
    return (void*)res;
}

int alloc_free(Arena* arena, void* ptr) { (void)arena; (void)ptr; return 0; }
int alloc_reset(Arena* arena) {
       if (arena->ptr == 0) return 1;
       arena->ptr = arena->buffer;
       return 0;
   }