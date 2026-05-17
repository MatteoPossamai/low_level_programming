#include <sys/mman.h>
#include <stdio.h>

char* buffer;
char* ptr = 0;
size_t buff_size;


int alloc_init(size_t size){
    if (ptr != 0){
        return 1; // Invalid if the memory is already initialized. Need de-initialization
    }
    buffer= mmap(0,size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buffer == MAP_FAILED) { perror("mmap"); return 1; }
    ptr = buffer;
    buff_size = size;
    return 0;
}
 
int alloc_deinit(){
    if (ptr == 0){
        return 1; // Cannot de-init if never initialized
    }
    munmap(buffer, buff_size);
    ptr = 0;
    buff_size = 0;
    return 0;
}

void * alloc_malloc(size_t size){
    if (ptr == 0){
        return 0; // Need initialized memory
    }
    if (ptr + size > buffer + buff_size) {
        return 0; // Out of memory
    }
    ptr += size;
    return ptr - size;
}


int alloc_free(void *){ }