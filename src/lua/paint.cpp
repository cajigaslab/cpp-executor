#include <lua/paint.hpp>
#include <lua/color.hpp>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include "include/core/SkPaint.h"
#include "include/core/SkColor.h"
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <absl/strings/str_format.h>

static int paint_new(lua_State* L) {
  auto color = static_cast<SkColor4f**>(luaL_checkudata(L, -1,  "Thalamus.Color4f"));

  auto userdata = static_cast<SkPaint**>(lua_newuserdata(L, sizeof(SkPaint*)));
  *userdata = nullptr;

  luaL_setmetatable(L, "Thalamus.Paint");

  *userdata = new SkPaint(**color);
  return 1;
}

static int paint_gc(lua_State* L) {
  auto userdata = static_cast<SkPaint**>(luaL_checkudata(L, 1,  "Thalamus.Paint"));
  (*userdata)->~SkPaint();
  return 0;
}

static int paint_tostring(lua_State* L) {
  luaL_checkudata(L, 1,  "Thalamus.Paint");

  lua_getfield(L, -1, "getColor4f");
  lua_pushvalue(L, -2);
  lua_call(L, 1, 1);

  auto color = luaL_tolstring(L, -1, nullptr);

  auto text = absl::StrFormat("Paint{color=%s}", color);
  lua_pushstring(L, text.c_str());
  return 1;
}

static int paint_getColor4f(lua_State* L) {
  auto userdata = *static_cast<SkPaint**>(luaL_checkudata(L, 1,  "Thalamus.Paint"));

  auto color = userdata->getColor4f();
  return color4f_wrap(L, color);
}

static int paint_setColor4f(lua_State* L) {
  auto userdata = *static_cast<SkPaint**>(luaL_checkudata(L, 1,  "Thalamus.Paint"));
  auto color = *static_cast<SkColor4f**>(luaL_checkudata(L, 2,  "Thalamus.Color4f"));

  userdata->setColor4f(*color);
  return 0;
}

static const struct luaL_Reg paint_m[] = {
  {"getColor4f", paint_getColor4f},
  {"setColor4f", paint_setColor4f},
  {"__gc", paint_gc},
  {"__tostring", paint_tostring},
  {nullptr, nullptr},
};

static const struct luaL_Reg paint_lib[] = {
  {"new", paint_new},
  {nullptr, nullptr},
};

int luaopen_paint(lua_State* L) {
  luaL_newmetatable(L, "Thalamus.Paint");

  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_setfuncs(L, paint_m, 0);

  luaL_newlib(L, paint_lib);
  return 1;
}
