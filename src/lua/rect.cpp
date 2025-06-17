#include <lua/rect.hpp>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include "include/core/SkRect.h"
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <absl/strings/str_format.h>

static int rect_gc(lua_State* L) {
  auto userdata = static_cast<SkRect**>(luaL_checkudata(L, 1,  "Thalamus.Rect"));
  (*userdata)->~SkRect();
  return 0;
}

static int rect_tostring(lua_State* L) {
  auto userdata = *static_cast<SkRect**>(luaL_checkudata(L, 1,  "Thalamus.Rect"));
  auto text = absl::StrFormat("Rect{x=%f, y=%f, w=%f, h=%f}", userdata->x(), userdata->y(), userdata->width(), userdata->height());
  lua_pushstring(L, text.c_str());
  return 1;
}

static int rect_x(lua_State* L) {
  auto userdata = *static_cast<SkRect**>(luaL_checkudata(L, 1,  "Thalamus.Rect"));
  lua_pushnumber(L, double(userdata->x()));
  return 1;
}

static int rect_y(lua_State* L) {
  auto userdata = *static_cast<SkRect**>(luaL_checkudata(L, 1,  "Thalamus.Rect"));
  lua_pushnumber(L, double(userdata->y()));
  return 1;
}

static int rect_width(lua_State* L) {
  auto userdata = *static_cast<SkRect**>(luaL_checkudata(L, 1,  "Thalamus.Rect"));
  lua_pushnumber(L, double(userdata->width()));
  return 1;
}

static int rect_height(lua_State* L) {
  auto userdata = *static_cast<SkRect**>(luaL_checkudata(L, 1,  "Thalamus.Rect"));
  lua_pushnumber(L, double(userdata->height()));
  return 1;
}

static int rect_contains(lua_State* L) {
  auto userdata = *static_cast<SkRect**>(luaL_checkudata(L, 1,  "Thalamus.Rect"));

  auto x = luaL_checknumber(L, 2);
  auto y = luaL_checknumber(L, 3);

  auto contains = userdata->contains(float(x), float(y));
  lua_pushboolean(L, contains);

  return 1;
}

static int rect_makexywh(lua_State* L) {
  auto x = luaL_checknumber(L, 1);
  auto y = luaL_checknumber(L, 2);
  auto w = luaL_checknumber(L, 3);
  auto h = luaL_checknumber(L, 4);

  auto userdata = static_cast<SkRect**>(lua_newuserdata(L, sizeof(SkRect*)));
  *userdata = nullptr;

  luaL_setmetatable(L, "Thalamus.Rect");

  *userdata = new SkRect(SkRect::MakeXYWH(float(x), float(y), float(w), float(h)));
  return 1;
}

int luaopen_rect(lua_State* L) {
  luaL_newmetatable(L, "Thalamus.Rect");
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");

  lua_pushcfunction(L, rect_x);
  lua_setfield(L, -2, "x");
  lua_pushcfunction(L, rect_y);
  lua_setfield(L, -2, "y");
  lua_pushcfunction(L, rect_width);
  lua_setfield(L, -2, "width");
  lua_pushcfunction(L, rect_height);
  lua_setfield(L, -2, "height");
  lua_pushcfunction(L, rect_contains);
  lua_setfield(L, -2, "contains");
  lua_pushcfunction(L, rect_gc);
  lua_setfield(L, -2, "__gc");
  lua_pushcfunction(L, rect_tostring);
  lua_setfield(L, -2, "__tostring");

  lua_newtable(L);
  lua_pushcfunction(L, rect_makexywh);
  lua_setfield(L, -2, "MakeXYWH");

  return 1;
}
