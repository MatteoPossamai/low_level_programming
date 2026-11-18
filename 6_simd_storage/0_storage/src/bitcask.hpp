#include <array>
#include <cstring>
#include <fstream>
#include <ios>
#include <map>
#include <optional>
#include <string>

template <typename K_t, typename V_t> class Bitcask {
  static_assert(std::is_trivially_copyable_v<K_t>);
  static_assert(std::is_trivially_copyable_v<V_t>);

  static constexpr size_t KEY_SIZE = sizeof(K_t);
  static constexpr size_t VAL_SIZE = sizeof(V_t);
  std::array<char, KEY_SIZE + VAL_SIZE> buffer{};
  size_t end_ptr;

  std::fstream file;
  std::map<K_t, size_t>
      index; // Balanced BST, like RB. Already sorted inherently

public:
  Bitcask() = delete;
  explicit Bitcask(std::string filename) {
    file.open(filename, std::ios::in | std::ios::out | std::ios::binary |
                            std::fstream::app);
    if (!file.is_open())
      throw std::ios_base::failure("Cannot open database file: " + filename);
    size_t counter = 0;

    while (
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()))) {
      K_t key;
      std::memcpy(&key, buffer.data(), KEY_SIZE);
      index[key] = (KEY_SIZE + VAL_SIZE) * counter;
      ++counter;
    }

    file.clear();
    file.seekg(0);                // Move read pointer to start
    file.seekp(0, std::ios::end); // Move write pointer to end
    end_ptr = counter;
  }

  Bitcask(const Bitcask &src) = delete;
  Bitcask(Bitcask &&src) = delete;
  Bitcask &operator=(const Bitcask &rhs) = delete;
  Bitcask &operator=(Bitcask &&rhs) = delete;
  ~Bitcask() {
    if (file.is_open())
      file.close();
  }

  // API
  size_t put(K_t key, V_t value) {
    file.write(reinterpret_cast<const char *>(&key), KEY_SIZE);
    file.write(reinterpret_cast<const char *>(&value),VAL_SIZE);
    index[key] = end_ptr * (KEY_SIZE + VAL_SIZE);
    ++end_ptr;
    return 0;
  }

  size_t del(K_t key) { return put(key, static_cast<V_t>(0)); }

  std::optional<V_t> get(K_t key) {
    if (!index.contains(key)) {
      return {};
    }
    size_t offset = index[key];
    file.seekg(static_cast<std::streamsize>(offset));
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    V_t value;
    std::memcpy(&value, buffer.data() + KEY_SIZE, VAL_SIZE);
    if (value == static_cast<V_t>(0)) {
      return {};
    }
    return value;
  }
};
