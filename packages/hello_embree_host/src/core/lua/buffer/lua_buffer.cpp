#include "./lua_buffer.hpp"
#include "../lua_utils.hpp"
#include <cfloat>
#include <cstdint>
#include <cstdlib>
#include <cstring>

using namespace hello::lua::buffer;

namespace {
const char *const BUFFER_NAME = "Buffer";

int L_alloc(lua_State *L) {
  auto size = static_cast<int32_t>(luaL_checkinteger(L, 1));
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
    pBuffer->usage = nullptr;
  }

  luaL_setmetatable(L, BUFFER_NAME);
  return 1;
}

int L_fromString(lua_State *L) {
  auto str = luaL_checkstring(L, 1);
  auto size = static_cast<int32_t>(strlen(str));
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
    memcpy(pBuffer->data, str, size);
    pBuffer->usage = nullptr;
  }

  luaL_setmetatable(L, BUFFER_NAME);
  return 1;
}

int L_setUint8(lua_State *L) {
  auto pBuffer = static_cast<UDBuffer *>(luaL_checkudata(L, 1, BUFFER_NAME));
  if (pBuffer->usage != nullptr) {
    luaL_error(L, "This operations is not permitted.");
  }
  auto idx = luaL_checkinteger(L, 2);
  auto n = static_cast<uint8_t>(luaL_checkinteger(L, 3));
  luaL_argcheck(L, idx >= 0 && idx < pBuffer->size, 2,
                "setUint8: Out of range.");
  pBuffer->data[idx] = n;
  return 0;
}

int L_fillUint8(lua_State *L) {
  auto pBuffer = static_cast<UDBuffer *>(luaL_checkudata(L, 1, BUFFER_NAME));
  if (pBuffer->usage != nullptr) {
    luaL_error(L, "This operations is not permitted.");
  }

  auto value = static_cast<uint8_t>(luaL_checkinteger(L, 2));

  auto bytesPerElement = static_cast<uint8_t>(sizeof(uint8_t));
  auto end = pBuffer->size - bytesPerElement;
  for (auto offset = 0; offset <= end; offset += bytesPerElement) {
    *reinterpret_cast<uint8_t *>(pBuffer->data + offset) = value;
  }
  return 0;
}

int L_getUint8(lua_State *L) {
  auto pBuffer = static_cast<UDBuffer *>(luaL_checkudata(L, 1, BUFFER_NAME));
  if (pBuffer->usage != nullptr) {
    luaL_error(L, "This operations is not permitted.");
  }
  auto idx = luaL_checkinteger(L, 2);
  luaL_argcheck(L, idx >= 0 && idx < pBuffer->size, 2,
                "setUint8: Out of range.");
  lua_pushinteger(L, pBuffer->data[idx]);
  return 1;
}

int L_setFloat32(lua_State *L) {
  auto pBuffer = static_cast<UDBuffer *>(luaL_checkudata(L, 1, BUFFER_NAME));
  if (pBuffer->usage != nullptr) {
    luaL_error(L, "This operations is not permitted.");
  }
  auto idx = luaL_checkinteger(L, 2);
  auto n = static_cast<float>(luaL_checknumber(L, 3));
  luaL_argcheck(
      L, idx >= 0 && idx + static_cast<int32_t>(sizeof(float)) <= pBuffer->size,
      2, "setFloat32: Out of range.");
  *reinterpret_cast<float *>(pBuffer->data + idx) = n;
  return 0;
}

int L_fillFloat32(lua_State *L) {
  auto pBuffer = static_cast<UDBuffer *>(luaL_checkudata(L, 1, BUFFER_NAME));
  if (pBuffer->usage != nullptr) {
    luaL_error(L, "This operations is not permitted.");
  }

  auto value = static_cast<float>(luaL_checknumber(L, 2));

  auto bytesPerElement = static_cast<int32_t>(sizeof(float));
  auto end = pBuffer->size - bytesPerElement;
  for (auto offset = 0; offset <= end; offset += bytesPerElement) {
    *reinterpret_cast<float *>(pBuffer->data + offset) = value;
  }
  return 0;
}

int L_setFloat32FromArray(lua_State *L) {
  auto pBuffer = static_cast<UDBuffer *>(luaL_checkudata(L, 1, BUFFER_NAME));
  if (pBuffer->usage != nullptr) {
    luaL_error(L, "This operations is not permitted.");
  }

  auto offset = luaL_checkinteger(L, 2);

  luaL_checktype(L, 3, LUA_TTABLE);
  const auto bytesPerElement = static_cast<int32_t>(sizeof(float));
  const auto end = pBuffer->size - bytesPerElement;
  for (int i = 1;; i++) {
    lua_rawgeti(L, 3, i);
    if (lua_isnil(L, -1)) {
      lua_pop(L, 1);
      break;
    }

    if (offset <= end) {
      auto value = static_cast<float>(luaL_checknumber(L, -1));
      *reinterpret_cast<float *>(pBuffer->data + offset) = value;
      offset += bytesPerElement;
    }

    lua_pop(L, 1);
  }

  lua_pushinteger(L, offset);
  return 1;
}

int L_getFloat32(lua_State *L) {
  auto pBuffer = static_cast<UDBuffer *>(luaL_checkudata(L, 1, BUFFER_NAME));
  if (pBuffer->usage != nullptr) {
    luaL_error(L, "This operations is not permitted.");
  }
  auto idx = luaL_checkinteger(L, 2);
  luaL_argcheck(
      L, idx >= 0 && idx + static_cast<int32_t>(sizeof(float)) <= pBuffer->size,
      2, "getFloat32: Out of range.");
  lua_pushnumber(L, *reinterpret_cast<float *>(pBuffer->data + idx));
  return 1;
}

int L_getString(lua_State *L) {
  auto udBuffer = static_cast<UDBuffer *>(luaL_checkudata(L, 1, BUFFER_NAME));
  if (udBuffer->usage != nullptr) {
    luaL_error(L, "This operations is not permitted.");
  }
  lua_pushlstring(L, reinterpret_cast<char *>(udBuffer->data),
                  static_cast<size_t>(udBuffer->size));
  return 1;
}

int L___gc(lua_State *L) {
  auto pBuffer = static_cast<UDBuffer *>(luaL_checkudata(L, 1, BUFFER_NAME));

  free(pBuffer->usage);
  free(pBuffer->data);

  pBuffer->usage = nullptr;
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

  lua_pushcfunction(L, L_fromString);
  lua_setfield(L, -2, "fromString");

  return 1;
}
} // namespace

namespace hello::lua::buffer {
UDBuffer *alloc(lua_State *L, int size, const char *usage) {
  lua_pushcfunction(L, L_alloc);
  lua_pushinteger(L, size);

  if (lua::utils::docall(L, 1, 1) != LUA_OK) {
    return nullptr;
  }

  auto result = hello::lua::buffer::get(L, -1);
  if (usage != nullptr) {
    auto len = static_cast<size_t>(strlen(usage));
    auto dstUsage = static_cast<char *>(malloc(len + 1));
    memcpy(dstUsage, usage, len);
    result->usage = dstUsage;
  }
  return result;
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
  lua_pushcfunction(L, L_fillUint8);
  lua_setfield(L, -2, "fillUint8");
  lua_pushcfunction(L, L_getUint8);
  lua_setfield(L, -2, "getUint8");
  lua_pushcfunction(L, L_setFloat32);
  lua_setfield(L, -2, "setFloat32");
  lua_pushcfunction(L, L_setFloat32FromArray);
  lua_setfield(L, -2, "setFloat32FromArray");
  lua_pushcfunction(L, L_fillFloat32);
  lua_setfield(L, -2, "fillFloat32");
  lua_pushcfunction(L, L_getFloat32);
  lua_setfield(L, -2, "getFloat32");
  lua_pushcfunction(L, L_getString);
  lua_setfield(L, -2, "getString");
  lua_setfield(L, -2, "__index");

  luaL_requiref(L, "buffer", L_require, false);

  lua_pop(L, 2);
}
} // namespace hello::lua::buffer
