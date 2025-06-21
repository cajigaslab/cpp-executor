#include <lua_coroutine_task.hpp>

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

struct LuaCoroutineTask::Impl {
  boost::asio::io_context& io_context;
  lua_State* L;
  int on_gaze_ref = LUA_NOREF;
  int on_touch_ref = LUA_NOREF;
  int renderer_ref = LUA_NOREF;
  int context_ref = LUA_NOREF;
  int coroutine_ref = LUA_NOREF;
  int subject_canvas_ref = LUA_NOREF;
  int operator_canvas_ref = LUA_NOREF;
  SkCanvas* subject_canvas = nullptr;
  SkCanvas* operator_canvas = nullptr;
  std::optional<task_controller_grpc::TaskResult> _status;
  Impl(boost::asio::io_context& io_context, lua_State* _l) : io_context(io_context), L(_l) {
    context_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    coroutine_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  }
  ~Impl() {
    luaL_unref(L, LUA_REGISTRYINDEX, context_ref);
    luaL_unref(L, LUA_REGISTRYINDEX, coroutine_ref);
  }
  void touch(int x, int y) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, context_ref);
    lua_getfield(L, -1, "on_touch");
    if(lua_isnil(L, -1)) {
      lua_pop(L, 2);
      return;
    }
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    if (lua_pcall(L, 2, 0, false)) {
      auto err_str = luaL_tolstring(L, -1, nullptr);
      std::cerr << err_str << std::endl;
      std::abort();
    }
    lua_pop(L, 1);
  }
  void gaze(int x, int y) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, context_ref);
    lua_getfield(L, -1, "on_gaze");
    if(lua_isnil(L, -1)) {
      lua_pop(L, 2);
      return;
    }
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    if (lua_pcall(L, 2, 0, false)) {
      auto err_str = luaL_tolstring(L, -1, nullptr);
      std::cerr << err_str << std::endl;
      std::abort();
    }
    lua_pop(L, 1);
  }
  void update(std::unique_lock<std::mutex>& lock) { 
    lua_rawgeti(L, LUA_REGISTRYINDEX, coroutine_ref);
    auto coroutine = lua_tothread(L, -1);
    auto status = LUA_YIELD;
    while (status == LUA_YIELD) {
      int nres;
      status = lua_resume(coroutine, nullptr, 0, &nres);
      if (status == LUA_ERRRUN) {
        auto errtext = lua_tostring(coroutine, -1);
        luaL_traceback(coroutine, coroutine, errtext, 1);
        errtext = lua_tostring(coroutine, -1);
        std::cerr << errtext << std::endl;
        std::abort();
      }
      else if (status == LUA_OK) {
        if(lua_isboolean(coroutine, -1)) {
          auto success = lua_toboolean(coroutine, -1);
          task_controller_grpc::TaskResult result;
          result.set_success(success);
          _status = result;
        } else if (lua_istable(coroutine, -1)) {
          lua_getfield(coroutine, -1, "success");
          auto success = lua_toboolean(coroutine, -1);
          task_controller_grpc::TaskResult result;
          result.set_success(success);
          _status = result;
        }
        return;
      }

      auto do_continue = false;
      lua_getglobal(coroutine, "debug");
      lua_getfield(coroutine, -1, "traceback");
      while (!do_continue) {
        lua_pushvalue(coroutine, -3);
        auto status = lua_pcall(coroutine, 0, 1, -2);
        if (status != LUA_OK) {
          auto msg = lua_tostring(coroutine, -1);
          std::cerr << msg << std::endl;
          std::abort();
        }
        do_continue = lua_toboolean(coroutine, -1);
        lua_pop(coroutine, 1);
        if (!do_continue) {
          lock.unlock();
          io_context.run_one();
          lock.lock();
          if(io_context.stopped()) {
            return;
          }
        }
      }
      lua_pop(coroutine, nres+2);
    }
  }
  void draw(SkCanvas* canvas, View view) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, context_ref);
    lua_getfield(L, -1, "on_render");
    if(lua_isnil(L, -1)) {
      lua_pop(L, 2);
      return;
    }

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

    if (lua_pcall(L, 2, 0, false)) {
      auto err_str = luaL_tolstring(L, -1, nullptr);
      std::cerr << err_str << std::endl;
      std::abort();
    }
    lua_pop(L, 1);
  }
  std::optional<task_controller_grpc::TaskResult> status() {
    return _status;
  }
};

LuaCoroutineTask::LuaCoroutineTask(boost::asio::io_context& io_context, lua_State* L) : impl(new Impl(io_context, L)) {}

void LuaCoroutineTask::touch(int x, int y) {
  return impl->touch(x, y);
}
void LuaCoroutineTask::gaze(int x, int y) {
  return impl->gaze(x, y);
}
void LuaCoroutineTask::update(std::unique_lock<std::mutex>& lock) {
  return impl->update(lock);
}
void LuaCoroutineTask::draw(SkCanvas* canvas, View view) {
  return impl->draw(canvas, view);
}

std::optional<task_controller_grpc::TaskResult> LuaCoroutineTask::status() {
  return impl->status();
}
