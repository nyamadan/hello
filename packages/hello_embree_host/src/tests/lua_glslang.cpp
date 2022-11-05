#include "../core/lua/glslang/lua_glslang.hpp"
#include "../core/lua/buffer/lua_buffer.hpp"
#include "../core/lua/lua_common.hpp"
#include "../core/lua/lua_utils.hpp"
#include <glslang/Public/ShaderLang.h>
#include <gtest/gtest.h>

class LuaGlslang_Test : public ::testing::Test {
public:
  static void SetUpTestSuite() { glslang::InitializeProcess(); }

  static void TearDownTestSuite() { glslang::FinalizeProcess(); }

protected:
  lua_State *L;

  virtual void SetUp() {
    L = luaL_newstate();
    luaL_openlibs(L);
    hello::lua::buffer::openlibs(L);
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

TEST_F(LuaGlslang_Test, newProgramTest) {
  ASSERT_EQ(hello::lua::utils::dostring(
                L, "local p = require('glslang').newProgram();"
                   "return p;"),
            LUA_OK)
      << lua_tostring(L, -1);
  luaL_checkudata(L, -1, "glslang_Program");
}

TEST_F(LuaGlslang_Test, wProgramGetInfoLogTest) {
  ASSERT_EQ(hello::lua::utils::dostring(
                L, "local p = require('glslang').newProgram();"
                   "return p:getInfoLog();"),
            LUA_OK)
      << lua_tostring(L, -1);
  ASSERT_STREQ(luaL_checkstring(L, -1), "");
}

TEST_F(LuaGlslang_Test, newShaderTest) {
  ASSERT_EQ(hello::lua::utils::dostring(
                L, "local p = require('glslang').newShader(4);"
                   "return p;"),
            LUA_OK)
      << lua_tostring(L, -1);
  luaL_checkudata(L, -1, "glslang_Shader");

  ASSERT_NE(hello::lua::utils::dostring(L, "require('glslang').newShader(-1);"),
            LUA_OK);
  ASSERT_STREQ(lua_tostring(L, -1),
               "[string \"require('glslang').newShader(-1);\"]:1: bad argument "
               "#1 to 'newShader' (only support for fragment or vertex)\nstack "
               "traceback:\n\t[C]: in function 'glslang.newShader'\n\t[string "
               "\"require('glslang').newShader(-1);\"]:1: in main chunk");
}

TEST_F(LuaGlslang_Test, ShaderGetInfoLogTest) {
  ASSERT_EQ(hello::lua::utils::dostring(
                L, "local p = require('glslang').newShader(4);"
                   "return p:getInfoLog();"),
            LUA_OK)
      << lua_tostring(L, -1);
  ASSERT_STREQ(luaL_checkstring(L, -1), "");
}