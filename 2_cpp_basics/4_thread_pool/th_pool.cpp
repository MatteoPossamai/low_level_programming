// Goal: thread safety only. Focus is not optimal
// and performant queue, but is the thread pool

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <future>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <stop_token>
#include <thread>
#include <tuple>
#include <vector>

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
  std::atomic<size_t> next_queue = 0;
  // condition_variable_any (not condition_variable): its wait() overload
  // taking a stop_token is what lets request_stop() wake sleeping workers
  std::condition_variable_any cv;
  std::mutex cv_mutex;

public:
  thread_pool() {
    int counter = 0;
    for (auto &t : threads) {
      t = std::jthread(
          [this, i = counter](std::stop_token st) { thread_handler(st, i); });
      counter++;
    }
  }
  ~thread_pool() {
    for (auto &t : threads)
      t.request_stop();
    // join here, not in jthread's destructor: members declared after
    // `threads` (queues, cv) are destroyed first, so workers must be
    // gone before this destructor body ends
    for (auto &t : threads)
      t.join();
  }

  std::future<void *> schedule_job(void *(*func)(void *), void *args) {
    std::promise<void *> promise;
    std::future<void *> future = promise.get_future();
    std::tuple<std::promise<void *>, void *(*)(void *), void *> tup = {
        std::move(promise), func, args};
    size_t q = next_queue.fetch_add(1, std::memory_order_relaxed) % S;
    promises[q].enqueue(std::move(tup));
    {
      std::lock_guard<std::mutex> lk(cv_mutex);
      cv.notify_all();
    }
    return future;
  }

private:
  void thread_handler(std::stop_token st, int idx) {
    while (!st.stop_requested()) {
      auto task = promises[idx].dequeue();
      // own queue empty: try stealing from the others
      for (size_t i = 0; i != S && !task; i++)
        task = promises[i].dequeue();
      if (!task) {
        std::unique_lock<std::mutex> lk(cv_mutex);
        cv.wait(lk, st, [this] {
          for (auto &q : promises)
            if (q.queue_len() > 0)
              return true;
          return false;
        });
        continue;
      }

      auto &[p, fn, args] = *task;
      try {
        p.set_value(fn(args));
      } catch (...) {
        p.set_exception(std::current_exception());
      }
    }
  }
};

void *square(void *arg) {
  auto v = reinterpret_cast<uintptr_t>(arg);
  return reinterpret_cast<void *>(v * v);
}

int main() {
  thread_pool<4> pool;

  std::vector<std::future<void *>> results;
  for (uintptr_t i = 1; i <= 16; i++)
    results.push_back(pool.schedule_job(square, reinterpret_cast<void *>(i)));

  for (uintptr_t i = 0; i < results.size(); i++) {
    auto r = reinterpret_cast<uintptr_t>(results[i].get());
    std::cout << (i + 1) << "^2 = " << r << '\n';
  }
  return 0;
}
