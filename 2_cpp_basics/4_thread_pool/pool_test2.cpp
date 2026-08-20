#include <cstdint>
#include <stdexcept>
// Goal: thread safety only. Focus is not optimal
// and performant queue, but is the thread pool

#include <future>
#include <mutex>
#include <optional>
#include <queue>
#include <stop_token>
#include <thread>
#include <tuple>

template <typename T> class th_sf_queue {
  std::mutex mutex;
  std::queue<T> queue;

public:
  th_sf_queue() = default;
  ~th_sf_queue() = default;
  bool enqueue(T data) {
    std::lock_guard<std::mutex> lk(mutex);
    queue.push(std::move(data));
    return true;
  }
  std::optional<T> dequeue() {
    std::lock_guard<std::mutex> lk(mutex);
    if (queue.empty()) {
      return std::nullopt;
    }
    auto res = std::move(queue.front());
    queue.pop();
    return res;
  }
  size_t queue_len() {
    std::lock_guard<std::mutex> lk(mutex);
    return queue.size();
  }
};

template <size_t S> class thread_pool {
  std::jthread threads[S];
  // task = (result promise, function, argument passed to the function)
  th_sf_queue<std::tuple<std::promise<void *>, void *(*)(void *), void *>>
      promises[S];
  size_t next_queue;

public:
  thread_pool() {
    int counter = 0;
    for (auto &t : threads) {
      t = std::jthread(
          [this, i = counter](std::stop_token st) { thread_handler(st, i); });
      counter++;
    }
    next_queue = 0;
  }
  ~thread_pool() {
    for (auto &t : threads)
      t.request_stop();
  }

  std::future<void *> schedule_job(void *(*func)(void *), void *args) {
    std::promise<void *> promise;
    std::future<void *> future = promise.get_future();
    std::tuple<std::promise<void *>, void *(*)(void *), void *> tup = {
        std::move(promise), func, args};
    promises[next_queue].enqueue(std::move(tup));
    next_queue = (next_queue + 1) % S;
    return future;
  }

private:
  void thread_handler(std::stop_token st, int idx) {
    while (!st.stop_requested()) {
      auto task = promises[idx].dequeue();
      // own queue empty: try stealing from the others
      for (size_t i = 0; i != S && !task; i++)
        task = promises[i].dequeue();
      if (!task)
        continue;

      auto &[p, fn, args] = *task;
      try {
        p.set_value(fn(args));
      } catch (...) {
        p.set_exception(std::current_exception());
      }
    }
  }
};

void *twice(void *a) { return (void *)((uintptr_t)a * 2); }
void *boom(void *) { throw std::runtime_error("boom"); }
int main() {
  thread_pool<4> pool;
  std::future<void *> f[100];
  for (uintptr_t i = 0; i < 100; i++) f[i] = pool.schedule_job(twice, (void *)i);
  for (uintptr_t i = 0; i < 100; i++) if ((uintptr_t)f[i].get() != i * 2) return 1;
  auto fe = pool.schedule_job(boom, nullptr);
  try { fe.get(); return 2; } catch (const std::runtime_error &) {}
  return 0;
}
