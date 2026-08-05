# Explicit free list

Baseline from [prev Readme](/implicit_free_list_coalasce/README.md). Same API,
same block layout (header + footer boundary tags), same coalescing.

## Instructions

Explicit free list

- Keep blocks as before, but link the free blocks in a doubly-linked list
- `malloc` searches only free blocks, not every block in the heap
- free unlinks coalesced neighbors and pushes the merged block on the list
- Done when: search cost depends on number of free blocks, not total blocks

## Structures -- DIFF

```c
static void *HEAD = NULL; // in allocator.c, not the header

typedef struct {
  void *prev;
  void *next;
} Pointers;
```

`HEAD` points at the header of some free block, or `NULL` when no free
blocks exist. It lives in allocator.c: `static` in a .h gives every
file that includes it a private copy, which is a bug waiting to happen.

`Pointers` are stored **inside the payload** of free blocks (first two
words). Allocated blocks don't need them, so this costs zero extra
memory - the minimum payload (ALIGN = 16) is exactly two pointers.

## Block layout -- DIFF

Same as before, plus prev/next when free (w = `sizeof(size_t)` = 8):

```
allocated: | size (w) | alloc=1 (w) | payload (size bytes)      | footer (w) |
free:      | size (w) | alloc=0 (w) | prev (w) | next (w) | ... | footer (w) |
           ^ header, list pointers reference this address
```

## Mechanism -- DIFF

The free list is circular and doubly linked. Free pushes at `HEAD`
(LIFO), `malloc` walks from `HEAD` first-fit until back at `HEAD`.

New bookkeeping compared to implicit list:

- malloc, block reused whole: unlink it (splice prev/next together,
  move `HEAD` if it pointed there, `NULL` if it was the only node)
- malloc, block split: the remainder replaces the old node in the list,
  so both neighbors must be re-pointed at the remainder's header
- free: coalesced neighbors were free, so they sit **in the list** and
  must be unlinked before merging; the merged block is inserted once
- deinit/reset: `HEAD = NULL`, or it dangles into unmapped memory

Node is "the only one" when `next == current` (points to itself). Not
when `prev == next`: in a 2-node list every node has prev == next (the
other node), and that check silently drops the whole list.

## Learning

- Crash site != bug site. Segfault reported in the split path, actual
  bug was the traversal writing the next pointer _into the header_
  (`*(size_t *)current = ...`) instead of advancing (`current = ...`).
- `static` in `.h` file makes it private so the `.c` and `test.c` would
  create their own copy, that is not desirable
