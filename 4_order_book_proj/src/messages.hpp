#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <variant>

// Enums backed by char: 1 byte on the wire.
enum class SideEnum : char { B = 'B', S = 'S', T = 'T', E = 'E' };
enum class TimeInForceEnum : char {
  Day = '0',
  IOC = '3',
  FOK = '5',
  GTC = '6',
  E = 'E'
};
enum class DisplayEnum : char { Y = 'Y', N = 'N', A = 'A' };
enum class CapacityEnum : char { A = 'A', P = 'P', R = 'R', O = 'O' };
enum class InterMarketSweepEligEnum : char { Y = 'Y', N = 'N' };
enum class CrossTypeEnum : char {
  N = 'N',
  O = 'O',
  C = 'C',
  H = 'H',
  S = 'S',
  R = 'R',
  E = 'E',
  A = 'A'
};
enum class OrderStateEnum : char { L = 'L', D = 'D' };

namespace detail {

inline uint16_t load_be16(const std::byte *p) {
  uint16_t v;
  std::memcpy(&v, p, sizeof(v));
  return __builtin_bswap16(v);
}
inline uint32_t load_be32(const std::byte *p) {
  uint32_t v;
  std::memcpy(&v, p, sizeof(v));
  return __builtin_bswap32(v);
}
inline uint64_t load_be64(const std::byte *p) {
  uint64_t v;
  std::memcpy(&v, p, sizeof(v));
  return __builtin_bswap64(v);
}
inline char load_char(const std::byte *p) { return std::bit_cast<char>(*p); }
inline std::string_view load_str(const std::byte *p, size_t n) {
  return {reinterpret_cast<const char *>(p), n};
}

inline void store_be16(std::byte *p, uint16_t v) {
  uint16_t be = __builtin_bswap16(v);
  std::memcpy(p, &be, sizeof(be));
}
inline void store_be32(std::byte *p, uint32_t v) {
  uint32_t be = __builtin_bswap32(v);
  std::memcpy(p, &be, sizeof(be));
}
inline void store_be64(std::byte *p, uint64_t v) {
  uint64_t be = __builtin_bswap64(v);
  std::memcpy(p, &be, sizeof(be));
}
inline void store_char(std::byte *p, char c) {
  *p = std::bit_cast<std::byte>(c);
}
// OUCH string fields are space-padded fixed width.
inline void store_str(std::byte *p, std::string_view s, size_t n) {
  size_t copy_n = std::min(s.size(), n);
  std::memcpy(p, s.data(), copy_n);
  if (copy_n < n) {
    std::memset(p + copy_n, ' ', n - copy_n);
  }
}

} // namespace detail

// ---------------- Views (decode side) ----------------
// Wrap a pointer into an externally-owned wire buffer. Buffer must outlive the
// view. Each accessor reads only the bytes it needs. Zero copy.

class EnterRequestView {
  const std::byte *p;

public:
  static constexpr char TYPE = 'O';
  static constexpr size_t WIRE_SIZE = 49;
  explicit EnterRequestView(const std::byte *buf) : p(buf) {}
  const std::byte *data() const { return p; }

  uint64_t UserRefNum() const { return detail::load_be64(p + 1); }
  SideEnum Side() const {
    return static_cast<SideEnum>(detail::load_char(p + 9));
  }
  uint32_t Quantity() const { return detail::load_be32(p + 10); }
  std::string_view Symbol() const { return detail::load_str(p + 14, 8); }
  uint64_t Price() const { return detail::load_be64(p + 22); }
  TimeInForceEnum TimeInForce() const {
    return static_cast<TimeInForceEnum>(detail::load_char(p + 30));
  }
  DisplayEnum Display() const {
    return static_cast<DisplayEnum>(detail::load_char(p + 31));
  }
  CapacityEnum Capacity() const {
    return static_cast<CapacityEnum>(detail::load_char(p + 32));
  }
  InterMarketSweepEligEnum InterMarketSweepElig() const {
    return static_cast<InterMarketSweepEligEnum>(detail::load_char(p + 33));
  }
  CrossTypeEnum CrossType() const {
    return static_cast<CrossTypeEnum>(detail::load_char(p + 34));
  }
  std::string_view ClOrdID() const { return detail::load_str(p + 35, 14); }
};

class CancelRequestView {
  const std::byte *p;

public:
  static constexpr char TYPE = 'X';
  static constexpr size_t WIRE_SIZE = 13;
  explicit CancelRequestView(const std::byte *buf) : p(buf) {}

  uint64_t UserRefNum() const { return detail::load_be64(p + 1); }
  uint32_t Quantity() const { return detail::load_be32(p + 9); }
};

class AcceptResponseView {
  const std::byte *p;

public:
  static constexpr char TYPE = 'A';
  static constexpr size_t WIRE_SIZE = 66;
  explicit AcceptResponseView(const std::byte *buf) : p(buf) {}

  uint64_t Timestamp() const { return detail::load_be64(p + 1); }
  uint64_t UserRefNum() const { return detail::load_be64(p + 9); }
  SideEnum Side() const {
    return static_cast<SideEnum>(detail::load_char(p + 17));
  }
  uint32_t Quantity() const { return detail::load_be32(p + 18); }
  std::string_view Symbol() const { return detail::load_str(p + 22, 8); }
  uint64_t Price() const { return detail::load_be64(p + 30); }
  TimeInForceEnum TimeInForce() const {
    return static_cast<TimeInForceEnum>(detail::load_char(p + 38));
  }
  DisplayEnum Display() const {
    return static_cast<DisplayEnum>(detail::load_char(p + 39));
  }
  uint64_t OrderReferenceNumber() const { return detail::load_be64(p + 40); }
  char Capacity() const { return detail::load_char(p + 48); }
  InterMarketSweepEligEnum InterMarketSweepElig() const {
    return static_cast<InterMarketSweepEligEnum>(detail::load_char(p + 49));
  }
  CrossTypeEnum CrossType() const {
    return static_cast<CrossTypeEnum>(detail::load_char(p + 50));
  }
  OrderStateEnum OrderState() const {
    return static_cast<OrderStateEnum>(detail::load_char(p + 51));
  }
  std::string_view ClOrdID() const { return detail::load_str(p + 52, 14); }
};

class CancelledResponseView {
  const std::byte *p;

public:
  static constexpr char TYPE = 'C';
  static constexpr size_t WIRE_SIZE = 22;
  explicit CancelledResponseView(const std::byte *buf) : p(buf) {}

  uint64_t Timestamp() const { return detail::load_be64(p + 1); }
  uint64_t UserRefNum() const { return detail::load_be64(p + 9); }
  uint32_t Quantity() const { return detail::load_be32(p + 17); }
  char Reason() const { return detail::load_char(p + 21); }
};

class ExecutedResponseView {
  const std::byte *p;

public:
  static constexpr char TYPE = 'E';
  static constexpr size_t WIRE_SIZE = 38;
  explicit ExecutedResponseView(const std::byte *buf) : p(buf) {}

  uint64_t Timestamp() const { return detail::load_be64(p + 1); }
  uint64_t UserRefNum() const { return detail::load_be64(p + 9); }
  uint32_t Quantity() const { return detail::load_be32(p + 17); }
  uint64_t Price() const { return detail::load_be64(p + 21); }
  char LiquidityFlag() const { return detail::load_char(p + 29); }
  uint64_t MatchNumber() const { return detail::load_be64(p + 30); }
};

// ---------------- Builders (encode side) ----------------
// Own a fixed-size buffer sized to WIRE_SIZE. Constructor stamps the type byte
// at offset 0. Setters write big-endian at fixed offsets and return *this for
// chaining. bytes() hands the wire buffer to the transport.

class EnterRequestBuilder {
  std::array<std::byte, EnterRequestView::WIRE_SIZE> buf{};

public:
  EnterRequestBuilder() {
    detail::store_char(buf.data() + 0, EnterRequestView::TYPE);
  }

  EnterRequestBuilder &UserRefNum(uint64_t v) {
    detail::store_be64(buf.data() + 1, v);
    return *this;
  }
  EnterRequestBuilder &Side(SideEnum v) {
    detail::store_char(buf.data() + 9, static_cast<char>(v));
    return *this;
  }
  EnterRequestBuilder &Quantity(uint32_t v) {
    detail::store_be32(buf.data() + 10, v);
    return *this;
  }
  EnterRequestBuilder &Symbol(std::string_view v) {
    detail::store_str(buf.data() + 14, v, 8);
    return *this;
  }
  EnterRequestBuilder &Price(uint64_t v) {
    detail::store_be64(buf.data() + 22, v);
    return *this;
  }
  EnterRequestBuilder &TimeInForce(TimeInForceEnum v) {
    detail::store_char(buf.data() + 30, static_cast<char>(v));
    return *this;
  }
  EnterRequestBuilder &Display(DisplayEnum v) {
    detail::store_char(buf.data() + 31, static_cast<char>(v));
    return *this;
  }
  EnterRequestBuilder &Capacity(CapacityEnum v) {
    detail::store_char(buf.data() + 32, static_cast<char>(v));
    return *this;
  }
  EnterRequestBuilder &InterMarketSweepElig(InterMarketSweepEligEnum v) {
    detail::store_char(buf.data() + 33, static_cast<char>(v));
    return *this;
  }
  EnterRequestBuilder &CrossType(CrossTypeEnum v) {
    detail::store_char(buf.data() + 34, static_cast<char>(v));
    return *this;
  }
  EnterRequestBuilder &ClOrdID(std::string_view v) {
    detail::store_str(buf.data() + 35, v, 14);
    return *this;
  }

  std::span<const std::byte> bytes() const { return buf; }
};

class CancelRequestBuilder {
  std::array<std::byte, CancelRequestView::WIRE_SIZE> buf{};

public:
  CancelRequestBuilder() {
    detail::store_char(buf.data() + 0, CancelRequestView::TYPE);
  }

  CancelRequestBuilder &UserRefNum(uint64_t v) {
    detail::store_be64(buf.data() + 1, v);
    return *this;
  }
  CancelRequestBuilder &Quantity(uint32_t v) {
    detail::store_be32(buf.data() + 9, v);
    return *this;
  }

  std::span<const std::byte> bytes() const { return buf; }
};

class AcceptResponseBuilder {
  std::array<std::byte, AcceptResponseView::WIRE_SIZE> buf{};

public:
  AcceptResponseBuilder() {
    detail::store_char(buf.data() + 0, AcceptResponseView::TYPE);
  }

  AcceptResponseBuilder &Timestamp(uint64_t v) {
    detail::store_be64(buf.data() + 1, v);
    return *this;
  }
  AcceptResponseBuilder &UserRefNum(uint64_t v) {
    detail::store_be64(buf.data() + 9, v);
    return *this;
  }
  AcceptResponseBuilder &Side(SideEnum v) {
    detail::store_char(buf.data() + 17, static_cast<char>(v));
    return *this;
  }
  AcceptResponseBuilder &Quantity(uint32_t v) {
    detail::store_be32(buf.data() + 18, v);
    return *this;
  }
  AcceptResponseBuilder &Symbol(std::string_view v) {
    detail::store_str(buf.data() + 22, v, 8);
    return *this;
  }
  AcceptResponseBuilder &Price(uint64_t v) {
    detail::store_be64(buf.data() + 30, v);
    return *this;
  }
  AcceptResponseBuilder &TimeInForce(TimeInForceEnum v) {
    detail::store_char(buf.data() + 38, static_cast<char>(v));
    return *this;
  }
  AcceptResponseBuilder &Display(DisplayEnum v) {
    detail::store_char(buf.data() + 39, static_cast<char>(v));
    return *this;
  }
  AcceptResponseBuilder &OrderReferenceNumber(uint64_t v) {
    detail::store_be64(buf.data() + 40, v);
    return *this;
  }
  AcceptResponseBuilder &Capacity(char v) {
    detail::store_char(buf.data() + 48, v);
    return *this;
  }
  AcceptResponseBuilder &InterMarketSweepElig(InterMarketSweepEligEnum v) {
    detail::store_char(buf.data() + 49, static_cast<char>(v));
    return *this;
  }
  AcceptResponseBuilder &CrossType(CrossTypeEnum v) {
    detail::store_char(buf.data() + 50, static_cast<char>(v));
    return *this;
  }
  AcceptResponseBuilder &OrderState(OrderStateEnum v) {
    detail::store_char(buf.data() + 51, static_cast<char>(v));
    return *this;
  }
  AcceptResponseBuilder &ClOrdID(std::string_view v) {
    detail::store_str(buf.data() + 52, v, 14);
    return *this;
  }

  std::span<const std::byte> bytes() const { return buf; }
};

class CancelledResponseBuilder {
  std::array<std::byte, CancelledResponseView::WIRE_SIZE> buf{};

public:
  CancelledResponseBuilder() {
    detail::store_char(buf.data() + 0, CancelledResponseView::TYPE);
  }

  CancelledResponseBuilder &Timestamp(uint64_t v) {
    detail::store_be64(buf.data() + 1, v);
    return *this;
  }
  CancelledResponseBuilder &UserRefNum(uint64_t v) {
    detail::store_be64(buf.data() + 9, v);
    return *this;
  }
  CancelledResponseBuilder &Quantity(uint32_t v) {
    detail::store_be32(buf.data() + 17, v);
    return *this;
  }
  CancelledResponseBuilder &Reason(char v) {
    detail::store_char(buf.data() + 21, v);
    return *this;
  }

  std::span<const std::byte> bytes() const { return buf; }
};

class ExecutedResponseBuilder {
  std::array<std::byte, ExecutedResponseView::WIRE_SIZE> buf{};

public:
  ExecutedResponseBuilder() {
    detail::store_char(buf.data() + 0, ExecutedResponseView::TYPE);
  }

  ExecutedResponseBuilder &Timestamp(uint64_t v) {
    detail::store_be64(buf.data() + 1, v);
    return *this;
  }
  ExecutedResponseBuilder &UserRefNum(uint64_t v) {
    detail::store_be64(buf.data() + 9, v);
    return *this;
  }
  ExecutedResponseBuilder &Quantity(uint32_t v) {
    detail::store_be32(buf.data() + 17, v);
    return *this;
  }
  ExecutedResponseBuilder &Price(uint64_t v) {
    detail::store_be64(buf.data() + 21, v);
    return *this;
  }
  ExecutedResponseBuilder &LiquidityFlag(char v) {
    detail::store_char(buf.data() + 29, v);
    return *this;
  }
  ExecutedResponseBuilder &MatchNumber(uint64_t v) {
    detail::store_be64(buf.data() + 30, v);
    return *this;
  }

  std::span<const std::byte> bytes() const { return buf; }
};

using UOUCHMessage =
    std::variant<EnterRequestView, CancelRequestView, AcceptResponseView,
                 CancelledResponseView, ExecutedResponseView>;
// Queue payloads: raw wire bytes, owned by value, sized for the largest type.
// Wrap with decode() / a View on the consumer side.
using OUCHMessageIn = std::array<
    std::byte, std::max(EnterRequestView::WIRE_SIZE, CancelRequestView::WIRE_SIZE)>;
using OUCHMessageOut =
    std::array<std::byte, std::max({AcceptResponseView::WIRE_SIZE,
                                    CancelledResponseView::WIRE_SIZE,
                                    ExecutedResponseView::WIRE_SIZE})>;

UOUCHMessage decode(const std::byte *bytes);
