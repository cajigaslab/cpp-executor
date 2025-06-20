#pragma once
#include <random>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include <task_controller.pb.h>

#include "json/value.h"

#include "include/core/SkColor.h"
#include "include/core/SkCanvas.h"

#include "boost/asio.hpp"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

struct lua_State;

struct Context {
  std::random_device device;
  std::mt19937 engine;
  Json::Value config;
  Context();
  double get_uniform_field(const std::string& field);
  double get_uniform(const Json::Value& value);
  SkColor get_color_field(const std::string& field);
  SkColor get_color(const Json::Value& value);
};

enum class View {
  SUBJECT,
  OPERATOR
};

class Task {
public:
  virtual ~Task() {}
  virtual void touch(int x, int y) = 0;
  virtual void gaze(int x, int y) = 0;
  virtual void update(std::unique_lock<std::mutex>&) = 0;
  virtual void draw(SkCanvas* canvas, View view) = 0;
  virtual std::optional<task_controller_grpc::TaskResult> status() = 0;
};

Task* make_task(boost::asio::io_context& io_context, Context& context, lua_State*);
