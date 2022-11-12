#include "./lua_opengl.hpp"
#include "../buffer/lua_buffer.hpp"
#include <SDL2/SDL.h>
#include <cfloat>
#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

namespace {
int L_loadGLLoader(lua_State *) {
#ifndef __EMSCRIPTEN__
  gladLoadGLLoader(SDL_GL_GetProcAddress);
#endif
  return 0;
}

int L_glClearColor(lua_State *L) {
  auto r = static_cast<GLclampf>(luaL_checknumber(L, 1));
  auto g = static_cast<GLclampf>(luaL_checknumber(L, 2));
  auto b = static_cast<GLclampf>(luaL_checknumber(L, 3));
  auto a = static_cast<GLclampf>(luaL_checknumber(L, 4));
  glClearColor(r, g, b, a);
  return 0;
}

int L_glClearDepth(lua_State *L) {
  auto depth = static_cast<GLclampd>(luaL_checknumber(L, 1));
  glClearDepth(depth);
  return 0;
}

int L_glClear(lua_State *L) {
  auto flags = static_cast<GLbitfield>(luaL_checknumber(L, 1));
  glClear(flags);
  return 0;
}

int L_glGenBuffer(lua_State *L) {
  GLuint buffers[] = {0};
  glGenBuffers(1, buffers);
  lua_pushinteger(L, buffers[0]);
  return 1;
}

int L_glBindBuffer(lua_State *L) {
  auto target = static_cast<GLenum>(luaL_checkinteger(L, 1));
  auto buffer = static_cast<GLuint>(luaL_checkinteger(L, 2));
  glBindBuffer(target, buffer);
  return 0;
}

int L_glBufferData(lua_State *L) {
  auto target = static_cast<GLenum>(luaL_checkinteger(L, 1));
  auto buffer = hello::lua::buffer::get(L, 2);
  luaL_argcheck(L, buffer != nullptr, 2, "Expected buffer");
  luaL_argcheck(L, buffer->usage == nullptr, 2, "operation is not permitted.");
  luaL_argcheck(L, buffer->size > 0, 2, "size must be breater than 0");
  auto usage = static_cast<GLenum>(luaL_checkinteger(L, 3));
  glBufferData(target, buffer->size, buffer->data, usage);
  return 0;
}

int L_require(lua_State *L) {
  lua_newtable(L);

  lua_pushinteger(L, GL_COLOR_BUFFER_BIT);
  lua_setfield(L, -2, "COLOR_BUFFER_BIT");

  lua_pushinteger(L, GL_DEPTH_BUFFER_BIT);
  lua_setfield(L, -2, "DEPTH_BUFFER_BIT");

  lua_pushinteger(L, GL_ARRAY_BUFFER);
  lua_setfield(L, -2, "ARRAY_BUFFER");

  lua_pushinteger(L, GL_STATIC_DRAW);
  lua_setfield(L, -2, "STATIC_DRAW");

  lua_pushcfunction(L, L_loadGLLoader);
  lua_setfield(L, -2, "loadGLLoader");

  lua_pushcfunction(L, L_glClearColor);
  lua_setfield(L, -2, "clearColor");

  lua_pushcfunction(L, L_glClearDepth);
  lua_setfield(L, -2, "clearDepth");

  lua_pushcfunction(L, L_glClear);
  lua_setfield(L, -2, "clear");

  lua_pushcfunction(L, L_glGenBuffer);
  lua_setfield(L, -2, "genBuffer");

  lua_pushcfunction(L, L_glBindBuffer);
  lua_setfield(L, -2, "bindBuffer");

  lua_pushcfunction(L, L_glBufferData);
  lua_setfield(L, -2, "bufferData");

  return 1;
}
} // namespace

namespace hello::lua::opengl {
void openlibs(lua_State *L) {
  luaL_requiref(L, "opengl", L_require, false);
  lua_pop(L, 1);
}
} // namespace hello::lua::opengl