#include <lua/fontstyle.hpp>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include "include/core/SkFontStyle.h"
#ifdef __clang__
#pragma clang diagnostic pop
#endif

static int fontstyle_new(lua_State* L) {
  return fontstyle_wrap(L, SkFontStyle());
}

int fontstyle_wrap(lua_State* L, const SkFontStyle& style) {
  auto userdata = static_cast<SkFontStyle**>(lua_newuserdata(L, sizeof(SkFontStyle*)));
  *userdata = nullptr;

  luaL_setmetatable(L, "Thalamus.FontStyle");

  *userdata = new SkFontStyle(style);
  return 1;
}

static int font_gc(lua_State* L) {
  auto userdata = *static_cast<SkFontStyle**>(luaL_checkudata(L, 1,  "Thalamus.FontStyle"));
  userdata->~SkFontStyle();
  return 0;
}

static const struct luaL_Reg font_lib[] = {
  {"new", fontstyle_new},
  {nullptr, nullptr},
};

int luaopen_fontstyle(lua_State* L) {
  luaL_newmetatable(L, "Thalamus.FontStyle");

  lua_pushcfunction(L, font_gc);
  lua_setfield(L, -2, "__gc");
  luaL_newlib(L, font_lib);
  return 1;
}
