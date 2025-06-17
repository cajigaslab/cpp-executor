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
#ifdef __clang__
#pragma clang diagnostic pop
#endif

class SkFont;

int luaopen_font(lua_State* L);
