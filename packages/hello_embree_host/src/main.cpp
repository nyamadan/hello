#include <SDL2/SDL.h>
#include <iostream>

#include "lua/lua_utils.hpp"
#include "lua/sdl2/lua_sdl2.hpp"
#include "utils/arguments.hpp"

namespace {
void run(const hello::utils::arguments::ParsedArguments &parsed) {
  auto L = luaL_newstate();
  luaL_openlibs(L);
  hello::lua::sdl2::openlibs(L);
  hello::lua::report(L, (luaL_loadfile(L, parsed.file.c_str()) ||
                         hello::lua::docall(L, 0, LUA_MULTRET)));
  lua_close(L);
}
} // namespace

int main(int argc, char **argv) {
  run(hello::utils::arguments::parse(argc, argv, std::cout, std::cerr));
  return 0;
}
