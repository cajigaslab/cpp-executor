#pragma once

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}
#include <include/core/SkTypeface.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

int typeface_wrap(lua_State* L, sk_sp<SkTypeface> typeface);
int luaopen_typeface(lua_State* L);
