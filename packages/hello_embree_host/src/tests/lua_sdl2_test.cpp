#include <gtest/gtest.h>

#include <SDL2/SDL.h>

#include "../core/lua/lua_common.hpp"
#include "../core/lua/lua_utils.hpp"
#include "../core/lua/sdl2/lua_sdl2.hpp"

using namespace hello::lua;

#define TEST_LUA_CONSTANT(m, val1, val2)                                       \
  do {                                                                         \
    ASSERT_EQ(LUA_OK, luaL_dostring(L, "return require('" m "')." val1))       \
        << lua_tostring(L, -1);                                                \
    ASSERT_TRUE(lua_isinteger(L, -1)) << val1 << " is not integer";            \
    ASSERT_EQ(val2, luaL_checkinteger(L, -1));                                 \
  } while (0)

class LuaSDL2_Test : public ::testing::Test {
public:
  static void SetUpTestSuite() { SDL_Init(SDL_INIT_VIDEO); }

  static void TearDownTestSuite() { SDL_Quit(); }

protected:
  lua_State *L;
  int refWindow = LUA_REFNIL;
  int refRenderer = LUA_REFNIL;

  void initLuaState() {
    L = luaL_newstate();
    luaL_openlibs(L);
    sdl2::openlibs(L);
  }

  void initWindow() {
    utils::dostring(L,
                    "local SDL = require('sdl2');"
                    "return SDL.CreateWindow('Hello', SDL.WINDOWPOS_UNDEFINED, "
                    "SDL.WINDOWPOS_UNDEFINED, 1280, 720, 0);");
    refWindow = luaL_ref(L, LUA_REGISTRYINDEX);
  }

  void initRenderer() {
    luaL_loadstring(L, "local SDL = require('sdl2');"
                       "local args = {...};"
                       "return SDL.CreateRenderer(args[1], -1, "
                       "SDL.RENDERER_ACCELERATED);");
    lua_rawgeti(L, LUA_REGISTRYINDEX, refWindow);
    utils::docall(L, 1);
    refRenderer = luaL_ref(L, LUA_REGISTRYINDEX);
  }

  virtual void SetUp() { initLuaState(); }

  virtual void TearDown() {
    if (refRenderer != LUA_REFNIL) {
      luaL_loadstring(L, "local SDL = require('sdl2');"
                         "local args = {...};"
                         "return SDL.DestroyRenderer(args[1]);");
      lua_rawgeti(L, LUA_REGISTRYINDEX, refWindow);
      utils::docall(L, 1);

      luaL_unref(L, LUA_REGISTRYINDEX, refRenderer);
      refRenderer = LUA_REFNIL;
    }

    if (refWindow != LUA_REFNIL) {
      luaL_loadstring(L, "local SDL = require('sdl2');"
                         "local args = {...};"
                         "return SDL.DestroyWindow(args[1]);");
      lua_rawgeti(L, LUA_REGISTRYINDEX, refWindow);
      utils::docall(L, 1);

      luaL_unref(L, LUA_REGISTRYINDEX, refWindow);
      refWindow = LUA_REFNIL;
    }

    if (L != nullptr) {
      lua_close(L);
      L = nullptr;
    }
  }
};

TEST_F(LuaSDL2_Test, TestSDLConstants) {
  TEST_LUA_CONSTANT("sdl2", "INIT_VIDEO", SDL_INIT_VIDEO);
  TEST_LUA_CONSTANT("sdl2", "INIT_TIMER", SDL_INIT_TIMER);
  TEST_LUA_CONSTANT("sdl2", "WINDOWPOS_UNDEFINED", SDL_WINDOWPOS_UNDEFINED);
  TEST_LUA_CONSTANT("sdl2", "RENDERER_ACCELERATED", SDL_RENDERER_ACCELERATED);
  TEST_LUA_CONSTANT("sdl2", "QUIT", SDL_QUIT);
  TEST_LUA_CONSTANT("sdl2", "GL_CONTEXT_FLAGS", SDL_GL_CONTEXT_FLAGS);
  TEST_LUA_CONSTANT("sdl2", "GL_CONTEXT_PROFILE_MASK",
                    SDL_GL_CONTEXT_PROFILE_MASK);
  TEST_LUA_CONSTANT("sdl2", "GL_CONTEXT_PROFILE_CORE",
                    SDL_GL_CONTEXT_PROFILE_CORE);
  TEST_LUA_CONSTANT("sdl2", "GL_CONTEXT_MAJOR_VERSION",
                    SDL_GL_CONTEXT_MAJOR_VERSION);
  TEST_LUA_CONSTANT("sdl2", "GL_CONTEXT_MINOR_VERSION",
                    SDL_GL_CONTEXT_MINOR_VERSION);
}

TEST_F(LuaSDL2_Test, TestGetError) {
  ASSERT_EQ(utils::dostring(L, "local SDL = require('sdl2');"
                               "return SDL.GetError();"),
            LUA_OK)
      << lua_tostring(L, -1);
  ASSERT_TRUE(lua_isstring(L, -1))
      << "GetError did not return string: " << lua_typename(L, -1);
}

TEST_F(LuaSDL2_Test, CreateWindow) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif
  ASSERT_EQ(utils::dostring(
                L, "local SDL = require('sdl2');"
                   "return SDL.CreateWindow('Hello', SDL.WINDOWPOS_UNDEFINED, "
                   "SDL.WINDOWPOS_UNDEFINED, 1280, 720, 0);"),
            LUA_OK)
      << "Failed to create: " << lua_tostring(L, -1);
  ASSERT_NE(luaL_testudata(L, -1, "SDL_Window"), nullptr)
      << "SDL_CreateWindow did not return SDL_Window ";
  refWindow = luaL_ref(L, LUA_REGISTRYINDEX);
}

TEST_F(LuaSDL2_Test, FailedToCreateWindow) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif
  ASSERT_NE(utils::dostring(L, "local SDL = require('sdl2');"
                               "return SDL.CreateWindow();"),
            LUA_OK);
}

TEST_F(LuaSDL2_Test, DestroyWindow) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif
  initWindow();
  luaL_loadstring(L, "local SDL = require('sdl2');"
                     "local args = {...};"
                     "return SDL.DestroyWindow(args[1]);");
  lua_rawgeti(L, LUA_REGISTRYINDEX, refWindow);
  ASSERT_EQ(utils::docall(L, 1), LUA_OK)
      << "Failed to destroy: " << lua_tostring(L, -1);
  luaL_unref(L, LUA_REGISTRYINDEX, refWindow);
  refWindow = LUA_REFNIL;
}

TEST_F(LuaSDL2_Test, CreateRenderer) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif
  initWindow();
  luaL_loadstring(L, "local SDL = require('sdl2');"
                     "local args = {...};"
                     "return SDL.CreateRenderer(args[1], -1, "
                     "SDL.RENDERER_ACCELERATED);");
  lua_rawgeti(L, LUA_REGISTRYINDEX, refWindow);
  ASSERT_EQ(utils::docall(L, 1), LUA_OK) << lua_tostring(L, -1);
  ASSERT_NE(luaL_testudata(L, -1, "SDL_Renderer"), nullptr)
      << "SDL_CreateWindow did not return SDL_Renderer";
}

TEST_F(LuaSDL2_Test, FailedToCreateRenderer) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif
  initWindow();
  luaL_loadstring(L, "local SDL = require('sdl2');"
                     "return SDL.CreateRenderer();");
  lua_rawgeti(L, LUA_REGISTRYINDEX, refWindow);
  ASSERT_NE(utils::docall(L, 1), LUA_OK);
}

TEST_F(LuaSDL2_Test, DestroyRenderer) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif
  initWindow();
  initRenderer();
  luaL_loadstring(L, "local SDL = require('sdl2');"
                     "local args = {...};"
                     "return SDL.DestroyRenderer(args[1]);");
  lua_rawgeti(L, LUA_REGISTRYINDEX, refRenderer);
  ASSERT_EQ(LUA_OK, utils::docall(L, 1)) << lua_tostring(L, -1);
  luaL_unref(L, LUA_REGISTRYINDEX, refRenderer);
  refRenderer = LUA_REFNIL;
}

TEST_F(LuaSDL2_Test, TestPollEvent) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif
  initWindow();

  ASSERT_EQ(LUA_OK, utils::dostring(L, "local SDL = require('sdl2');"
                                       "return SDL.PollEvent();"))
      << lua_tostring(L, -1);
  ASSERT_TRUE(lua_istable(L, -1))
      << "SDL_PollEvent did not return table: " << lua_typename(L, -1);
  int idx = lua_gettop(L);
  lua_getfield(L, idx, "type");
  ASSERT_TRUE(lua_isinteger(L, -1))
      << "SDL_PollEvent returned table did not contain type: "
      << lua_typename(L, -1);
}

TEST_F(LuaSDL2_Test, TestDelay) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif
  initWindow();

  ASSERT_EQ(LUA_OK, utils::dostring(L, "local SDL = require('sdl2');"
                                       "return SDL.Delay(16);"))
      << lua_tostring(L, -1);
}

TEST_F(LuaSDL2_Test, TestFailToDelay) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif
  initWindow();

  ASSERT_NE(LUA_OK, utils::dostring(L, "local SDL = require('sdl2');"
                                       "return SDL.Delay();"))
      << lua_tostring(L, -1);
}

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