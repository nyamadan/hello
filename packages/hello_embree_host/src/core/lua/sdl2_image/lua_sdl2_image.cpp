#include "./lua_sdl2_image.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace {
struct UDSDL_Surface {
  SDL_Surface *surface;
};

const char *const SDL_SURFACE_NAME = "SDL_Surface";

int L_Load(lua_State *L) {
  auto filename = static_cast<const char *>(luaL_checkstring(L, 1));
  auto surface = IMG_Load(filename);

  if (surface != nullptr) {
    auto pudSurface =
        static_cast<UDSDL_Surface *>(lua_newuserdata(L, sizeof(UDSDL_Surface)));
    pudSurface->surface = surface;
    luaL_setmetatable(L, SDL_SURFACE_NAME);
  } else {
    lua_pushnil(L);
  }
  return 1;
}

int L_getInfo(lua_State *L) {
  auto pudSurface =
      static_cast<UDSDL_Surface *>(luaL_checkudata(L, 1, SDL_SURFACE_NAME));
  luaL_argcheck(L, pudSurface != nullptr, 1, "already freed.");

  lua_newtable(L);

  lua_pushinteger(L, pudSurface->surface->w);
  lua_setfield(L, -2, "w");

  lua_pushinteger(L, pudSurface->surface->h);
  lua_setfield(L, -2, "h");

  lua_newtable(L);
  lua_pushinteger(L, pudSurface->surface->format->BitsPerPixel);
  lua_setfield(L, -2, "BitsPerPixel");
  lua_pushinteger(L, pudSurface->surface->format->BytesPerPixel);
  lua_setfield(L, -2, "BytesPerPixel");
  lua_pushinteger(L, pudSurface->surface->format->format);
  lua_setfield(L, -2, "format");

  lua_setfield(L, -2, "format");
  return 1;
}

int L_freeSurface(lua_State *L) {
  auto pudSurface =
      static_cast<UDSDL_Surface *>(luaL_checkudata(L, 1, SDL_SURFACE_NAME));
  SDL_FreeSurface(pudSurface->surface);
  pudSurface->surface = nullptr;
  return 0;
}

int L_require(lua_State *L) {
  lua_newtable(L);
  lua_pushcfunction(L, L_Load);
  lua_setfield(L, -2, "load");
  return 1;
}
} // namespace

namespace hello::lua::sdl2_image {
void openlibs(lua_State *L) {
  luaL_newmetatable(L, SDL_SURFACE_NAME);
  lua_pushcfunction(L, L_freeSurface);
  lua_setfield(L, -2, "__gc");
  lua_newtable(L);
  lua_pushcfunction(L, L_freeSurface);
  lua_setfield(L, -2, "free");
  lua_pushcfunction(L, L_getInfo);
  lua_setfield(L, -2, "getInfo");
  lua_setfield(L, -2, "__index");

  luaL_requiref(L, "sdl2_image", L_require, false);
  lua_pop(L, 2);
}
} // namespace hello::lua::sdl2_image