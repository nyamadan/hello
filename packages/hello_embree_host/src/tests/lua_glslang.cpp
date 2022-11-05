#include <gtest/gtest.h>

#include "../core/lua/glslang/lua_glslang.hpp"
#include "../core/lua/lua_common.hpp"
#include "../core/lua/lua_utils.hpp"

class LuaGlslang_Test : public ::testing::Test {
protected:
  lua_State *L;

  virtual void SetUp() {
    L = luaL_newstate();
    luaL_openlibs(L);
    hello::lua::glslang::openlibs(L);
  }
  virtual void TearDown() {
    if (L != nullptr) {
      lua_close(L);
      L = nullptr;
    }
  }
};

TEST_F(LuaGlslang_Test, Require) {
  ASSERT_EQ(hello::lua::utils::dostring(L, "local glslang = require('glslang');"
                                           "return glslang;"),
            LUA_OK)
      << lua_tostring(L, -1);

  ASSERT_EQ(lua_type(L, -1), LUA_TTABLE);
}
