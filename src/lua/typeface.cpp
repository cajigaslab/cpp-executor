#include <lua/typeface.hpp>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include "include/core/SkTypeface.h"
#ifdef __clang__
#pragma clang diagnostic pop
#endif

int typeface_wrap(lua_State* L, sk_sp<SkTypeface> typeface) {
  auto userdata = static_cast<sk_sp<SkTypeface>*>(lua_newuserdata(L, sizeof(sk_sp<SkTypeface>)));
  userdata = new (userdata) sk_sp<SkTypeface>();

  luaL_setmetatable(L, "Thalamus.Typeface");

  *userdata = typeface;
  return 1;
}

static int typeface_gc(lua_State* L) {
  auto userdata = static_cast<sk_sp<SkTypeface>*>(luaL_checkudata(L, 1,  "Thalamus.Typeface"));
  userdata->reset();
  return 0;
}

static const struct luaL_Reg typeface_lib[] = {
  {nullptr, nullptr},
};

int luaopen_typeface(lua_State* L) {
  luaL_newmetatable(L, "Thalamus.Typeface");

  lua_pushcfunction(L, typeface_gc);
  lua_setfield(L, -2, "__gc");
  luaL_newlib(L, typeface_lib);
  return 1;
}
