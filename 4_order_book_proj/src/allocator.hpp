#pragma once

#include <algorithm>
#include <cstddef>
#include <new>
#include <sys/mman.h>

template <typename T, size_t Size> class Allocator {
  struct Pointer {
    Pointer *next;
  };

  void *buffer;
  Pointer *free_head; // head of free list

  static constexpr size_t used_size = std::max(sizeof(T), sizeof(Pointer));

public:
  Allocator() {
    static_assert(Size > 0);
    buffer = mmap(nullptr, Size * used_size, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (buffer == MAP_FAILED)
      throw std::bad_alloc();

    free_head = static_cast<Pointer *>(buffer);
    char *current = static_cast<char *>(buffer) + (Size - 1) * used_size;
    reinterpret_cast<Pointer *>(current)->next = nullptr;
    while (current > static_cast<char *>(buffer)) {
      current -= used_size;
      reinterpret_cast<Pointer *>(current)->next =
          reinterpret_cast<Pointer *>(current + used_size);
    }
  }

  ~Allocator() { munmap(buffer, Size * used_size); }

  Allocator(const Allocator &) = delete;
  Allocator &operator=(const Allocator &) = delete;
  Allocator(Allocator &&) = delete;
  Allocator &operator=(Allocator &&) = delete;

  T *allocate() {
    if (full())
      return nullptr;

    Pointer *head = free_head;
    free_head = head->next;
    return reinterpret_cast<T *>(head);
  }

  void deallocate(T *pointer) {
    auto *node = reinterpret_cast<Pointer *>(pointer);
    node->next = free_head;
    free_head = node;
  }

  bool full() { return free_head == nullptr; }
};
