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
#ifdef __EMSCRIPTEN__
  auto depth = static_cast<GLfloat>(luaL_checknumber(L, 1));
  glClearDepthf(depth);
#else
  auto depth = static_cast<GLclampd>(luaL_checknumber(L, 1));
  glClearDepth(depth);
#endif
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

int L_glGenVertexArray(lua_State *L) {
  GLuint buffers[] = {0};
  glGenVertexArrays(1, buffers);
  lua_pushinteger(L, buffers[0]);
  return 1;
}

int L_glBindVertexArray(lua_State *L) {
  auto array = static_cast<GLuint>(luaL_checkinteger(L, 1));
  glBindVertexArray(array);
  return 0;
}

int L_glEnableVertexAttribArray(lua_State *L) {
  auto index = static_cast<GLuint>(luaL_checkinteger(L, 1));
  glEnableVertexAttribArray(index);
  return 0;
}

int L_glVertexAttribPointer(lua_State *L) {
  auto index = static_cast<GLuint>(luaL_checkinteger(L, 1));
  auto size = static_cast<GLint>(luaL_checkinteger(L, 2));
  auto type = static_cast<GLenum>(luaL_checkinteger(L, 3));
  auto normalized = static_cast<GLboolean>(lua_toboolean(L, 4));
  auto stride = static_cast<GLsizei>(luaL_checkinteger(L, 5));
  auto pointer = reinterpret_cast<const void *>(
      lua_isnil(L, 6) ? 0 : luaL_checkinteger(L, 6));
  glVertexAttribPointer(index, size, type, normalized, stride, pointer);
  return 0;
}

int L_glViewport(lua_State *L) {
  auto x = static_cast<GLint>(luaL_checkinteger(L, 1));
  auto y = static_cast<GLint>(luaL_checkinteger(L, 2));
  auto width = static_cast<GLsizei>(luaL_checkinteger(L, 3));
  auto height = static_cast<GLsizei>(luaL_checkinteger(L, 4));
  glViewport(x, y, width, height);
  return 0;
}

int L_glDrawArrays(lua_State *L) {
  auto mode = static_cast<GLenum>(luaL_checkinteger(L, 1));
  auto first = static_cast<GLint>(luaL_checkinteger(L, 2));
  auto count = static_cast<GLsizei>(luaL_checkinteger(L, 3));
  glDrawArrays(mode, first, count);
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

  lua_pushinteger(L, GL_TRUE);
  lua_setfield(L, -2, "TRUE");

  lua_pushinteger(L, GL_FALSE);
  lua_setfield(L, -2, "FALSE");

  lua_pushinteger(L, GL_FLOAT);
  lua_setfield(L, -2, "FLOAT");

  lua_pushinteger(L, GL_TRIANGLES);
  lua_setfield(L, -2, "TRIANGLES");

  lua_pushcfunction(L, L_loadGLLoader);
  lua_setfield(L, -2, "loadGLLoader");

  lua_pushcfunction(L, L_glClearColor);
  lua_setfield(L, -2, "clearColor");

  lua_pushcfunction(L, L_glClearDepth);
  lua_setfield(L, -2, "clearDepth");

  lua_pushcfunction(L, L_glClear);
  lua_setfield(L, -2, "clear");

  lua_pushcfunction(L, L_glViewport);
  lua_setfield(L, -2, "viewport");

  lua_pushcfunction(L, L_glDrawArrays);
  lua_setfield(L, -2, "drawArrays");

  lua_pushcfunction(L, L_glGenBuffer);
  lua_setfield(L, -2, "genBuffer");

  lua_pushcfunction(L, L_glBindBuffer);
  lua_setfield(L, -2, "bindBuffer");

  lua_pushcfunction(L, L_glBufferData);
  lua_setfield(L, -2, "bufferData");

  lua_pushcfunction(L, L_glGenVertexArray);
  lua_setfield(L, -2, "genVertexArray");

  lua_pushcfunction(L, L_glBindVertexArray);
  lua_setfield(L, -2, "bindVertexArray");

  lua_pushcfunction(L, L_glEnableVertexAttribArray);
  lua_setfield(L, -2, "enableVertexAttribArray");

  lua_pushcfunction(L, L_glVertexAttribPointer);
  lua_setfield(L, -2, "vertexAttribPointer");

  return 1;
}
} // namespace

namespace hello::lua::opengl {
void openlibs(lua_State *L) {
  luaL_requiref(L, "opengl", L_require, false);
  lua_pop(L, 1);
}
} // namespace hello::lua::opengl