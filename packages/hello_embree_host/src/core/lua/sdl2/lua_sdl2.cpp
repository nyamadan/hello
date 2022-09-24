#include "./lua_sdl2.hpp"

#include <SDL2/SDL.h>

namespace {
int L_SDL_Init(lua_State *L) {
  auto flags = static_cast<Uint32>(luaL_checkinteger(L, 1));
  auto result = SDL_Init(flags);
  lua_pushinteger(L, result);
  return 1;
}

int L_SDL_Quit(lua_State *) {
  SDL_Quit();
  return 0;
}

int L_SDL_GetError(lua_State *L) {
  const auto error = SDL_GetError();
  lua_pushstring(L, error);
  return 1;
}

int L_SDL_CreateWindow(lua_State *L) {
  const char *const title = luaL_checkstring(L, 1);
  const auto x = static_cast<int32_t>(luaL_checkinteger(L, 2));
  const auto y = static_cast<int32_t>(luaL_checkinteger(L, 3));
  const auto w = static_cast<int32_t>(luaL_checkinteger(L, 4));
  const auto h = static_cast<int32_t>(luaL_checkinteger(L, 5));
  const auto flags = static_cast<uint32_t>(luaL_checkinteger(L, 6));
  auto window = SDL_CreateWindow(title, x, y, w, h, flags);
  lua_pushlightuserdata(L, window);
  luaL_newmetatable(L, "SDL_Window");
  lua_setmetatable(L, -2);
  return 1;
}

int L_SDL_DestroyWindow(lua_State *L) {
  auto window = static_cast<SDL_Window *>(luaL_checkudata(L, 1, "SDL_Window"));
  SDL_DestroyWindow(window);
  return 0;
}

int L_SDL_CreateRenderer(lua_State *L) {
  SDL_Window *window =
      reinterpret_cast<SDL_Window *>(luaL_checkudata(L, 1, "SDL_Window"));
  const int32_t index = static_cast<int32_t>(luaL_checkinteger(L, 2));
  const uint32_t flags = static_cast<uint32_t>(luaL_checkinteger(L, 3));
  auto renderer = SDL_CreateRenderer(window, index, flags);
  lua_pushlightuserdata(L, renderer);
  luaL_newmetatable(L, "SDL_Renderer");
  lua_setmetatable(L, -2);
  return 1;
}

int L_SDL_DestroyRenderer(lua_State *L) {
  auto renderer =
      static_cast<SDL_Renderer *>(luaL_checkudata(L, 1, "SDL_Renderer"));
  SDL_DestroyRenderer(renderer);
  return 0;
}

int L_SDL_PollEvent(lua_State *L) {
  SDL_Event event = {0};
  int result = SDL_PollEvent(&event);

  if (result) {
    lua_newtable(L);
    lua_pushinteger(L, event.type);
    lua_setfield(L, -2, "type");
  } else {
    lua_pushnil(L);
  }

  return 1;
}

int L_SDL_Delay(lua_State *L) {
  const auto ms = static_cast<Uint32>(luaL_checkinteger(L, 1));
  SDL_Delay(ms);
  return 0;
}

int L_SDL_GL_SetAttribute(lua_State *L) {
  const auto attr = static_cast<SDL_GLattr>(luaL_checkinteger(L, 1));
  const auto value = static_cast<int>(luaL_checkinteger(L, 2));
  auto result = SDL_GL_SetAttribute(attr, value);
  lua_pushinteger(L, result);
  return 1;
}

int L_require(lua_State *L) {
  lua_newtable(L);

  lua_pushinteger(L, SDL_INIT_VIDEO);
  lua_setfield(L, -2, "INIT_VIDEO");

  lua_pushinteger(L, SDL_INIT_TIMER);
  lua_setfield(L, -2, "INIT_TIMER");

  lua_pushinteger(L, SDL_WINDOWPOS_UNDEFINED);
  lua_setfield(L, -2, "WINDOWPOS_UNDEFINED");

  lua_pushinteger(L, SDL_RENDERER_ACCELERATED);
  lua_setfield(L, -2, "RENDERER_ACCELERATED");

  lua_pushinteger(L, SDL_QUIT);
  lua_setfield(L, -2, "QUIT");

  lua_pushinteger(L, SDL_GL_CONTEXT_FLAGS);
  lua_setfield(L, -2, "GL_CONTEXT_FLAGS");

  lua_pushinteger(L, SDL_GL_CONTEXT_PROFILE_MASK);
  lua_setfield(L, -2, "GL_CONTEXT_PROFILE_MASK");

  lua_pushinteger(L, SDL_GL_CONTEXT_PROFILE_CORE);
  lua_setfield(L, -2, "GL_CONTEXT_PROFILE_CORE");

  lua_pushinteger(L, SDL_GL_CONTEXT_MAJOR_VERSION);
  lua_setfield(L, -2, "GL_CONTEXT_MAJOR_VERSION");

  lua_pushinteger(L, SDL_GL_CONTEXT_MINOR_VERSION);
  lua_setfield(L, -2, "GL_CONTEXT_MINOR_VERSION");

  lua_pushcfunction(L, L_SDL_Init);
  lua_setfield(L, -2, "Init");

  lua_pushcfunction(L, L_SDL_Quit);
  lua_setfield(L, -2, "Quit");

  lua_pushcfunction(L, L_SDL_GetError);
  lua_setfield(L, -2, "GetError");

  lua_pushcfunction(L, L_SDL_CreateWindow);
  lua_setfield(L, -2, "CreateWindow");

  lua_pushcfunction(L, L_SDL_DestroyWindow);
  lua_setfield(L, -2, "DestroyWindow");

  lua_pushcfunction(L, L_SDL_CreateRenderer);
  lua_setfield(L, -2, "CreateRenderer");

  lua_pushcfunction(L, L_SDL_DestroyRenderer);
  lua_setfield(L, -2, "DestroyRenderer");

  lua_pushcfunction(L, L_SDL_PollEvent);
  lua_setfield(L, -2, "PollEvent");

  lua_pushcfunction(L, L_SDL_Delay);
  lua_setfield(L, -2, "Delay");

  lua_pushcfunction(L, L_SDL_GL_SetAttribute);
  lua_setfield(L, -2, "GL_SetAttribute");

  return 1;
}
} // namespace

namespace hello::lua::sdl2 {
void openlibs(lua_State *L) {
  luaL_requiref(L, "sdl2", L_require, false);
  lua_pop(L, 1);
}
} // namespace hello::lua::sdl2