#include "bitcask.hpp"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <map>
#include <random>

using K_T = size_t;
using V_T = double_t;

int main() {
  // Part 1
  {
    Bitcask<K_T, V_T> bitcask{"./data.bin"};
    std::map<K_T, V_T> checker{};

    bitcask.put(10, 10);
    checker[10] = 10;

    assert(bitcask.get(10) == checker[10]);
    assert(!bitcask.get(20).has_value());

    bitcask.put(10, 20);
    checker[10] = 20;
    assert(bitcask.get(10) == checker[10]);

    checker.erase(10);
    bitcask.del(10);
    assert(!bitcask.get(10).has_value());

    bitcask.put(11, 20);
    assert(bitcask.get(11) == 20);
  }

  // Part 2: restart
  Bitcask<K_T, V_T> bitcask{"./data.bin"};
  assert(!bitcask.get(10).has_value());
  assert(bitcask.get(11) == 20);
  std::filesystem::remove("./data.bin");

  // Part 3: random operations
  std::filesystem::remove("bitcask_random_test.bin");
  {
    Bitcask<K_T, V_T> random_store{"bitcask_random_test.bin"};
    std::map<K_T, V_T> checker;
    std::mt19937 rng{42};
    std::uniform_int_distribution<int> operations{0, 2};
    std::uniform_int_distribution<K_T> keys{0, 99};
    std::uniform_int_distribution<int> values{1, 1000};

    for (int i = 0; i < 200; ++i) {
      K_T key = keys(rng);
      switch (operations(rng)) {
      case 0: {
        V_T value = values(rng);
        random_store.put(key, value);
        checker[key] = value;
        break;
      }
      case 1:
        random_store.del(key);
        checker.erase(key);
        break;
      case 2:
        break;
      }

      auto actual = random_store.get(key);
      auto expected = checker.find(key);
      if (expected == checker.end())
        assert(!actual.has_value());
      else
        assert(actual == expected->second);
    }
  }
  std::filesystem::remove("bitcask_random_test.bin");

  return 0;
}
