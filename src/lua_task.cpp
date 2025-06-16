#include <lua_task.hpp>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <lua/canvas.hpp>

enum class State {
  NONE,
  INTERTRIAL,
  START_ON,
  START_ACQ,
  SUCCESS,
  FAIL
};

struct LuaTask::Impl {
  lua_State* L;
  int ref;
  int subject_canvas_ref = LUA_NOREF;
  int operator_canvas_ref = LUA_NOREF;
  SkCanvas* subject_canvas = nullptr;
  SkCanvas* operator_canvas = nullptr;
  Impl(lua_State* _l, int _ref) : L(_l), ref(_ref) {}
  void touch(int x, int y) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    lua_getfield(L, -1, "touch");
    lua_pushvalue(L, -2);
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    if (lua_pcall(L, 3, 0, false)) {
      auto err_str = luaL_tolstring(L, -1, nullptr);
      std::cerr << err_str << std::endl;
      std::abort();
    }
    lua_pop(L, 1);
  }
  void gaze(int x, int y) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    lua_getfield(L, -1, "gaze");
    lua_pushvalue(L, -2);
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    if (lua_pcall(L, 3, 0, false)) {
      auto err_str = luaL_tolstring(L, -1, nullptr);
      std::cerr << err_str << std::endl;
      std::abort();
    }
    lua_pop(L, 1);
  }
  void update() {
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    lua_getfield(L, -1, "update");
    lua_pushvalue(L, -2);
    if (lua_pcall(L, 1, 0, false)) {
      auto err_str = luaL_tolstring(L, -1, nullptr);
      std::cerr << err_str << std::endl;
      std::abort();
    }
    lua_pop(L, 1);
  }
  void draw(SkCanvas* canvas, View view) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    lua_getfield(L, -1, "draw");
    lua_pushvalue(L, -2);

    if(view == View::OPERATOR) {
      if(canvas != operator_canvas) {
        luaL_unref(L, LUA_REGISTRYINDEX, operator_canvas_ref);
        canvas_wrap(L, canvas);
        operator_canvas_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        operator_canvas = canvas;
      }
      lua_rawgeti(L, LUA_REGISTRYINDEX, operator_canvas_ref);
      lua_pushstring(L, "OPERATOR");
    } else {
      if(canvas != subject_canvas) {
        luaL_unref(L, LUA_REGISTRYINDEX, subject_canvas_ref);
        canvas_wrap(L, canvas);
        subject_canvas_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        subject_canvas = canvas;
      }
      lua_rawgeti(L, LUA_REGISTRYINDEX, subject_canvas_ref);
      lua_pushstring(L, "SUBJECT");
    }

    if (lua_pcall(L, 3, 0, false)) {
      auto err_str = luaL_tolstring(L, -1, nullptr);
      std::cerr << err_str << std::endl;
      std::abort();
    }
    lua_pop(L, 1);
  }
  std::optional<task_controller_grpc::TaskResult> status() {
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    lua_getfield(L, -1, "status");
    lua_pushvalue(L, -2);
    if (lua_pcall(L, 1, 1, false)) {
      auto err_str = luaL_tolstring(L, -1, nullptr);
      std::cerr << err_str << std::endl;
      std::abort();
    }

    if(lua_isnil(L, -1)) {
      lua_pop(L, 2);
      return std::nullopt;
    } else {
      lua_getfield(L, -1, "success");
      auto success = lua_toboolean(L, -1);
      lua_pop(L, 2);
      task_controller_grpc::TaskResult result;
      result.set_success(success);
      return result;
    }
  }
};

LuaTask::LuaTask(lua_State* L, int ref) : impl(new Impl(L, ref)) {}

void LuaTask::touch(int x, int y) {
  return impl->touch(x, y);
}
void LuaTask::gaze(int x, int y) {
  return impl->gaze(x, y);
}
void LuaTask::update() {
  return impl->update();
}
void LuaTask::draw(SkCanvas* canvas, View view) {
  return impl->draw(canvas, view);
}

std::optional<task_controller_grpc::TaskResult> LuaTask::status() {
  return impl->status();
}
