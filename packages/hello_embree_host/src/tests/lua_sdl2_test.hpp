#ifndef __LUA_SDL2_TEST_HPP__
#define __LUA_SDL2_TEST_HPP__
#include <gtest/gtest.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "../core/lua/lua_common.hpp"
#include "../core/lua/lua_utils.hpp"
#include "../core/lua/opengl/lua_opengl.hpp"
#include "../core/lua/sdl2/lua_sdl2.hpp"

#define TEST_LUA_CONSTANT(m, val1, val2)                                       \
  do {                                                                         \
    EXPECT_EQ(luaL_dostring(L, "return require('" m "')." val1), LUA_OK)       \
        << lua_tostring(L, -1);                                                \
    EXPECT_TRUE(lua_isinteger(L, -1))                                          \
        << val1 << " is not integer <type is "                                 \
        << lua_typename(L, lua_type(L, -1)) << ">";                            \
    EXPECT_EQ(lua_tointeger(L, -1), val2);                                     \
  } while (0)

class LuaSDL2_Test : public ::testing::Test {
public:
  static void SetUpTestSuite();
  static void TearDownTestSuite();

protected:
  lua_State *L = nullptr;
  SDL_Window *window = nullptr;
  SDL_Renderer *renderer = nullptr;

  void initLuaState();
  void initOpenGL();
  void initWindow(Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  void pushWindow();
  void initRenderer();
  void pushRenderer();
  virtual void SetUp();
  virtual void TearDown();
};
#endif