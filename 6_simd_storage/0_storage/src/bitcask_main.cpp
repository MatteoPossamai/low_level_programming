#include "bitcask.hpp"
#include <cassert>
#include <cmath>
#include <cstddef>
#include <map>

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

  return 0;
}
