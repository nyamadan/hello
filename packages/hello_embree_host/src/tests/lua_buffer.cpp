#include <gtest/gtest.h>

#include "../core/lua/buffer/lua_buffer.hpp"
#include "../core/lua/lua_common.hpp"
#include "../core/lua/lua_utils.hpp"

class LuaBuffer_Test : public ::testing::Test {
protected:
  lua_State *L;

  virtual void SetUp() {
    L = luaL_newstate();
    luaL_openlibs(L);
    hello::lua::buffer::openlibs(L);
  }
  virtual void TearDown() {
    if (L != nullptr) {
      lua_close(L);
      L = nullptr;
    }
  }
};

TEST_F(LuaBuffer_Test, NewBuffer) {
  ASSERT_EQ(hello::lua::utils::dostring(L, "local buffer = require('buffer');"
                                           "local b = buffer.alloc(4);"
                                           "return #b, b"),
            LUA_OK)
      << lua_tostring(L, -1);

  luaL_checkudata(L, -1, "Buffer");
  ASSERT_EQ(luaL_checkinteger(L, -2), 4);
}

TEST_F(LuaBuffer_Test, SetGetUint8) {
  ASSERT_EQ(hello::lua::utils::dostring(L, "local buffer = require('buffer');"
                                           "local b = buffer.alloc(1);"
                                           "b:setUint8(0, 10);"
                                           "return b:getUint8(0)"),
            LUA_OK)
      << lua_tostring(L, -1);
  ASSERT_EQ(luaL_checkinteger(L, -1), 10);

  ASSERT_NE(hello::lua::utils::dostring(L, "local buffer = require('buffer');"
                                           "local b = buffer.alloc(1);"
                                           "b:setUint8(1, 10);"),
            LUA_OK);
  ASSERT_NE(hello::lua::utils::dostring(L, "local buffer = require('buffer');"
                                           "local b = buffer.alloc(1);"
                                           "b:setUint8(-1, 10);"),
            LUA_OK);

  ASSERT_NE(hello::lua::utils::dostring(L, "local buffer = require('buffer');"
                                           "local b = buffer.alloc(1);"
                                           "b:getUint8(1);"),
            LUA_OK);
  ASSERT_NE(hello::lua::utils::dostring(L, "local buffer = require('buffer');"
                                           "local b = buffer.alloc(1);"
                                           "b:getUint8(-1);"),
            LUA_OK);
}