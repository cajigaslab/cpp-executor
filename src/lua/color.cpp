#include <lua/color.hpp>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include "include/core/SkColor.h"
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <absl/strings/str_format.h>

static int color4f_gc(lua_State* L) {
  auto userdata = static_cast<SkColor4f**>(luaL_checkudata(L, 1,  "Thalamus.Color4f"));
  (*userdata)->~SkColor4f();
  return 0;
}

static int color4f_tostring(lua_State* L) {
  auto userdata = *static_cast<SkColor4f**>(luaL_checkudata(L, 1,  "Thalamus.Color4f"));
  auto text = absl::StrFormat("Color{fR=%f, fG=%f, fB=%f, fA=%f}", userdata->fR, userdata->fG, userdata->fB, userdata->fA);
  lua_pushstring(L, text.c_str());
  return 1;
}

static int color4f_index(lua_State* L) {
  auto userdata = *static_cast<SkColor4f**>(luaL_checkudata(L, 1,  "Thalamus.Color4f"));
  std::string key = luaL_checkstring(L, -1);
  if(key == "fR") {
    lua_pushnumber(L, double(userdata->fR));
  } else if(key == "fG") {
    lua_pushnumber(L, double(userdata->fG));
  } else if(key == "fB") {
    lua_pushnumber(L, double(userdata->fB));
  } else if(key == "fA") {
    lua_pushnumber(L, double(userdata->fA));
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int color4f_newindex(lua_State* L) {
  auto userdata = *static_cast<SkColor4f**>(luaL_checkudata(L, 1,  "Thalamus.Color4f"));
  std::string key = luaL_checkstring(L, -2);
  float value = float(luaL_checknumber(L, -1));
  if(key == "fR") {
    userdata->fR = value;
  } else if(key == "fG") {
    userdata->fG = value;
  } else if(key == "fB") {
    userdata->fB = value;
  } else if(key == "fA") {
    userdata->fA = value;
  }
  return 0;
}

static int color4f_fromcolor(lua_State* L) {
  auto c = luaL_checkinteger(L, 1);

  auto color = SkColor4f::FromColor(uint32_t(c));
  return color4f_wrap(L, color);
}

static int color_setrgb(lua_State* L) {
  auto r = luaL_checkinteger(L, 1);
  auto g = luaL_checkinteger(L, 2);
  auto b = luaL_checkinteger(L, 3);

  auto color = SkColorSetRGB(uint32_t(r), uint32_t(g), uint32_t(b));
  lua_pushinteger(L, color);
  return 1;
}

static int color_setargb(lua_State* L) {
  auto a = luaL_checkinteger(L, 1);
  auto r = luaL_checkinteger(L, 2);
  auto g = luaL_checkinteger(L, 3);
  auto b = luaL_checkinteger(L, 4);

  auto color = SkColorSetARGB(uint32_t(a), uint32_t(r), uint32_t(g), uint32_t(b));
  lua_pushinteger(L, color);
  return 1;
}

int color4f_wrap(lua_State* L, const SkColor4f& color) {
  auto userdata = static_cast<SkColor4f**>(lua_newuserdata(L, sizeof(SkColor4f*)));
  *userdata = nullptr;

  luaL_setmetatable(L, "Thalamus.Color4f");

  *userdata = new SkColor4f(color);
  return 1;
}

int luaopen_color(lua_State* L) {
  lua_newtable(L);

#define ADD_COLOR(c) lua_pushinteger(L, SK_Color##c);\
  lua_setfield(L, -2, #c)

  ADD_COLOR(TRANSPARENT);
  ADD_COLOR(BLACK);
  ADD_COLOR(DKGRAY);
  ADD_COLOR(GRAY);
  ADD_COLOR(LTGRAY);
  ADD_COLOR(WHITE);
  ADD_COLOR(RED);
  ADD_COLOR(GREEN);
  ADD_COLOR(BLUE);
  ADD_COLOR(YELLOW);
  ADD_COLOR(CYAN);
  ADD_COLOR(MAGENTA);

  lua_pushcfunction(L, color_setrgb);
  lua_setfield(L, -2, "SetRGB");
  lua_pushcfunction(L, color_setargb);
  lua_setfield(L, -2, "SetARGB");

  lua_newtable(L);
  lua_pushvalue(L, -1);
  lua_setfield(L, -3, "Color4f");

  lua_pushcfunction(L, color4f_fromcolor);
  lua_setfield(L, -2, "FromColor");

  luaL_newmetatable(L, "Thalamus.Color4f");

  lua_pushcfunction(L, color4f_index);
  lua_setfield(L, -2, "__index");
  lua_pushcfunction(L, color4f_newindex);
  lua_setfield(L, -2, "__newindex");

  lua_pushcfunction(L, color4f_gc);
  lua_setfield(L, -2, "__gc");
  lua_pushcfunction(L, color4f_tostring);
  lua_setfield(L, -2, "__tostring");
  lua_pop(L, 2);
  return 1;
}
