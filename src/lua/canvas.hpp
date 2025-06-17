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
#include <include/core/SkCanvas.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

int canvas_wrap(lua_State* L, SkCanvas* canvas);
int luaopen_canvas(lua_State* L);
