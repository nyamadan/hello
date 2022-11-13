#include "./lua_sdl2_test.hpp"

using namespace hello::lua;

TEST_F(LuaSDL2_Test, TestGlSetAttribute) {
  ASSERT_EQ(LUA_OK,
            utils::dostring(
                L, "local SDL = require('sdl2');"
                   "return SDL.GL_SetAttribute(SDL.GL_CONTEXT_FLAGS, 0);"))
      << lua_tostring(L, -1);
}

TEST_F(LuaSDL2_Test, TestFailedToGlSetAttribute) {
  ASSERT_NE(LUA_OK, utils::dostring(L, "local SDL = require('sdl2');"
                                       "return SDL.GL_SetAttribute();"))
      << lua_tostring(L, -1);
}

TEST_F(LuaSDL2_Test, TestGL_CreateContext) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif
  initWindow();
  initRenderer();
  luaL_loadstring(L, "local SDL = require('sdl2');"
                     "local args = {...};"
                     "SDL.GL_SetAttribute(SDL.GL_CONTEXT_FLAGS, 0);"
                     "SDL.GL_SetAttribute(SDL.GL_CONTEXT_PROFILE_MASK, "
                     "SDL.GL_CONTEXT_PROFILE_CORE);"
                     "SDL.GL_SetAttribute(SDL.GL_CONTEXT_MAJOR_VERSION, 3);"
                     "SDL.GL_SetAttribute(SDL.GL_CONTEXT_MINOR_VERSION, 0);"
                     "return SDL.GL_CreateContext(args[1]);");
  pushWindow();
  ASSERT_EQ(LUA_OK, utils::docall(L, 1, 1)) << lua_tostring(L, -1);
  ASSERT_NE(nullptr, luaL_testudata(L, -1, "SDL_GL_Context"));
}

TEST_F(LuaSDL2_Test, TestGL_MakeCurrent) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif
  initWindow();
  initRenderer();
  luaL_loadstring(L, "local SDL = require('sdl2');"
                     "local args = {...};"
                     "SDL.GL_SetAttribute(SDL.GL_CONTEXT_FLAGS, 0);"
                     "SDL.GL_SetAttribute(SDL.GL_CONTEXT_PROFILE_MASK, "
                     "SDL.GL_CONTEXT_PROFILE_CORE);"
                     "SDL.GL_SetAttribute(SDL.GL_CONTEXT_MAJOR_VERSION, 3);"
                     "SDL.GL_SetAttribute(SDL.GL_CONTEXT_MINOR_VERSION, 0);"
                     "local context = SDL.GL_CreateContext(args[1]);"
                     "return SDL.GL_MakeCurrent(args[1], context);");
  pushWindow();
  ASSERT_EQ(LUA_OK, utils::docall(L, 1)) << lua_tostring(L, -1);
  ASSERT_TRUE(lua_isinteger(L, -1));
}

TEST_F(LuaSDL2_Test, TestGL_SetSwapInterval) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif
  initWindow();
  initRenderer();
  luaL_loadstring(L, "local SDL = require('sdl2');"
                     "local args = {...};"
                     "SDL.GL_SetAttribute(SDL.GL_CONTEXT_FLAGS, 0);"
                     "SDL.GL_SetAttribute(SDL.GL_CONTEXT_PROFILE_MASK, "
                     "SDL.GL_CONTEXT_PROFILE_CORE);"
                     "SDL.GL_SetAttribute(SDL.GL_CONTEXT_MAJOR_VERSION, 3);"
                     "SDL.GL_SetAttribute(SDL.GL_CONTEXT_MINOR_VERSION, 0);"
                     "local context = SDL.GL_CreateContext(args[1]);"
                     "SDL.GL_MakeCurrent(args[1], context);"
                     "return SDL.GL_SetSwapInterval(1);");
  pushWindow();
  ASSERT_EQ(LUA_OK, utils::docall(L, 1)) << lua_tostring(L, -1);
  ASSERT_TRUE(lua_isinteger(L, -1));
}

TEST_F(LuaSDL2_Test, TestGL_SwapWindow) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif
  initWindow();
  initRenderer();
  luaL_loadstring(L, "local SDL = require('sdl2');"
                     "local args = {...};"
                     "SDL.GL_SetAttribute(SDL.GL_CONTEXT_FLAGS, 0);"
                     "SDL.GL_SetAttribute(SDL.GL_CONTEXT_PROFILE_MASK, "
                     "SDL.GL_CONTEXT_PROFILE_CORE);"
                     "SDL.GL_SetAttribute(SDL.GL_CONTEXT_MAJOR_VERSION, 3);"
                     "SDL.GL_SetAttribute(SDL.GL_CONTEXT_MINOR_VERSION, 0);"
                     "local context = SDL.GL_CreateContext(args[1]);"
                     "SDL.GL_MakeCurrent(args[1], context);"
                     "SDL.GL_SetSwapInterval(1);"
                     "SDL.GL_SwapWindow(args[1]);");
  pushWindow();
  ASSERT_EQ(LUA_OK, utils::docall(L, 1)) << lua_tostring(L, -1);
}

TEST_F(LuaSDL2_Test, TestglClearColor) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif

  initWindow();
  initRenderer();
  initOpenGL();

  ASSERT_EQ(utils::dostring(L, "local gl = require('opengl');"
                               "gl.clearColor(1, 0, 1, 1);"),
            LUA_OK)
      << lua_tostring(L, -1);
}

TEST_F(LuaSDL2_Test, TestglClearDepth) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif

  initWindow();
  initRenderer();
  initOpenGL();

  ASSERT_EQ(utils::dostring(L, "local gl = require('opengl');"
                               "gl.clearDepth(1.0);"),
            LUA_OK)
      << lua_tostring(L, -1);
}

TEST_F(LuaSDL2_Test, TestglClear) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif

  initWindow();
  initRenderer();
  initOpenGL();

  ASSERT_EQ(utils::dostring(L, "local gl = require('opengl');"
                               "gl.clear(gl.COLOR_BUFFER_BIT);"),
            LUA_OK)
      << lua_tostring(L, -1);
}

TEST_F(LuaSDL2_Test, TestVAO) {
#if defined(RUN_ON_GITHUB_ACTIONS)
  GTEST_SKIP() << "Not work for GitHub Actions";
#endif

#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif

  initWindow();
  initRenderer();
  initOpenGL();

  ASSERT_EQ(utils::dostring(
                L, "local gl = require('opengl');\n"
                   "local buffer = require('buffer');\n"
                   "local points = {\n"
                   "0.0, 0.5, 0.0,  1.0, 0.0, 0.0,\n"
                   "0.5, -0.5, 0.0, 0.0, 1.0, 0.0,\n"
                   "-0.5, -0.5, 0.0, 0.0, 0.0, 1.0\n"
                   "};\n"
                   "local data = buffer.alloc(72);\n"
                   "for i, v in ipairs(points) do\n"
                   "data:setFloat32(4 * (i - 1), v);\n"
                   "end\n"
                   "local vbo = gl.genBuffer();\n"
                   "gl.bindBuffer(gl.ARRAY_BUFFER, vbo);\n"
                   "gl.bufferData(gl.ARRAY_BUFFER, data, gl.STATIC_DRAW);"),
            LUA_OK)
      << lua_tostring(L, -1);
}