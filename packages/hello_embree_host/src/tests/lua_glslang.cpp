#include "../core/lua/glslang/lua_glslang.hpp"
#include "../core/lua/lua_common.hpp"
#include "../core/lua/lua_utils.hpp"
#include <glslang/Public/ShaderLang.h>
#include <gtest/gtest.h>

#define TEST_LUA_CONSTANT(m, val1, val2)                                       \
  do {                                                                         \
    EXPECT_EQ(luaL_dostring(L, "return require('" m "')." val1), LUA_OK)       \
        << lua_tostring(L, -1);                                                \
    EXPECT_TRUE(lua_isinteger(L, -1))                                          \
        << val1 << " is not integer <type is "                                 \
        << lua_typename(L, lua_type(L, -1)) << ">";                            \
    EXPECT_EQ(lua_tointeger(L, -1), val2);                                     \
  } while (0)

class LuaGlslang_Test : public ::testing::Test {
public:
  static void SetUpTestSuite() { glslang::InitializeProcess(); }

  static void TearDownTestSuite() { glslang::FinalizeProcess(); }

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

TEST_F(LuaGlslang_Test, Constant) {
  TEST_LUA_CONSTANT("glslang", "EShSourceGlsl", glslang::EShSourceGlsl);
  TEST_LUA_CONSTANT("glslang", "EShClientOpenGL", glslang::EShClientOpenGL);
  TEST_LUA_CONSTANT("glslang", "EShTargetOpenGL_450",
                    glslang::EShTargetOpenGL_450);
  TEST_LUA_CONSTANT("glslang", "EShTargetSpv", glslang::EShTargetSpv);
  TEST_LUA_CONSTANT("glslang", "EShTargetSpv_1_0", glslang::EShTargetSpv_1_0);
  TEST_LUA_CONSTANT("glslang", "EShLangFragment", EShLanguage::EShLangFragment);
  TEST_LUA_CONSTANT("glslang", "EShLangVertex", EShLanguage::EShLangVertex);
}

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

TEST_F(LuaGlslang_Test, ParseShaderTest) {
  // WIP
  ASSERT_EQ(hello::lua::utils::dostring(
                L, "local glslang = require('glslang');\n"
                   "local shader = glslang.newShader(4);\n"
                   "local program = glslang.newProgram();\n"
                   "local source = [[#version 310 es\n"
                   "#extension GL_GOOGLE_include_directive : enable\n"
                   "precision mediump float;\n"
                   "precision highp int;\n"
                   "#include \"color\"\n"
                   "layout(location=0) out vec4 fragColor;\n"
                   "void main()\n"
                   "{\n"
                   "  fragColor = getColor();\n"
                   "}\n"
                   "]];\n"
                   "shader:setString(source);\n"
                   "shader:setEnvInput(1, 4, 2, 100);\n"
                   "shader:setEnvClient(2, 450);\n"
                   "shader:setEnvTarget(1, 65536);\n"
                   "shader:setAutoMapLocations(true);\n"
                   "local parseResult = shader:parse(100, true, 0, function"
                   "(module, header, includer, depth)\n"
                   "  local color = [[//module\n"
                   "vec4 getColor()\n"
                   "{\n"
                   "  return vec4(1.0, 0.5, 1.0, 1.0);\n"
                   "}\n"
                   "]];\n"
                   "  return header, color;\n"
                   "end);\n"
                   "program:addShader(shader);\n"
                   "local linkResult = program:link(0);\n"
                   "local intermediate = program:getIntermediate(4);\n"
                   "local spv = glslang.glslangToSpv(program, intermediate);\n"
                   "return linkResult, spv;\n"),
            LUA_OK)
      << lua_tostring(L, -1);

  ASSERT_TRUE(lua_toboolean(L, -1));
}