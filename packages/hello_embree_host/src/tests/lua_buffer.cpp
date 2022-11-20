#include "../core/lua/buffer/lua_buffer.hpp"
#include "../core/lua/lua_common.hpp"
#include "../core/lua/lua_utils.hpp"
#include <gtest/gtest.h>

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

  auto udBuffer = hello::lua::buffer::get(L, -1);
  ASSERT_NE(udBuffer, nullptr);
  ASSERT_EQ(udBuffer->size, 4);
  ASSERT_EQ(luaL_checkinteger(L, -2), 4);
}

TEST_F(LuaBuffer_Test, AllocAndGet) {
  auto udBuffer = hello::lua::buffer::alloc(L, 12);
  ASSERT_EQ(udBuffer->size, 12);
  ASSERT_EQ(hello::lua::buffer::get(L, -1), udBuffer);
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

TEST_F(LuaBuffer_Test, SetGetFloat32) {
  ASSERT_EQ(hello::lua::utils::dostring(L, "local buffer = require('buffer');"
                                           "local b = buffer.alloc(4);"
                                           "b:setFloat32(0, 10.0);"
                                           "return b:getFloat32(0)"),
            LUA_OK)
      << lua_tostring(L, -1);
  ASSERT_EQ(luaL_checknumber(L, -1), 10.0);

  ASSERT_NE(hello::lua::utils::dostring(L, "local buffer = require('buffer');"
                                           "local b = buffer.alloc(4);"
                                           "b:setFloat32(1, 10.0);"),
            LUA_OK);
  ASSERT_NE(hello::lua::utils::dostring(L, "local buffer = require('buffer');"
                                           "local b = buffer.alloc(4);"
                                           "b:setFloat32(-1, 10.0);"),
            LUA_OK);

  ASSERT_NE(hello::lua::utils::dostring(L, "local buffer = require('buffer');"
                                           "local b = buffer.alloc(4);"
                                           "b:getFloat32(1);"),
            LUA_OK);
  ASSERT_NE(hello::lua::utils::dostring(L, "local buffer = require('buffer');"
                                           "local b = buffer.alloc(4);"
                                           "b:getFloat32(-1);"),
            LUA_OK);
}

TEST_F(LuaBuffer_Test, GetString) {
  ASSERT_EQ(hello::lua::utils::dostring(L, "local buffer = require('buffer');\n"
                                           "local b = buffer.alloc(3);\n"
                                           "b:setUint8(0, 65);\n"
                                           "b:setUint8(1, 66);\n"
                                           "b:setUint8(2, 67);\n"
                                           "return b:getString()"),
            LUA_OK)
      << lua_tostring(L, -1);
  ASSERT_STREQ(luaL_checkstring(L, -1), "ABC");
}