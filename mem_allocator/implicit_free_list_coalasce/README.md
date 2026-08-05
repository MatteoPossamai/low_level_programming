# Implicit free list Coalasce

Baseline from [prev Readme](/implicit_free_list/README.md). Same API,
basic allocation mechanisms and structures.

## Instructions

Coalescing + boundary tags

- Add immediate coalescing of adjacent free blocks
- Add boundary tags (footer with size) for O(1) coalescing-with-previous
- Done when: heap doesn't fragment into uselessness on mixed workload

## Mechanism -- DIFF

Added footer as another end of the memory. This makes allocation and coalescing
faster and simpler rather than parsing again the entire allocator, but wastes
more memory, in addition to header.

This time need to do coalescing. Need to coalesce only next and prev, since if
you run on the previous cases, then it should be already contiguous and you just
need one in constant time for both directions. Is much pointer arithmetics.

## Block layout

Every block, allocated or free (w = `sizeof(size_t)` = 8):

```
| size (w) | allocated (w) | payload (size bytes) | footer = size (w) |
^ header                   ^ user pointer
```

Overhead per block: 24 bytes. From a user pointer `ptr`:

- own size:        `*(ptr - 2w)`
- own alloc flag:  `*(ptr - 1w)`
- prev footer:     `*(ptr - 3w)`  -> gives prev size
- prev alloc flag: `*(ptr - 4w - prev_size)`
- next header:     `ptr + size + 1w`

## Learning

- For coalesce-ing just need to do the next one and prev, since you keep this
  true, so just those are to check
