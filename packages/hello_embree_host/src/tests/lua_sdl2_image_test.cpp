
#include "./lua_sdl2_test.hpp"

using namespace hello::lua;

TEST_F(LuaSDL2_Test, LoadImageTest) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif
  ASSERT_EQ(
      LUA_OK,
      utils::dostring(
          L,
          "local SDL_image = require('sdl2_image');\n"
          "local image = "
          "SDL_image.load('../../hello_embree_host/assets/uv_checker.png');\n"
          "image:lock();\n"
          "image:flipVertical();\n"
          "image:unlock();\n"
          "return image:getInfo();\n"))
      << lua_tostring(L, -1);
}
