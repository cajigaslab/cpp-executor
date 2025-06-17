#include <hold_state_machine.hpp>
#include <optional>

using namespace std::chrono_literals;

struct HoldStateMachine::Impl {
  std::chrono::milliseconds hold;
  std::chrono::milliseconds blink;
  std::optional<std::chrono::steady_clock::time_point> blink_start;
  std::chrono::steady_clock::time_point start;
  std::chrono::milliseconds blink_time = 0ms;

  Impl()
    : hold(0ms), blink(0ms), start(std::chrono::steady_clock::now()) {}
  
  Impl(std::chrono::milliseconds _hold, std::chrono::milliseconds _blink)
    : hold(_hold), blink(_blink), start(std::chrono::steady_clock::now()) {}
};

HoldStateMachine::HoldStateMachine(HoldStateMachine&& other) : impl(new Impl()) {
  std::swap(impl, other.impl);
}
HoldStateMachine::HoldStateMachine()
  : impl(new Impl()) {}

HoldStateMachine::HoldStateMachine(std::chrono::milliseconds hold, std::chrono::milliseconds blink)
  : impl(new Impl(hold, blink)) {}
HoldStateMachine::~HoldStateMachine() {}

HoldStateMachine& HoldStateMachine::operator=(HoldStateMachine&& other) {
  std::swap(impl, other.impl);
  return *this;
}

void HoldStateMachine::lost() {
  impl->blink_start = std::chrono::steady_clock::now();
}

void HoldStateMachine::acquired(bool value) {
  if(!value) {
    return lost();
  }
  if(!impl->blink_start.has_value()) {
    return;
  }
  auto now = std::chrono::steady_clock::now();
  impl->blink_time += std::chrono::duration_cast<std::chrono::milliseconds>(now - *impl->blink_start);
  impl->blink_start = std::nullopt;
}

HoldStateMachine::Status HoldStateMachine::update() {
  auto now = std::chrono::steady_clock::now();
  if(impl->blink_start.has_value()) {
    auto current_blink_time = now - *impl->blink_start;
    if(current_blink_time >= impl->blink) {
      return Status::FAIL;
    }
  } else if (now - impl->start > impl->hold + impl->blink_time) {
    return Status::SUCCESS;
  }
  return Status::PENDING;
}
