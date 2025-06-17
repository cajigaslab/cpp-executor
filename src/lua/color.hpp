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
#include <include/core/SkColor.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

int color4f_wrap(lua_State* L, const SkColor4f& color);
int luaopen_color(lua_State* L);
