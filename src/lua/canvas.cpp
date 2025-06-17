#include <lua/canvas.hpp>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include <include/core/SkCanvas.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

int canvas_wrap(lua_State* L, SkCanvas* canvas) {
  auto userdata = static_cast<SkCanvas**>(lua_newuserdata(L, sizeof(SkCanvas*)));
  *userdata = nullptr;

  luaL_setmetatable(L, "Thalamus.Canvas");

  *userdata = canvas;
  return 1;
}

static int canvas_drawString(lua_State* L) {
  auto canvas = static_cast<SkCanvas**>(luaL_checkudata(L, 1,  "Thalamus.Canvas"));

  auto string = luaL_checkstring(L, 2);
  auto x = luaL_checknumber(L, 3);
  auto y = luaL_checknumber(L, 4);
  auto font = static_cast<SkFont**>(luaL_checkudata(L, 5,  "Thalamus.Font"));
  auto paint = static_cast<SkPaint**>(luaL_checkudata(L, 6,  "Thalamus.Paint"));

  (*canvas)->drawString(string, float(x), float(y), **font, **paint);

  return 0;
}

static int canvas_drawRect(lua_State* L) {
  auto canvas = static_cast<SkCanvas**>(luaL_checkudata(L, 1,  "Thalamus.Canvas"));

  auto rect = static_cast<SkRect**>(luaL_checkudata(L, 2,  "Thalamus.Rect"));
  auto paint = static_cast<SkPaint**>(luaL_checkudata(L, 3,  "Thalamus.Paint"));

  (*canvas)->drawRect(**rect, **paint);

  return 0;
}

static const struct luaL_Reg canvas_m[] = {
  {"drawString", canvas_drawString},
  {"drawRect", canvas_drawRect},
  {nullptr, nullptr},
};

static const struct luaL_Reg canvas_lib[] = {
  {nullptr, nullptr},
};

int luaopen_canvas(lua_State* L) {
  luaL_newmetatable(L, "Thalamus.Canvas");

  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_setfuncs(L, canvas_m, 0);

  luaL_newlib(L, canvas_lib);
  return 1;
}
