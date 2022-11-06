#ifndef __LUA_BUFFER_HPP__
#define __LUA_BUFFER_HPP__

#include "../lua_common.hpp"
#include <cstdint>

namespace hello::lua::buffer {
struct UDBuffer {
  int32_t size;
  uint8_t *data;
};
UDBuffer *alloc(lua_State *L, int size);
UDBuffer *get(lua_State *L, int idx);
void openlibs(lua_State *L);
} // namespace hello::lua::buffer
#endif