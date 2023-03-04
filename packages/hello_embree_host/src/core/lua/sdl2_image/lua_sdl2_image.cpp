#include "./lua_sdl2_image.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <memory>

namespace {
using hello::lua::sdl2_image::UDSDL_Surface;

const char *const SDL_SURFACE_NAME = "SDL_Surface";

int L_load(lua_State *L) {
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

int L_lockSurface(lua_State *L) {
  auto pudSurface =
      static_cast<UDSDL_Surface *>(luaL_checkudata(L, 1, SDL_SURFACE_NAME));
  luaL_argcheck(L, pudSurface->surface != nullptr, 1, "already freed.");
  auto surface = pudSurface->surface;
  SDL_LockSurface(surface);
  return 0;
}

int L_unlockSurface(lua_State *L) {
  auto pudSurface =
      static_cast<UDSDL_Surface *>(luaL_checkudata(L, 1, SDL_SURFACE_NAME));
  luaL_argcheck(L, pudSurface->surface != nullptr, 1, "already freed.");
  auto surface = pudSurface->surface;
  SDL_UnlockSurface(surface);
  return 0;
}

int L_flipVertical(lua_State *L) {
  auto pudSurface =
      static_cast<UDSDL_Surface *>(luaL_checkudata(L, 1, SDL_SURFACE_NAME));
  luaL_argcheck(L, pudSurface->surface != nullptr, 1, "already freed.");
  auto surface = pudSurface->surface;

  int pitch = surface->pitch;   // row size
  char *temp = new char[pitch]; // intermediate buffer
  char *pixels = (char *)surface->pixels;

  for (int i = 0; i < surface->h / 2; ++i) {
    // get pointers to the two rows to swap
    char *row1 = pixels + i * pitch;
    char *row2 = pixels + (surface->h - i - 1) * pitch;

    // swap rows
    ::memcpy(temp, row1, pitch);
    ::memcpy(row1, row2, pitch);
    ::memcpy(row2, temp, pitch);
  }

  delete[] temp;

  return 0;
}

int L_getInfoSurface(lua_State *L) {
  auto pudSurface =
      static_cast<UDSDL_Surface *>(luaL_checkudata(L, 1, SDL_SURFACE_NAME));
  luaL_argcheck(L, pudSurface->surface != nullptr, 1, "already freed.");

  lua_newtable(L);

  lua_pushinteger(L, pudSurface->surface->w);
  lua_setfield(L, -2, "w");

  lua_pushinteger(L, pudSurface->surface->h);
  lua_setfield(L, -2, "h");

  lua_pushinteger(L, pudSurface->surface->pitch);
  lua_setfield(L, -2, "pitch");

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
  lua_pushcfunction(L, L_load);
  lua_setfield(L, -2, "load");
  return 1;
}
} // namespace

namespace hello::lua::sdl2_image {
UDSDL_Surface *get(lua_State *L, int idx) {
  auto pudSurface =
      static_cast<UDSDL_Surface *>(luaL_testudata(L, idx, SDL_SURFACE_NAME));
  return pudSurface;
}

void openlibs(lua_State *L) {
  luaL_newmetatable(L, SDL_SURFACE_NAME);
  lua_pushcfunction(L, L_freeSurface);
  lua_setfield(L, -2, "__gc");
  lua_newtable(L);
  lua_pushcfunction(L, L_freeSurface);
  lua_setfield(L, -2, "free");
  lua_pushcfunction(L, L_getInfoSurface);
  lua_setfield(L, -2, "getInfo");
  lua_pushcfunction(L, L_lockSurface);
  lua_setfield(L, -2, "lock");
  lua_pushcfunction(L, L_unlockSurface);
  lua_setfield(L, -2, "unlock");
  lua_pushcfunction(L, L_flipVertical);
  lua_setfield(L, -2, "flipVertical");
  lua_setfield(L, -2, "__index");

  luaL_requiref(L, "sdl2_image", L_require, false);
  lua_pop(L, 2);
}
} // namespace hello::lua::sdl2_image