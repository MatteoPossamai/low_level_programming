#include <future>
#include <iostream>

int compute() {
    return 42;
}

int main() {
    // Runs asynchronously on a separate thread
    auto f1 = std::async(std::launch::async, compute);

    // Runs only when get() or wait() is called
    auto f2 = std::async(std::launch::deferred, compute);

    // Implementation chooses async or deferred
    auto f3 = std::async(compute);

    std::cout << f1.get() << '\n';
    std::cout << f2.get() << '\n';
    std::cout << f3.get() << '\n';
}
