#include <SDL2/SDL.h>
#include <iostream>

#include "core/lua/lua_utils.hpp"
#include "core/lua/sdl2/lua_sdl2.hpp"
#include "core/utils/arguments.hpp"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

namespace {
void initialize(lua_State *L) {
  luaL_openlibs(L);
  hello::lua::sdl2::openlibs(L);
}
void finalize(lua_State *L) { lua_close(L); }
void handleEvents(void *arg) {
  auto L = static_cast<lua_State *>(arg);

#if defined(__EMSCRIPTEN__)
  if (hello::lua::utils::getFunction(L, "update") == LUA_TNIL) {
    lua_pop(L, 1);

    emscripten_cancel_main_loop();
    return;
  }
#endif
}

int run(const hello::utils::arguments::ParsedArguments &parsed) {
  auto L = luaL_newstate();
  initialize(L);
  hello::lua::utils::report(L, (luaL_loadfile(L, parsed.file.c_str()) ||
                                hello::lua::utils::docall(L, 0, LUA_MULTRET)));
#if defined(__EMSCRIPTEN__)
  emscripten_set_main_loop_arg(handleEvents, L, 0, true);
#else
  while (hello::lua::utils::getFunction(L, "update") != LUA_TNIL) {
    lua_pop(L, 1);
    handleEvents(L);
  }
#endif
  finalize(L);
  return 0;
}
} // namespace

int main(int argc, char **argv) {
  return run(hello::utils::arguments::parse(argc, argv, std::cout, std::cerr));
}
