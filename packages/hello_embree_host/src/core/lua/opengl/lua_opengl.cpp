#include "./lua_opengl.hpp"
#include "../buffer/lua_buffer.hpp"
#include <SDL2/SDL.h>
#include <cfloat>
#include <cstring>
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
      lua_isnoneornil(L, 6) ? 0 : luaL_checkinteger(L, 6));
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

int L_glCreateShader(lua_State *L) {
  auto type = static_cast<GLenum>(luaL_checkinteger(L, 1));
  auto result = glCreateShader(type);
  lua_pushinteger(L, static_cast<lua_Integer>(result));
  return 1;
}

int L_glDeleteShader(lua_State *L) {
  auto shader = static_cast<GLuint>(luaL_checkinteger(L, 1));
  glDeleteShader(shader);
  return 0;
}

int L_glShaderSource(lua_State *L) {
  auto shader = static_cast<GLuint>(luaL_checkinteger(L, 1));
  auto source = static_cast<const char *>(luaL_checkstring(L, 2));
  const char *const sources[] = {source};
  const GLint lengths[] = {static_cast<GLint>(strlen(source))};
  glShaderSource(shader, 1, sources, lengths);
  return 0;
}

int L_glCompileShader(lua_State *L) {
  auto shader = static_cast<GLuint>(luaL_checkinteger(L, 1));
  glCompileShader(shader);
  return 0;
}

int L_glGetShaderiv(lua_State *L) {
  auto shader = static_cast<GLuint>(luaL_checkinteger(L, 1));
  auto pname = static_cast<GLenum>(luaL_checkinteger(L, 2));
  GLint params[] = {0};
  glGetShaderiv(shader, pname, params);
  lua_pushinteger(L, params[0]);
  return 1;
}

int L_glGetShaderInfoLog(lua_State *L) {
  auto shader = static_cast<GLuint>(luaL_checkinteger(L, 1));
  GLint maxLength = 0;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
  if (maxLength == 0) {
    lua_pushstring(L, "");
    return 1;
  }
  if (maxLength < 0) {
    luaL_error(L, "Invalid Length: %d", maxLength);
  }
  auto errorLog = static_cast<GLchar *>(malloc(sizeof(GLchar) * maxLength));
  glGetShaderInfoLog(shader, maxLength, &maxLength, errorLog);
  lua_pushstring(L, errorLog);
  free(errorLog);
  return 1;
}

int L_glCreateProgram(lua_State *L) {
  auto program = glCreateProgram();
  lua_pushinteger(L, program);
  return 1;
}

int L_glAttachShader(lua_State *L) {
  auto program = static_cast<GLuint>(luaL_checkinteger(L, 1));
  auto shader = static_cast<GLuint>(luaL_checkinteger(L, 2));
  glAttachShader(program, shader);
  return 0;
}

int L_glLinkProgram(lua_State *L) {
  auto program = static_cast<GLuint>(luaL_checkinteger(L, 1));
  glLinkProgram(program);
  return 0;
}

int L_glDeleteProgram(lua_State *L) {
  auto program = static_cast<GLuint>(luaL_checkinteger(L, 1));
  glDeleteProgram(program);
  return 0;
}

int L_glGetProgramiv(lua_State *L) {
  auto program = static_cast<GLuint>(luaL_checkinteger(L, 1));
  auto pname = static_cast<GLenum>(luaL_checkinteger(L, 2));
  GLint params[] = {0};
  glGetProgramiv(program, pname, params);
  lua_pushinteger(L, params[0]);
  return 1;
}

int L_glGetProgramInfoLog(lua_State *L) {
  auto program = static_cast<GLuint>(luaL_checkinteger(L, 1));
  GLint maxLength = 0;
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);
  if (maxLength == 0) {
    lua_pushstring(L, "");
    return 1;
  }
  if (maxLength < 0) {
    luaL_error(L, "Invalid Length: %d", maxLength);
  }
  auto errorLog = static_cast<GLchar *>(malloc(sizeof(GLchar) * maxLength));
  glGetProgramInfoLog(program, maxLength, &maxLength, errorLog);
  lua_pushstring(L, errorLog);
  free(errorLog);
  return 1;
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

  lua_pushinteger(L, GL_VERTEX_SHADER);
  lua_setfield(L, -2, "VERTEX_SHADER");

  lua_pushinteger(L, GL_FRAGMENT_SHADER);
  lua_setfield(L, -2, "FRAGMENT_SHADER");

  lua_pushinteger(L, GL_COMPILE_STATUS);
  lua_setfield(L, -2, "COMPILE_STATUS");

  lua_pushinteger(L, GL_LINK_STATUS);
  lua_setfield(L, -2, "LINK_STATUS");

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

  lua_pushcfunction(L, L_glCreateShader);
  lua_setfield(L, -2, "createShader");

  lua_pushcfunction(L, L_glDeleteShader);
  lua_setfield(L, -2, "deleteShader");

  lua_pushcfunction(L, L_glShaderSource);
  lua_setfield(L, -2, "shaderSource");

  lua_pushcfunction(L, L_glCompileShader);
  lua_setfield(L, -2, "compileShader");

  lua_pushcfunction(L, L_glGetShaderiv);
  lua_setfield(L, -2, "getShaderiv");

  lua_pushcfunction(L, L_glGetShaderInfoLog);
  lua_setfield(L, -2, "getShaderInfoLog");

  lua_pushcfunction(L, L_glCreateProgram);
  lua_setfield(L, -2, "createProgram");

  lua_pushcfunction(L, L_glAttachShader);
  lua_setfield(L, -2, "attachShader");

  lua_pushcfunction(L, L_glLinkProgram);
  lua_setfield(L, -2, "linkProgram");

  lua_pushcfunction(L, L_glDeleteProgram);
  lua_setfield(L, -2, "deleteProgram");

  lua_pushcfunction(L, L_glGetProgramiv);
  lua_setfield(L, -2, "getProgramiv");

  lua_pushcfunction(L, L_glGetProgramInfoLog);
  lua_setfield(L, -2, "getProgramInfoLog");
  return 1;
}
} // namespace

namespace hello::lua::opengl {
void openlibs(lua_State *L) {
  luaL_requiref(L, "opengl", L_require, false);
  lua_pop(L, 1);
}
} // namespace hello::lua::opengl