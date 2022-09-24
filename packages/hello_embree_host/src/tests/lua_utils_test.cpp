#include <gtest/gtest.h>

#include "../core/lua/lua_common.hpp"
#include "../core/lua/lua_utils.hpp"

using namespace hello::lua::utils;

namespace {
int L_noop(lua_State *) { return 0; }
} // namespace

class LuaUtils_Test : public ::testing::Test {
protected:
  lua_State *L;

  virtual void SetUp() { L = luaL_newstate(); }
  virtual void TearDown() {
    if (L != nullptr) {
      lua_close(L);
      L = nullptr;
    }
  }
};

TEST_F(LuaUtils_Test, Print) {
  luaL_openlibs(L);
  openlibs(L);
  testing::internal::CaptureStdout();
  auto result = dostring(L, "print 'Hello World!!'");
  ASSERT_STREQ("Hello World!!\n",
               testing::internal::GetCapturedStdout().c_str());

  report(L, result);
  ASSERT_EQ(0, result);
}

TEST_F(LuaUtils_Test, isEmscriptenTest) {
#if defined(__EMSCRIPTEN__)
  const auto isEmscripten = true;
#else
  const auto isEmscripten = false;
#endif
  luaL_openlibs(L);
  openlibs(L);
  ASSERT_EQ(LUA_OK, dostring(L,
                             "local utils = require('utils');"
                             "return utils.isEmscripten();",
                             0, 1));
  ASSERT_EQ(isEmscripten, lua_toboolean(L, -1));
}

TEST_F(LuaUtils_Test, registerFunctionTest) {
  luaL_openlibs(L);
  openlibs(L);
  luaL_loadstring(L, "local utils = require('utils');"
                     "local args = {...};"
                     "return utils.registerFunction('update', args[1]);");
  lua_pushcfunction(L, L_noop);
  ASSERT_EQ(LUA_OK, docall(L, 1, 0));

  lua_settop(L, 0);
  getFunction(L, "update");
  ASSERT_EQ(1, lua_gettop(L));
  ASSERT_EQ(L_noop, lua_tocfunction(L, 1))
      << "typename: " << lua_typename(L, 1);
}

TEST_F(LuaUtils_Test, registerFunctionNoMethodTest) {
  luaL_openlibs(L);
  openlibs(L);

  lua_settop(L, 0);
  ASSERT_EQ(LUA_TNIL, getFunction(L, "name1"));
  ASSERT_EQ(1, lua_gettop(L));

  lua_settop(L, 0);
  ASSERT_EQ(LUA_TNIL, getFunction(L, "name2"));
  ASSERT_EQ(1, lua_gettop(L));
}