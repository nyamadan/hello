#include "./lua_opengl.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

namespace {
int L_glClearColor(lua_State *L) {
  auto r = static_cast<GLclampf>(luaL_checknumber(L, 1));
  auto g = static_cast<GLclampf>(luaL_checknumber(L, 2));
  auto b = static_cast<GLclampf>(luaL_checknumber(L, 3));
  auto a = static_cast<GLclampf>(luaL_checknumber(L, 4));
  glClearColor(r, g, b, a);
  return 0;
}

int L_glClearDepth(lua_State *L) {
  auto depth = static_cast<GLclampd>(luaL_checknumber(L, 1));
  glClearDepth(depth);
  return 0;
}

int L_glClear(lua_State *L) {
  auto flags = static_cast<GLbitfield>(luaL_checknumber(L, 1));
  glClear(flags);
  return 0;
}

int L_require(lua_State *L) {
  lua_newtable(L);

  lua_pushinteger(L, GL_COLOR_BUFFER_BIT);
  lua_setfield(L, -2, "COLOR_BUFFER_BIT");

  lua_pushinteger(L, GL_DEPTH_BUFFER_BIT);
  lua_setfield(L, -2, "DEPTH_BUFFER_BIT");

  lua_pushcfunction(L, L_glClearColor);
  lua_setfield(L, -2, "ClearColor");

  lua_pushcfunction(L, L_glClearDepth);
  lua_setfield(L, -2, "ClearDepth");

  lua_pushcfunction(L, L_glClear);
  lua_setfield(L, -2, "Clear");

  return 1;
}
} // namespace

namespace hello::lua::opengl {
void openlibs(lua_State *L) {
  luaL_requiref(L, "opengl", L_require, false);
  lua_pop(L, 1);
}
} // namespace hello::lua::opengl