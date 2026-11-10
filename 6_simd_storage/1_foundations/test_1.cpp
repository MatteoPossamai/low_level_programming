#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

// Keep the reduction in a separate function so the vectorization report and
// assembly show the loop we're studying.
int sum(const int *values, std::size_t size) {
  int total = 0;
  for (std::size_t i = 0; i < size; ++i) {
    total += values[i];
  }
  return total;
}

int main() {
  constexpr std::size_t size = 1'000'000;
  std::mt19937 generator(std::random_device{}());
  std::uniform_int_distribution<int> distribution(0, 100);

  std::vector<int> values(size);
  for (int &value : values) {
    value = distribution(generator);
  }

  std::cout << sum(values.data(), values.size()) << '\n';
}
