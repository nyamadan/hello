
#include "./lua_sdl3_test.hpp"

using namespace hello::lua;

TEST_F(LuaSDL3_Test, LoadImageTest) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif
  ASSERT_EQ(LUA_OK,
            utils::dostring(
                L, "local SDL_image = require('sdl3_image');\n"
                   "local image = "
                   "SDL_image.load('../../hello_host/assets/uv_checker.png');\n"
                   "image:lock();\n"
                   "image:flipVertical();\n"
                   "image:unlock();\n"
                   "return image:getInfo();\n"))
      << lua_tostring(L, -1);
}
