# Bump Allocator - Arena

Simple allocator for Arena style allocation.

## Structure

Based off of the structure:

```c
typedef struct {
    char* buffer;
    char* ptr;
    size_t buff_size;
} Arena;
```

To keep track in the allocator of the buffer size and the dimension. Use `char*` so that each block has a one byte jump on every sum operation. Since we use alignment to avoid CPU slowdown, could have just used that property and could have made it more easy to read, but prefer this approach that is closer to the mental model of pointer arithmetic and gives more freedom in case we do NOT want to have alignment. That is what was originally done. 

## API

```c
int   alloc_init(Arena* arena, size_t size);
int   alloc_deinit(Arena* arena);
void* alloc_malloc(Arena* arena, size_t size);
int   alloc_free(Arena* arena, void* ptr);
int   alloc_reset(Arena* arena);
```

## Mechanism

An arena is an area allocated with `mmap` that is then used to get pointers. When we reach the end of the area size, we cannot reuse it anymore. We can only de-initialize or reset it, losing all of the data. This is likely the fastest approach since is just appending at the end of a "file", but is very limiting in terms of memory waste, and usability of memory.

## Learning

- Alignment is something that we need to take care of manually in memory allocators if we care about it;
- `mmap` function requires all the flag to be initialized correctly, or later accecss will cause `Segmentation fault`;
- `.h` files and `ifndefine` are really useful and powerful to avoid linker errors and use the same structure in different files;
- Initialization is very important to get right. Some errors might not be exposed by the compiler, or might be cryptic;