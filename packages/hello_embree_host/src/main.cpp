#include <SDL2/SDL.h>
#include <iostream>

#include "core/lua/lua_utils.hpp"
#include "core/lua/sdl2/lua_sdl2.hpp"
#include "core/utils/arguments.hpp"

namespace {
int run(const hello::utils::arguments::ParsedArguments &parsed) {
  auto L = luaL_newstate();
  luaL_openlibs(L);
  hello::lua::sdl2::openlibs(L);
  hello::lua::report(L, (luaL_loadfile(L, parsed.file.c_str()) ||
                         hello::lua::docall(L, 0, LUA_MULTRET)));
  lua_close(L);
  return 0;
}
} // namespace

int main(int argc, char **argv) {
  return run(hello::utils::arguments::parse(argc, argv, std::cout, std::cerr));
}
