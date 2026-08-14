#include <condition_variable>
#include <mutex>
#include <optional>
#include <stack>

template <typename T> class stack_s {
  std::stack<T> stack;
  std::condition_variable conditional;
  std::mutex mutex;

public:
  stack_s() = default;
  bool empty();
  size_t size();
  void push(T element);
  T pop();
  std::optional<T> try_pop();
};

template <typename T> bool stack_s<T>::empty() {
  std::lock_guard<std::mutex> lock(mutex);
  return stack.empty();
}

template <typename T> size_t stack_s<T>::size() {
  std::lock_guard<std::mutex> lock(mutex);
  return stack.size();
}

template <typename T> void stack_s<T>::push(T element) {
  std::lock_guard<std::mutex> lock(mutex);
  stack.push(std::move(element));
  conditional.notify_one();
}

template <typename T> T stack_s<T>::pop() {
  std::unique_lock<std::mutex> lock(mutex);
  conditional.wait(lock, [&]() { return stack.size() > 0; });
  T value = std::move(stack.top());
  stack.pop();
  return value;
}

template <typename T> std::optional<T> stack_s<T>::try_pop() {
  std::lock_guard<std::mutex> lock(mutex);
  if (stack.empty())
    return std::nullopt;
  T value = std::move(stack.top());
  stack.pop();
  return value;
}
