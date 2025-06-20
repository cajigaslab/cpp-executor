#pragma once

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include <boost/asio.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <task.hpp>

class LuaCoroutineTask : public Task {
  struct Impl;
  std::unique_ptr<Impl> impl;
public:
  LuaCoroutineTask() = delete;
  LuaCoroutineTask(boost::asio::io_context& io_context, lua_State*);
  void touch(int x, int y) override;
  void gaze(int x, int y) override;
  void update(std::unique_lock<std::mutex>&) override;
  void draw(SkCanvas* canvas, View view) override;
  std::optional<task_controller_grpc::TaskResult> status() override;
};
