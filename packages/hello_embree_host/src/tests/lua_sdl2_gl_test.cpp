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
                               "gl.ClearColor(1, 0, 1, 1);"),
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
                               "gl.ClearDepth(1.0);"),
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
                               "gl.Clear(gl.COLOR_BUFFER_BIT);"),
            LUA_OK)
      << lua_tostring(L, -1);
}