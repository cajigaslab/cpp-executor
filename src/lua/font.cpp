#include <lua/font.hpp>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include "include/core/SkFont.h"
#ifdef __clang__
#pragma clang diagnostic pop
#endif

static int font_new(lua_State* L) {
  auto userdata = static_cast<SkFont**>(lua_newuserdata(L, sizeof(SkFont*)));

  auto typeface = static_cast<sk_sp<SkTypeface>*>(luaL_checkudata(L, 1,  "Thalamus.Typeface"));
  auto size = luaL_checknumber(L, 2);

  *userdata = nullptr;

  luaL_setmetatable(L, "Thalamus.Font");

  *userdata = new SkFont(*typeface, float(size));
  return 1;
}

static int font_gc(lua_State* L) {
  auto userdata = *static_cast<SkFont**>(luaL_checkudata(L, 1,  "Thalamus.Font"));
  userdata->~SkFont();
  return 0;
}

static const struct luaL_Reg font_lib[] = {
  {"new", font_new},
  {nullptr, nullptr},
};

int luaopen_font(lua_State* L) {
  luaL_newmetatable(L, "Thalamus.Font");

  lua_pushcfunction(L, font_gc);
  lua_setfield(L, -2, "__gc");
  luaL_newlib(L, font_lib);
  return 1;
}
