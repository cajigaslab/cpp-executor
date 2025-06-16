#pragma once

#include <task.hpp>

class SimpleTask : public Task {
  struct Impl;
  std::unique_ptr<Impl> impl;
public:
  SimpleTask() = delete;
  SimpleTask(Context& context);
  void touch(int x, int y) override;
  void gaze(int x, int y) override;
  void update() override;
  void draw(SkCanvas* canvas, View view) override;
  std::optional<task_controller_grpc::TaskResult> status() override;
};
