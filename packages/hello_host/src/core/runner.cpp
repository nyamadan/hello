#include "runner.hpp"
#include "lua/glslang/lua_glslang.hpp"
#include "lua/lua_utils.hpp"
#include "lua/opengl/lua_opengl.hpp"
#include "lua/sdl2/lua_sdl2.hpp"
#include "lua/sdl2_image/lua_sdl2_image.hpp"
#include "lua/spv_cross/lua_spv_cross.hpp"
#include <iostream>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

using namespace hello;

namespace {
void initialize(lua_State *L) {
  luaL_openlibs(L);
  lua::utils::openlibs(L);
  lua::sdl2::openlibs(L);
  lua::opengl::openlibs(L);
  lua::glslang::openlibs(L);
  lua::spv_cross::openlibs(L);
  lua::sdl2_image::openlibs(L);
}

void finalize(lua_State *L) {
  if (lua::utils::getFunction(L, "finalize") == LUA_TFUNCTION) {
    lua::utils::report(L, lua::utils::docall(L, 0, LUA_MULTRET));
  }
  lua_close(L);
}

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
  if (argc < 2) {
    printf("usage: %s filename\n", argv[0]);
    return -1;
  }

  const auto file = argv[1];
  auto L = luaL_newstate();
  initialize(L);
  lua::utils::report(
      L, (luaL_loadfile(L, file) || lua::utils::docall(L, 0, LUA_MULTRET)));
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
