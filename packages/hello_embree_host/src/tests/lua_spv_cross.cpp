#include "../core/lua/spv_cross/lua_spv_cross.hpp"
#include "../core/lua/buffer/lua_buffer.hpp"
#include "../core/lua/lua_common.hpp"
#include "../core/lua/lua_utils.hpp"
#include <gtest/gtest.h>

class LuaSpvCross_Test : public ::testing::Test {
protected:
  lua_State *L;

  virtual void SetUp() {
    L = luaL_newstate();
    luaL_openlibs(L);
    hello::lua::buffer::openlibs(L);
    hello::lua::spv_cross::openlibs(L);
  }
  virtual void TearDown() {
    if (L != nullptr) {
      lua_close(L);
      L = nullptr;
    }
  }
};

TEST_F(LuaSpvCross_Test, CompileTest) {
  const uint8_t spv[] = {
      0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, 0x00, 0x0a, 0x00, 0x08, 0x00,
      0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x47, 0x4c, 0x53, 0x4c, 0x2e, 0x73, 0x74, 0x64, 0x2e, 0x34, 0x35, 0x30,
      0x00, 0x00, 0x00, 0x00, 0x0e, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x07, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x50, 0x53, 0x4d, 0x61, 0x69, 0x6e, 0x00, 0x00,
      0x17, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x10, 0x00, 0x03, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x03, 0x00, 0x03, 0x00,
      0x05, 0x00, 0x00, 0x00, 0xf4, 0x01, 0x00, 0x00, 0x05, 0x00, 0x04, 0x00,
      0x04, 0x00, 0x00, 0x00, 0x50, 0x53, 0x4d, 0x61, 0x69, 0x6e, 0x00, 0x00,
      0x05, 0x00, 0x05, 0x00, 0x17, 0x00, 0x00, 0x00, 0x69, 0x6e, 0x70, 0x75,
      0x74, 0x2e, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x00, 0x05, 0x00, 0x07, 0x00,
      0x1b, 0x00, 0x00, 0x00, 0x40, 0x65, 0x6e, 0x74, 0x72, 0x79, 0x50, 0x6f,
      0x69, 0x6e, 0x74, 0x4f, 0x75, 0x74, 0x70, 0x75, 0x74, 0x00, 0x00, 0x00,
      0x47, 0x00, 0x04, 0x00, 0x17, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x47, 0x00, 0x04, 0x00, 0x1b, 0x00, 0x00, 0x00,
      0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x00, 0x02, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00,
      0x02, 0x00, 0x00, 0x00, 0x16, 0x00, 0x03, 0x00, 0x06, 0x00, 0x00, 0x00,
      0x20, 0x00, 0x00, 0x00, 0x17, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00,
      0x06, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00,
      0x16, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
      0x3b, 0x00, 0x04, 0x00, 0x16, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x20, 0x00, 0x04, 0x00, 0x1a, 0x00, 0x00, 0x00,
      0x03, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3b, 0x00, 0x04, 0x00,
      0x1a, 0x00, 0x00, 0x00, 0x1b, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
      0x36, 0x00, 0x05, 0x00, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x02, 0x00,
      0x05, 0x00, 0x00, 0x00, 0x3d, 0x00, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00,
      0x18, 0x00, 0x00, 0x00, 0x17, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x03, 0x00,
      0x1b, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x01, 0x00,
      0x38, 0x00, 0x01, 0x00};

  luaL_loadstring(L, "local args = {...};\n"
                     "local p = require('spv_cross').compile(args[1]);\n"
                     "return p;\n");
  const auto size = sizeof(spv);
  auto udBuffer = hello::lua::buffer::alloc(L, size);
  memcpy(udBuffer->data, spv, size);
  ASSERT_EQ(hello::lua::utils::docall(L, 1), LUA_OK) << lua_tostring(L, -1);
  ASSERT_STRNE(luaL_checkstring(L, -1), "");
}