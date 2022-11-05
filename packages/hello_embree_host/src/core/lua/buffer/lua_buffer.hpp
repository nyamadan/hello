#ifndef __LUA_BUFFER_HPP__
#define __LUA_BUFFER_HPP__

#include "../lua_common.hpp"
#include <cstdint>

namespace hello::lua::buffer {
struct Buffer {
  int32_t size;
  uint8_t *p;
};
int alloc(lua_State *L);
void openlibs(lua_State *L);
} // namespace hello::lua::buffer
#endif