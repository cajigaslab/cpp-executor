#include <task.hpp>
#include <simple_task.hpp>
#include <lua_task.hpp>
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

Context::Context() : device(), engine(device()) {}

double Context::get_uniform_field(const std::string& field) {
  return get_uniform(config[field]);
}

double Context::get_uniform(const Json::Value& value) {
  auto min = value["min"].asDouble();
  auto max = value["max"].asDouble();
  std::uniform_real_distribution<> distrib(min, max);
  return distrib(engine);
}

SkColor Context::get_color_field(const std::string& field) {
  return get_color(config[field]);
}

SkColor Context::get_color(const Json::Value& value) {
  auto r = value[0].asInt();
  auto g = value[1].asInt();
  auto b = value[2].asInt();
  return SkColorSetRGB(uint32_t(r), uint32_t(g), uint32_t(b));
}

static void json_to_lua(lua_State* L, const Json::Value& json) {
  lua_newtable(L);
  auto offset = json.isArray() ? 1 : 0;
  for(auto i = json.begin();i != json.end();++i) {
    auto key = i.key();
    if(key.isIntegral()) {
      lua_pushinteger(L, key.asInt() + offset);
    } else if (key.isString()) {
      lua_pushstring(L, key.asString().c_str());
    }
    if(i->isIntegral()) {
      lua_pushinteger(L, i->asInt());
    } else if(i->isNumeric()) {
      lua_pushnumber(L, i->asDouble());
    } else if(i->isString()) {
      lua_pushstring(L, i->asString().c_str());
    } else if(i->isBool()) {
      lua_pushboolean(L, i->asBool());
    } else {
      json_to_lua(L, *i);
    }
    
    lua_settable(L, -3);
  }
}

Task* make_task(boost::asio::io_context& io_context, Context& context, lua_State* L) {
  lua_getglobal(L, "make_task");
  auto v = lua_isnil(L, -1);
  json_to_lua(L, context.config);
  auto err = lua_pcall(L, 1, 2, 0);
  if(err) {
    auto err_str = luaL_tolstring(L, -1, nullptr);
    std::cerr << err_str << std::endl;
    std::abort();
  } else if(lua_type(L, -2) != LUA_TNIL) {
    if(lua_isfunction(L, -2)) {
      return new LuaCoroutineTask(io_context, L);
    }
    lua_pop(L, 1);
    auto ref = luaL_ref(L, LUA_REGISTRYINDEX);
    lua_pop(L, 1);
    return new LuaTask(L, ref);
  }

  std::string task_type = context.config["task_type"].asString();
  if(task_type == "simple") {
    return new SimpleTask(context);
  }
  std::cout << "Unexpected task_type: " <<  task_type;
  std::abort();
}
