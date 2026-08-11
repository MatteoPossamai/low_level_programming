#include <cstddef>
#define ALIGN 16

class Arena {
  size_t size;
  void *start_ptr;
  void *curr_ptr;

public:
  Arena(size_t size);
  template <typename T> auto alloc(size_t count) -> T *;
  auto dealloc(void *ptr) -> size_t;
  auto reset() -> size_t;
  ~Arena();
};

template <typename T> auto Arena::alloc(size_t count) -> T * {
  size_t bytes = count * sizeof(T);
  size_t aligned = (bytes % ALIGN == 0) ? bytes : bytes + (ALIGN - (bytes % ALIGN));
  if ((char *)curr_ptr + aligned > (char *)start_ptr + size) {
    return nullptr;
  }
  void *res = this->curr_ptr;
  this->curr_ptr = (char *)this->curr_ptr + aligned;
  return (T *)res;
}
