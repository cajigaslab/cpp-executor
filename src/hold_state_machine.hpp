#pragma once

#include <chrono>

class HoldStateMachine {
  struct Impl;
  std::unique_ptr<Impl> impl;
public:
  enum class Status {
    PENDING,
    SUCCESS,
    FAIL
  } status = Status::PENDING;
  HoldStateMachine();
  HoldStateMachine(HoldStateMachine&&);
  HoldStateMachine(std::chrono::milliseconds hold, std::chrono::milliseconds blink);
  ~HoldStateMachine();

  HoldStateMachine& operator=(HoldStateMachine&&);

  void lost();
  void acquired(bool value = true);
  Status update();
};
