#include "./lua_buffer.hpp"
#include "../lua_utils.hpp"
#include <cstdlib>

using namespace hello::lua::buffer;

namespace {
const char *const BUFFER_NAME = "Buffer";

int L_alloc(lua_State *L) {
  auto size = static_cast<int32_t>(luaL_checknumber(L, 1));
  luaL_argcheck(L, size >= 0, 1, "Invalid buffer size");

  auto pBuffer = static_cast<UDBuffer *>(lua_newuserdata(L, sizeof(UDBuffer)));

  if (size == 0) {
    pBuffer->data = nullptr;
    pBuffer->size = 0;
  } else {
    auto p = static_cast<uint8_t *>(malloc(size));
    if (p == nullptr) {
      luaL_error(L, "Could not allocate buffer!");
    }
    pBuffer->size = size;
    pBuffer->data = p;
  }

  luaL_setmetatable(L, BUFFER_NAME);
  return 1;
}

int L_setUint8(lua_State *L) {
  auto pBuffer = static_cast<UDBuffer *>(luaL_checkudata(L, 1, BUFFER_NAME));
  auto idx = luaL_checkinteger(L, 2);
  auto n = static_cast<uint8_t>(luaL_checkinteger(L, 3));
  luaL_argcheck(L, idx >= 0 && idx < pBuffer->size, 2,
                "setUint8: Out of range.");
  pBuffer->data[idx] = n;
  return 0;
}

int L_getUint8(lua_State *L) {
  auto pBuffer = static_cast<UDBuffer *>(luaL_checkudata(L, 1, BUFFER_NAME));
  auto idx = luaL_checkinteger(L, 2);
  luaL_argcheck(L, idx >= 0 && idx < pBuffer->size, 2,
                "setUint8: Out of range.");
  lua_pushinteger(L, pBuffer->data[idx]);
  return 1;
}

int L___gc(lua_State *L) {
  auto pBuffer = static_cast<UDBuffer *>(luaL_checkudata(L, 1, BUFFER_NAME));

  free(pBuffer->data);

  pBuffer->data = nullptr;
  pBuffer->size = 0;
  return 0;
}

int L___len(lua_State *L) {
  auto pBuffer = static_cast<UDBuffer *>(luaL_checkudata(L, 1, BUFFER_NAME));
  lua_pushinteger(L, pBuffer->size);
  return 1;
}

int L_require(lua_State *L) {
  lua_newtable(L);

  lua_pushcfunction(L, L_alloc);
  lua_setfield(L, -2, "alloc");

  return 1;
}
} // namespace

namespace hello::lua::buffer {
UDBuffer *alloc(lua_State *L, int size) {
  lua_pushcfunction(L, L_alloc);
  lua_pushinteger(L, size);

  if (lua::utils::docall(L, 1, 1) != LUA_OK) {
    return nullptr;
  }

  return hello::lua::buffer::get(L, -1);
}

UDBuffer *get(lua_State *L, int idx) {
  return static_cast<UDBuffer *>(luaL_checkudata(L, idx, BUFFER_NAME));
}

void openlibs(lua_State *L) {
  luaL_newmetatable(L, BUFFER_NAME);
  lua_pushcfunction(L, L___gc);
  lua_setfield(L, -2, "__gc");
  lua_pushcfunction(L, L___len);
  lua_setfield(L, -2, "__len");
  lua_newtable(L);
  lua_pushcfunction(L, L_setUint8);
  lua_setfield(L, -2, "setUint8");
  lua_pushcfunction(L, L_getUint8);
  lua_setfield(L, -2, "getUint8");
  lua_setfield(L, -2, "__index");

  luaL_requiref(L, "buffer", L_require, false);

  lua_pop(L, 2);
}
} // namespace hello::lua::buffer
