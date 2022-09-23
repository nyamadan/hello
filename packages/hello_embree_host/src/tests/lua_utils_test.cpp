#include <gtest/gtest.h>

#include "../core/lua/lua_common.hpp"
#include "../core/lua/lua_utils.hpp"

using namespace hello::lua;

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