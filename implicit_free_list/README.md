# Implicit List Allocator

Baseline from [prev Readme](/bump_allocator/README.md). Same API and
basic allocation mechanisms.

## Instruction

Implicit free-list allocator

- Replace bump with real free-list
- Headers in front of each block (size + allocated bit)
- First-fit search
- Implement actual free()
- Done when: can run all API functions without leaks; stress-tested with random patterns

## Structures -- DIFF

```c
typedef struct {
  size_t block_size;
  size_t allocated;
} Header;

typedef struct {
  void *start_ptr;
  void *last_ptr;
  size_t size;
} Allocator;
```

Changed allocator to use `void *`, and added a marker for the last
seen segment.

Created `Header` to save structure of the pre-data segment that
tells you size and if allocated, about the following data segment.

## Mechanism -- DIFF

This time we have an array, and we prepend to each data segment
a header, that is nothing but a tuple of two `size_t` telling
how much space is taken and if allocated. No compaction, just
linearly look at the next one and check for a new big enough
location. First fit algorithm. Using the `->last_ptr` as the
barrier.

## Learning

- To assign to a `void *` you need to cast the pointer, not the value:

```c
    *(size_t *)ptr = 0;
```
