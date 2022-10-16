#include "runner.hpp"

#include <iostream>

#include "lua/buffer/lua_buffer.hpp"
#include "lua/lua_utils.hpp"
#include "lua/opengl/lua_opengl.hpp"
#include "lua/sdl2/lua_sdl2.hpp"
#include "utils/arguments.hpp"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

using namespace hello;

namespace {
void initialize(lua_State *L) {
  luaL_openlibs(L);
  lua::utils::openlibs(L);
  lua::buffer::openlibs(L);
  lua::sdl2::openlibs(L);
  lua::opengl::openlibs(L);
}
void finalize(lua_State *L) { lua_close(L); }
void handleEvents(void *arg) {
  auto L = static_cast<lua_State *>(arg);
#if defined(__EMSCRIPTEN__)
  if (lua::utils::getFunction(L, "update") != LUA_TFUNCTION) {
    finalize(L);
    emscripten_cancel_main_loop();
    return;
  }
#endif
  lua::utils::report(L, lua::utils::docall(L, 0, LUA_MULTRET));
}
} // namespace

namespace hello::runner {
int run(int argc, char **argv) {
  const auto parsed = utils::arguments::parse(argc, argv, std::cout, std::cerr);

  auto L = luaL_newstate();
  initialize(L);
  lua::utils::report(L, (luaL_loadfile(L, parsed.file.c_str()) ||
                         lua::utils::docall(L, 0, LUA_MULTRET)));
#if defined(__EMSCRIPTEN__)
  emscripten_set_main_loop_arg(handleEvents, L, 0, true);
#else
  while (lua::utils::getFunction(L, "update") == LUA_TFUNCTION) {
    handleEvents(L);
  }
#endif
  finalize(L);
  return 0;
}
} // namespace hello::runner
