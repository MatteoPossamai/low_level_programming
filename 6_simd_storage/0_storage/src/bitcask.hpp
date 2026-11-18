#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <ios>
#include <map>
#include <optional>
#include <string>
#include <unistd.h>
#include <zlib.h>

template <typename K_t, typename V_t> class Bitcask {
  static_assert(std::is_trivially_copyable_v<K_t>);
  static_assert(std::is_trivially_copyable_v<V_t>);

  static constexpr size_t KEY_SIZE = sizeof(K_t);
  static constexpr size_t VAL_SIZE = sizeof(V_t);
  static constexpr size_t RECORD_SIZE = KEY_SIZE + VAL_SIZE + 4;
  std::array<char, RECORD_SIZE> buffer{};
  size_t end_ptr;

  int fd = -1;
  std::map<K_t, size_t>
      index; // Balanced BST, like RB. Already sorted inherently

  uint32_t checksum(const K_t &key, const V_t &value) {
    std::array<char, KEY_SIZE + VAL_SIZE> buff{};
    std::memcpy(buff.data(), &key, KEY_SIZE);
    std::memcpy(buff.data() + KEY_SIZE, &value, VAL_SIZE);

    return static_cast<uint32_t>(
        crc32(0L, reinterpret_cast<const Bytef *>(buff.data()), buff.size()));
  }

public:
  Bitcask() = delete;
  explicit Bitcask(std::string filename) {
    fd = ::open(filename.c_str(), O_RDWR | O_CREAT | O_APPEND, 0600);
    if (fd == -1)
      throw std::ios_base::failure("Cannot open database file: " + filename);
    size_t counter = 0;

    try {
      while (true) {
        ssize_t read_b = ::read(fd, buffer.data(), buffer.size());

        if (read_b < static_cast<ssize_t>(buffer.size())) {
          if (read_b == -1) {
            throw std::ios_base::failure("Cannot read database file: " +
                                         filename);
          }
          if (read_b > 0) {
            std::filesystem::resize_file(filename, counter * RECORD_SIZE);
          }
          break;
        }

        K_t key;
        V_t value;
        uint32_t checksum_v;
        std::memcpy(&key, buffer.data(), KEY_SIZE);
        std::memcpy(&value, buffer.data() + KEY_SIZE, VAL_SIZE);
        std::memcpy(&checksum_v, buffer.data() + KEY_SIZE + VAL_SIZE,
                    sizeof(uint32_t));

        if (checksum_v != checksum(key, value)) { // Corrupted CRC
          throw std::ios_base::failure("Detected corruption in file" + filename);
          // Middle transaction was wrong. File is corrupted in the middle
        }

        index[key] = RECORD_SIZE * counter;
        ++counter;
      }
    } catch (...) {
      ::close(fd);
      throw;
    }

    end_ptr = counter;
  }

  Bitcask(const Bitcask &src) = delete;
  Bitcask(Bitcask &&src) = delete;
  Bitcask &operator=(const Bitcask &rhs) = delete;
  Bitcask &operator=(Bitcask &&rhs) = delete;
  ~Bitcask() {
    if (fd != -1)
      ::close(fd);
  }

  // API
  size_t put(K_t key, V_t value) {
    uint32_t checksum_v = checksum(key, value);
    ::write(fd, &key, KEY_SIZE);
    ::write(fd, &value, VAL_SIZE);
    ::write(fd, &checksum_v, sizeof(uint32_t));
    ::fsync(fd);
    index[key] = end_ptr * (RECORD_SIZE);
    ++end_ptr;
    return 0;
  }

  size_t del(K_t key) { return put(key, static_cast<V_t>(0)); }

  std::optional<V_t> get(K_t key) {
    if (!index.contains(key)) {
      return {};
    }
    size_t offset = index[key];
    ::pread(fd, buffer.data(), buffer.size(), static_cast<off_t>(offset));
    V_t value;
    std::memcpy(&value, buffer.data() + KEY_SIZE, VAL_SIZE);
    if (value == static_cast<V_t>(0)) {
      return {};
    }
    return value;
  }
};
