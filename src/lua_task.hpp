#pragma once

#include <task.hpp>

class LuaTask : public Task {
  struct Impl;
  std::unique_ptr<Impl> impl;
public:
  LuaTask() = delete;
  LuaTask(lua_State*, int);
  void touch(int x, int y) override;
  void gaze(int x, int y) override;
  void update(std::unique_lock<std::mutex>& lock) override;
  void draw(SkCanvas* canvas, View view) override;
  std::optional<task_controller_grpc::TaskResult> status() override;
};
