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
  static constexpr size_t WIRE_SIZE = 47;
  explicit EnterRequestView(const std::byte *buf) : p(buf) {}
  const std::byte *data() const { return p; }

  uint32_t UserRefNum() const { return detail::load_be32(p + 1); }
  SideEnum Side() const {
    return static_cast<SideEnum>(detail::load_char(p + 5));
  }
  uint32_t Quantity() const { return detail::load_be32(p + 6); }
  std::string_view Symbol() const { return detail::load_str(p + 10, 8); }
  uint64_t Price() const { return detail::load_be64(p + 18); }
  TimeInForceEnum TimeInForce() const {
    return static_cast<TimeInForceEnum>(detail::load_char(p + 26));
  }
  DisplayEnum Display() const {
    return static_cast<DisplayEnum>(detail::load_char(p + 27));
  }
  CapacityEnum Capacity() const {
    return static_cast<CapacityEnum>(detail::load_char(p + 28));
  }
  InterMarketSweepEligEnum InterMarketSweepElig() const {
    return static_cast<InterMarketSweepEligEnum>(detail::load_char(p + 29));
  }
  CrossTypeEnum CrossType() const {
    return static_cast<CrossTypeEnum>(detail::load_char(p + 30));
  }
  std::string_view ClOrdID() const { return detail::load_str(p + 31, 14); }
  uint16_t AppendageLength() const { return detail::load_be16(p + 45); }
};

class CancelRequestView {
  const std::byte *p;

public:
  static constexpr char TYPE = 'X';
  static constexpr size_t WIRE_SIZE = 9; // Appendage Length is optional
  explicit CancelRequestView(const std::byte *buf) : p(buf) {}

  uint32_t UserRefNum() const { return detail::load_be32(p + 1); }
  uint32_t Quantity() const { return detail::load_be32(p + 5); }
};

class AcceptResponseView {
  const std::byte *p;

public:
  static constexpr char TYPE = 'A';
  static constexpr size_t WIRE_SIZE = 64;
  explicit AcceptResponseView(const std::byte *buf) : p(buf) {}

  uint64_t Timestamp() const { return detail::load_be64(p + 1); }
  uint32_t UserRefNum() const { return detail::load_be32(p + 9); }
  SideEnum Side() const {
    return static_cast<SideEnum>(detail::load_char(p + 13));
  }
  uint32_t Quantity() const { return detail::load_be32(p + 14); }
  std::string_view Symbol() const { return detail::load_str(p + 18, 8); }
  uint64_t Price() const { return detail::load_be64(p + 26); }
  TimeInForceEnum TimeInForce() const {
    return static_cast<TimeInForceEnum>(detail::load_char(p + 34));
  }
  DisplayEnum Display() const {
    return static_cast<DisplayEnum>(detail::load_char(p + 35));
  }
  uint64_t OrderReferenceNumber() const { return detail::load_be64(p + 36); }
  char Capacity() const { return detail::load_char(p + 44); }
  InterMarketSweepEligEnum InterMarketSweepElig() const {
    return static_cast<InterMarketSweepEligEnum>(detail::load_char(p + 45));
  }
  CrossTypeEnum CrossType() const {
    return static_cast<CrossTypeEnum>(detail::load_char(p + 46));
  }
  OrderStateEnum OrderState() const {
    return static_cast<OrderStateEnum>(detail::load_char(p + 47));
  }
  std::string_view ClOrdID() const { return detail::load_str(p + 48, 14); }
  uint16_t AppendageLength() const { return detail::load_be16(p + 62); }
};

class CancelledResponseView {
  const std::byte *p;

public:
  static constexpr char TYPE = 'C';
  static constexpr size_t WIRE_SIZE = 18; // Appendage Length is optional
  explicit CancelledResponseView(const std::byte *buf) : p(buf) {}

  uint64_t Timestamp() const { return detail::load_be64(p + 1); }
  uint32_t UserRefNum() const { return detail::load_be32(p + 9); }
  uint32_t Quantity() const { return detail::load_be32(p + 13); }
  char Reason() const { return detail::load_char(p + 17); }
};

class ExecutedResponseView {
  const std::byte *p;

public:
  static constexpr char TYPE = 'E';
  static constexpr size_t WIRE_SIZE = 36;
  explicit ExecutedResponseView(const std::byte *buf) : p(buf) {}

  uint64_t Timestamp() const { return detail::load_be64(p + 1); }
  uint32_t UserRefNum() const { return detail::load_be32(p + 9); }
  uint32_t Quantity() const { return detail::load_be32(p + 13); }
  uint64_t Price() const { return detail::load_be64(p + 17); }
  char LiquidityFlag() const { return detail::load_char(p + 25); }
  uint64_t MatchNumber() const { return detail::load_be64(p + 26); }
  uint16_t AppendageLength() const { return detail::load_be16(p + 34); }
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

  EnterRequestBuilder &UserRefNum(uint32_t v) {
    detail::store_be32(buf.data() + 1, v);
    return *this;
  }
  EnterRequestBuilder &Side(SideEnum v) {
    detail::store_char(buf.data() + 5, static_cast<char>(v));
    return *this;
  }
  EnterRequestBuilder &Quantity(uint32_t v) {
    detail::store_be32(buf.data() + 6, v);
    return *this;
  }
  EnterRequestBuilder &Symbol(std::string_view v) {
    detail::store_str(buf.data() + 10, v, 8);
    return *this;
  }
  EnterRequestBuilder &Price(uint64_t v) {
    detail::store_be64(buf.data() + 18, v);
    return *this;
  }
  EnterRequestBuilder &TimeInForce(TimeInForceEnum v) {
    detail::store_char(buf.data() + 26, static_cast<char>(v));
    return *this;
  }
  EnterRequestBuilder &Display(DisplayEnum v) {
    detail::store_char(buf.data() + 27, static_cast<char>(v));
    return *this;
  }
  EnterRequestBuilder &Capacity(CapacityEnum v) {
    detail::store_char(buf.data() + 28, static_cast<char>(v));
    return *this;
  }
  EnterRequestBuilder &InterMarketSweepElig(InterMarketSweepEligEnum v) {
    detail::store_char(buf.data() + 29, static_cast<char>(v));
    return *this;
  }
  EnterRequestBuilder &CrossType(CrossTypeEnum v) {
    detail::store_char(buf.data() + 30, static_cast<char>(v));
    return *this;
  }
  EnterRequestBuilder &ClOrdID(std::string_view v) {
    detail::store_str(buf.data() + 31, v, 14);
    return *this;
  }

  EnterRequestBuilder &AppendageLength(uint16_t v) {
    detail::store_be16(buf.data() + 45, v);
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

  CancelRequestBuilder &UserRefNum(uint32_t v) {
    detail::store_be32(buf.data() + 1, v);
    return *this;
  }
  CancelRequestBuilder &Quantity(uint32_t v) {
    detail::store_be32(buf.data() + 5, v);
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
  AcceptResponseBuilder &UserRefNum(uint32_t v) {
    detail::store_be32(buf.data() + 9, v);
    return *this;
  }
  AcceptResponseBuilder &Side(SideEnum v) {
    detail::store_char(buf.data() + 13, static_cast<char>(v));
    return *this;
  }
  AcceptResponseBuilder &Quantity(uint32_t v) {
    detail::store_be32(buf.data() + 14, v);
    return *this;
  }
  AcceptResponseBuilder &Symbol(std::string_view v) {
    detail::store_str(buf.data() + 18, v, 8);
    return *this;
  }
  AcceptResponseBuilder &Price(uint64_t v) {
    detail::store_be64(buf.data() + 26, v);
    return *this;
  }
  AcceptResponseBuilder &TimeInForce(TimeInForceEnum v) {
    detail::store_char(buf.data() + 34, static_cast<char>(v));
    return *this;
  }
  AcceptResponseBuilder &Display(DisplayEnum v) {
    detail::store_char(buf.data() + 35, static_cast<char>(v));
    return *this;
  }
  AcceptResponseBuilder &OrderReferenceNumber(uint64_t v) {
    detail::store_be64(buf.data() + 36, v);
    return *this;
  }
  AcceptResponseBuilder &Capacity(char v) {
    detail::store_char(buf.data() + 44, v);
    return *this;
  }
  AcceptResponseBuilder &InterMarketSweepElig(InterMarketSweepEligEnum v) {
    detail::store_char(buf.data() + 45, static_cast<char>(v));
    return *this;
  }
  AcceptResponseBuilder &CrossType(CrossTypeEnum v) {
    detail::store_char(buf.data() + 46, static_cast<char>(v));
    return *this;
  }
  AcceptResponseBuilder &OrderState(OrderStateEnum v) {
    detail::store_char(buf.data() + 47, static_cast<char>(v));
    return *this;
  }
  AcceptResponseBuilder &ClOrdID(std::string_view v) {
    detail::store_str(buf.data() + 48, v, 14);
    return *this;
  }

  AcceptResponseBuilder &AppendageLength(uint16_t v) {
    detail::store_be16(buf.data() + 62, v);
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
  CancelledResponseBuilder &UserRefNum(uint32_t v) {
    detail::store_be32(buf.data() + 9, v);
    return *this;
  }
  CancelledResponseBuilder &Quantity(uint32_t v) {
    detail::store_be32(buf.data() + 13, v);
    return *this;
  }
  CancelledResponseBuilder &Reason(char v) {
    detail::store_char(buf.data() + 17, v);
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
  ExecutedResponseBuilder &UserRefNum(uint32_t v) {
    detail::store_be32(buf.data() + 9, v);
    return *this;
  }
  ExecutedResponseBuilder &Quantity(uint32_t v) {
    detail::store_be32(buf.data() + 13, v);
    return *this;
  }
  ExecutedResponseBuilder &Price(uint64_t v) {
    detail::store_be64(buf.data() + 17, v);
    return *this;
  }
  ExecutedResponseBuilder &LiquidityFlag(char v) {
    detail::store_char(buf.data() + 25, v);
    return *this;
  }
  ExecutedResponseBuilder &MatchNumber(uint64_t v) {
    detail::store_be64(buf.data() + 26, v);
    return *this;
  }

  ExecutedResponseBuilder &AppendageLength(uint16_t v) {
    detail::store_be16(buf.data() + 34, v);
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

// What inbound pushes to the engine: the wire bytes plus the OUCH account of
// the connection they arrived on. The account is not part of the message.
struct InboundMessage {
  uint32_t account;
  OUCHMessageIn bytes;
};

struct OutboundMessage {
  uint32_t account;
  OUCHMessageOut bytes;
};

UOUCHMessage decode(const std::byte *bytes);
