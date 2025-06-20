#include <simple_task.hpp>

#include <hold_state_machine.hpp>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include <include/core/SkRect.h>
#include <include/core/SkFont.h>
#include <include/core/SkColor.h>
#include "include/core/SkFontMgr.h"
#include "include/ports/SkTypeface_win.h"
#ifdef __clang__
#pragma clang diagnostic pop
#endif

enum class State {
  NONE,
  INTERTRIAL,
  START_ON,
  START_ACQ,
  SUCCESS,
  FAIL
};

struct SimpleTask::Impl {
  State state = State::NONE;
  sk_sp<SkTypeface> face;
  std::chrono::milliseconds intertrial_timeout;
  std::chrono::milliseconds start_timeout;
  std::chrono::milliseconds hold_timeout;
  std::chrono::milliseconds blink_timeout;
  std::chrono::milliseconds fail_timeout;
  std::chrono::milliseconds success_timeout;
  double target_x;
  double target_y;
  double target_width;
  double target_height;
  SkRect target;
  SkColor target_color;
  std::chrono::steady_clock::time_point start;
  std::optional<task_controller_grpc::TaskResult> _status;
  HoldStateMachine hold;
  Impl(Context& context) {
    intertrial_timeout = std::chrono::milliseconds(int(1000*context.get_uniform_field("intertrial_timeout")));
    start_timeout = std::chrono::milliseconds(int(1000*context.get_uniform_field("start_timeout")));
    hold_timeout = std::chrono::milliseconds(int(1000*context.get_uniform_field("hold_timeout")));
    success_timeout = std::chrono::milliseconds(int(1000*context.get_uniform_field("success_timeout")));
    fail_timeout = std::chrono::milliseconds(int(1000*context.get_uniform_field("fail_timeout")));
    blink_timeout = std::chrono::milliseconds(int(1000*context.get_uniform_field("blink_timeout")));
    target_x = context.get_uniform_field("target_x");
    target_y = context.get_uniform_field("target_y");
    target_width = context.get_uniform_field("target_width");
    target_height = context.get_uniform_field("target_height");
    target = SkRect::MakeXYWH(float(target_x), float(target_y), float(target_width), float(target_height));
    target_color = context.get_color_field("target_color");

    auto fontMgr = SkFontMgr_New_GDI();
    face = fontMgr->matchFamilyStyle(nullptr, SkFontStyle());
    //face = fontMgr->makeFromFile("C:\\Users\\jarl\\Downloads\\cmu\\cmunsx.ttf");
  }
  void touch(int x, int y) {
    switch(state) {
      case State::NONE:
      case State::SUCCESS:
      case State::FAIL:
        {
          break;
        }
      case State::INTERTRIAL:
        {
          start = std::chrono::steady_clock::now();
          state = State::FAIL;
          break;
        }
      case State::START_ON:
        {
          if(target.contains(float(x), float(y))) {
            hold = HoldStateMachine(hold_timeout, blink_timeout);
            state = State::START_ACQ;
          } else {
            start = std::chrono::steady_clock::now();
            state = State::FAIL;
          }
          break;
        }
      case State::START_ACQ:
        {
          hold.acquired(target.contains(float(x), float(y)));
          break;
        }
    }
  }
  void gaze(int, int) {}
  void update() {
    switch(state) {
      case State::NONE:
        {
          start = std::chrono::steady_clock::now();
          state = State::INTERTRIAL;
          break;
        }
      case State::INTERTRIAL:
        {
          auto now = std::chrono::steady_clock::now();
          if(now - start > intertrial_timeout) {
            start = std::chrono::steady_clock::now();
            state = State::START_ON;
          }
          break;
        }
      case State::START_ON:
        {
          auto now = std::chrono::steady_clock::now();
          if(now - start > start_timeout) {
            start = std::chrono::steady_clock::now();
            state = State::INTERTRIAL;
          }
          break;
        }
      case State::START_ACQ:
        {
          auto status = hold.update();
          if(status == HoldStateMachine::Status::SUCCESS) {
            start = std::chrono::steady_clock::now();
            state = State::SUCCESS;
          } else if (status == HoldStateMachine::Status::FAIL) {
            start = std::chrono::steady_clock::now();
            state = State::FAIL;
          }
          break;
        }
      case State::SUCCESS:
        {
          auto now = std::chrono::steady_clock::now();
          if(now - start > success_timeout) {
            _status = task_controller_grpc::TaskResult();
            _status->set_success(true);
          }
          break;
        }
      case State::FAIL:
        {
          auto now = std::chrono::steady_clock::now();
          if(now - start > fail_timeout) {
            _status = task_controller_grpc::TaskResult();
          }
          break;
        }
    }
  }
  void draw(SkCanvas* canvas, View view) {
    if(state == State::START_ON || state == State::START_ACQ) {
      canvas->drawRect(target, SkPaint(SkColor4f::FromColor(target_color)));
    }
    if(view == View::OPERATOR) {
      auto now = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
      auto duration_string = SkString(std::to_string(duration.count()));
      canvas->drawString(duration_string, 0, 50, SkFont(face, 48), SkPaint(SkColor4f::FromColor(SK_ColorRED)));
    }
  }
  std::optional<task_controller_grpc::TaskResult> status() {
    return _status;
  }
};

SimpleTask::SimpleTask(Context& context) : impl(new Impl(context)) {}

void SimpleTask::touch(int x, int y) {
  return impl->touch(x, y);
}
void SimpleTask::gaze(int x, int y) {
  return impl->gaze(x, y);
}
void SimpleTask::update(std::unique_lock<std::mutex>& lock) {
  return impl->update();
}
void SimpleTask::draw(SkCanvas* canvas, View view) {
  return impl->draw(canvas, view);
}

std::optional<task_controller_grpc::TaskResult> SimpleTask::status() {
  return impl->status();
}
