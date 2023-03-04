#ifndef __LUA_SDL2_IMAGE_HPP__
#define __LUA_SDL2_IMAGE_HPP__

#include "../lua_common.hpp"
#include "../lua_utils.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

namespace hello::lua::sdl2_image {
struct UDSDL_Surface {
  SDL_Surface *surface;
};

void openlibs(lua_State *L);
UDSDL_Surface *get(lua_State *L, int idx);
} // namespace hello::lua::sdl2_image
#endif