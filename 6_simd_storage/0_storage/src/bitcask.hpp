#include <fstream>
#include <map>
#include <optional>
#include <string>
template <typename K_t, typename V_t> class Bitcask {
  std::fstream file;
  std::map<K_t, V_t> index; // Balanced BST, like RB. Already sorted inherently

public:
  Bitcask() = delete;
  explicit Bitcask(std::string filename) {
    file.open(filename, std::ios::in | std::ios::out | std::ios::binary);

    // TODO: recreated the index operation
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
  size_t put(K_t key, V_t value); // TODO: implement
  std::optional<V_t> get(K_t key); // TODO: implement
};
