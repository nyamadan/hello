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

TEST_F(LuaSDL2_Test, TestglViewport) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif

  initWindow();
  initRenderer();
  initOpenGL();

  ASSERT_EQ(utils::dostring(L, "local gl = require('opengl');"
                               "gl.viewport(0, 0, 100, 100);"),
            LUA_OK)
      << lua_tostring(L, -1);
}

TEST_F(LuaSDL2_Test, TestDrawArrays) {
#if defined(RUN_ON_GITHUB_ACTIONS)
  GTEST_SKIP() << "Not work for GitHub Actions";
#endif

#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif

  initWindow();
  initRenderer();
  initOpenGL();

  ASSERT_EQ(
      utils::dostring(
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
             "gl.bufferData(gl.ARRAY_BUFFER, data, gl.STATIC_DRAW);\n"
             "local vao = gl.genVertexArray();\n"
             "gl.bindVertexArray(vao);\n"
             "gl.enableVertexAttribArray(0);\n"
             "gl.enableVertexAttribArray(1);\n"
             "gl.bindBuffer(gl.ARRAY_BUFFER, vbo);\n"
             "gl.vertexAttribPointer(0, 3, gl.FLOAT, gl.FALSE, 6 * 4, nil);\n"
             "gl.vertexAttribPointer(1, 3, gl.FLOAT, gl.FALSE, 6 * 4, 3 * 4);\n"
             "gl.drawArrays(gl.TRIANGLES, 0, 3);\n"),
      LUA_OK)
      << lua_tostring(L, -1);
}

TEST_F(LuaSDL2_Test, TestCompileShader) {
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
                   "local shader = gl.createShader(gl.VERTEX_SHADER);\n"
                   "gl.shaderSource(shader, '');\n"
                   "gl.compileShader(shader);\n"
                   "local a = gl.getShaderiv(shader, gl.COMPILE_STATUS);\n"
                   "local b = gl.getShaderInfoLog(shader);\n"
                   "gl.deleteShader(shader);\n"
                   "return b, a;\n"),
            LUA_OK)
      << lua_tostring(L, -1);

  ASSERT_EQ(luaL_checkinteger(L, -1), GL_TRUE);
  ASSERT_STREQ(luaL_checkstring(L, -2), "");
}

TEST_F(LuaSDL2_Test, TestFailToCompileShader) {
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
                   "local shader = gl.createShader(gl.VERTEX_SHADER);\n"
                   "gl.shaderSource(shader, 'error input');\n"
                   "gl.compileShader(shader);\n"
                   "local a = gl.getShaderiv(shader, gl.COMPILE_STATUS);\n"
                   "local b = gl.getShaderInfoLog(shader);\n"
                   "gl.deleteShader(shader);\n"
                   "return b, a;\n"),
            LUA_OK)
      << lua_tostring(L, -1);

  ASSERT_EQ(luaL_checkinteger(L, -1), GL_FALSE);
  ASSERT_STRNE(luaL_checkstring(L, -2), "");
}

TEST_F(LuaSDL2_Test, TestFailToLinkProgram) {
#if defined(RUN_ON_GITHUB_ACTIONS)
  GTEST_SKIP() << "Not work for GitHub Actions";
#endif

#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif

  initWindow();
  initRenderer();
  initOpenGL();

  ASSERT_EQ(utils::dostring(L,
                            "local gl = require('opengl');\n"
                            "local vs = gl.createShader(gl.VERTEX_SHADER);\n"
                            "gl.shaderSource(vs, 'error input');\n"
                            "gl.compileShader(vs);\n"
                            "local fs = gl.createShader(gl.FRAGMENT_SHADER);\n"
                            "gl.shaderSource(fs, 'error input');\n"
                            "gl.compileShader(fs);\n"
                            "local p = gl.createProgram();\n"
                            "gl.attachShader(p, vs);\n"
                            "gl.attachShader(p, fs);\n"
                            "gl.linkProgram(p);\n"
                            "gl.useProgram(p);\n"
                            "local a = gl.getProgramiv(p, gl.LINK_STATUS);\n"
                            "local b = gl.getProgramInfoLog(p);\n"
                            "gl.deleteShader(vs);\n"
                            "gl.deleteShader(fs);\n"
                            "gl.deleteProgram(p);\n"
                            "return b, a;\n"),
            LUA_OK)
      << lua_tostring(L, -1);
  ASSERT_EQ(luaL_checkinteger(L, -1), GL_FALSE);
  ASSERT_STRNE(luaL_checkstring(L, -2), "");
}

TEST_F(LuaSDL2_Test, TestGenTexture) {
#if defined(RUN_ON_GITHUB_ACTIONS)
  GTEST_SKIP() << "Not work for GitHub Actions";
#endif

#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif

  initWindow();
  initRenderer();
  initOpenGL();

  ASSERT_EQ(
      utils::dostring(
          L,
          "local gl = require('opengl');\n"
          "local tex = gl.genTexture();\n"
          "gl.bindTexture(gl.TEXTURE_2D, tex);\n"
          "gl.pixelStorei(gl.UNPACK_ALIGNMENT, 1);\n"
          "gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, 512, "
          "512, 0, gl.RGBA, gl.UNSIGNED_BYTE, nil);\n"
          "gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, "
          "gl.CLAMP_TO_EDGE);\n"
          "gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, "
          "gl.CLAMP_TO_EDGE);\n"
          "gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);\n"
          "gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);\n"
          "gl.deleteTexture(tex);\n"
          "return tex;\n"),
      LUA_OK)
      << lua_tostring(L, -1);
  luaL_checkinteger(L, -1);
}

TEST_F(LuaSDL2_Test, TestGenFramebuffer) {
#if defined(__EMSCRIPTEN__)
  GTEST_SKIP() << "Not work for Emscripten";
#endif

  initWindow();
  initRenderer();
  initOpenGL();

  ASSERT_EQ(utils::dostring(L, "local gl = require('opengl');\n"
                               "local fbo = gl.genFramebuffer();\n"
                               "gl.deleteFramebuffer(fbo);\n"
                               "return fbo;\n"),
            LUA_OK)
      << lua_tostring(L, -1);
  luaL_checkinteger(L, -1);
}