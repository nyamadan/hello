#include "./lua_opengl.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

namespace {

int L_require(lua_State *L) {
  lua_newtable(L);
  return 1;
}
} // namespace

namespace hello::lua::opengl {
void openlibs(lua_State *L) {
  luaL_requiref(L, "opengl", L_require, false);
  lua_pop(L, 1);
}
} // namespace hello::lua::opengl