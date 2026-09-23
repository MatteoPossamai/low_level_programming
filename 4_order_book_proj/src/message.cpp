#include "messages.hpp"
#include <bit>

UOUCHMessage decode(const std::byte *bytes) {
  switch (std::bit_cast<char>(bytes[0])) {
  case EnterRequestView::TYPE:
    return EnterRequestView{bytes};
  case CancelRequestView::TYPE:
    return CancelRequestView{bytes};
  case AcceptResponseView::TYPE:
    return AcceptResponseView{bytes};
  case CancelledResponseView::TYPE:
    return CancelledResponseView{bytes};
  case ExecutedResponseView::TYPE:
    return ExecutedResponseView{bytes};
  default:
    __builtin_unreachable();
  }
}
