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

class SkFontStyle;

int fontstyle_wrap(lua_State* L, const SkFontStyle& font);
int luaopen_fontstyle(lua_State* L);
