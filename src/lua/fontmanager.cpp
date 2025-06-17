#include <lua/fontmanager.hpp>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include "include/core/SkFont.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkFontStyle.h"
#include "include/ports/SkTypeface_win.h"
#include <lua/font.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <lua/typeface.hpp>

static int fontmanager_new(lua_State* L) {
  auto userdata = static_cast<sk_sp<SkFontMgr>*>(lua_newuserdata(L, sizeof(sk_sp<SkFontMgr>)));
  userdata = new (userdata) sk_sp<SkFontMgr>();

  luaL_setmetatable(L, "Thalamus.FontManager");

  *userdata = SkFontMgr_New_GDI();
  return 1;
}

static int fontmanager_matchFamilyStyle(lua_State* L) {
  auto fontMgr = static_cast<sk_sp<SkFontMgr>*>(luaL_checkudata(L, 1,  "Thalamus.FontManager"));

  const char* arg1 = nullptr;
  if(lua_type(L, 2) != LUA_TNIL) {
    arg1 = lua_tostring(L, 2);
  }

  auto fontstyle = *static_cast<SkFontStyle**>(luaL_checkudata(L, 3,  "Thalamus.FontStyle"));

  return typeface_wrap(L, (*fontMgr)->matchFamilyStyle(arg1, *fontstyle));
}

static int fontmanager_gc(lua_State* L) {
  auto userdata = static_cast<sk_sp<SkFontMgr>*>(luaL_checkudata(L, 1,  "Thalamus.FontManager"));
  userdata->reset();
  return 0;
}

static const struct luaL_Reg fontmanager_m[] = {
  {"matchFamilyStyle", fontmanager_matchFamilyStyle},
  {"__gc", fontmanager_gc},
  {nullptr, nullptr},
};

static const struct luaL_Reg fontmanager_lib[] = {
  {"new", fontmanager_new},
  {nullptr, nullptr},
};

int luaopen_fontmanager(lua_State* L) {
  luaL_newmetatable(L, "Thalamus.FontManager");

  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_setfuncs(L, fontmanager_m, 0);

  luaL_newlib(L, fontmanager_lib);
  return 1;
}
