#include "./lua_opengl.hpp"
#include "../sdl2_image/lua_sdl2_image.hpp"
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

int L_glDeleteBuffer(lua_State *L) {
  GLuint buffers[] = {0};
  buffers[0] = static_cast<GLuint>(luaL_checkinteger(L, 1));
  glDeleteBuffers(1, buffers);
  return 0;
}

int L_glBindBuffer(lua_State *L) {
  auto target = static_cast<GLenum>(luaL_checkinteger(L, 1));
  auto buffer = static_cast<GLuint>(luaL_checkinteger(L, 2));
  glBindBuffer(target, buffer);
  return 0;
}

int L_glBufferData(lua_State *L) {
  auto target = static_cast<GLenum>(luaL_checkinteger(L, 1));
  size_t size;
  auto data = luaL_checklstring(L, 2, &size);
  luaL_argcheck(L, size > 0, 2, "size must be breater than 0");
  auto usage = static_cast<GLenum>(luaL_checkinteger(L, 3));
  glBufferData(target, size, data, usage);
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

int L_glDrawElements(lua_State *L) {
  auto mode = static_cast<GLenum>(luaL_checkinteger(L, 1));
  auto count = static_cast<GLsizei>(luaL_checkinteger(L, 2));
  auto type = static_cast<GLenum>(luaL_checkinteger(L, 3));
  glDrawElements(mode, count, type, nullptr);
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

int L_glUseProgram(lua_State *L) {
  auto program = static_cast<GLuint>(luaL_checkinteger(L, 1));
  glUseProgram(program);
  return 0;
}

int L_glUniform1i(lua_State *L) {
  auto location = static_cast<GLint>(luaL_checkinteger(L, 1));
  auto v0 = static_cast<GLint>(luaL_checkinteger(L, 2));
  glUniform1i(location, v0);
  return 0;
}

int L_glGenTexture(lua_State *L) {
  GLuint textures[] = {0};
  glGenTextures(1, textures);
  lua_pushinteger(L, textures[0]);
  return 1;
}

int L_glTexImage2D(lua_State *L) {
  auto target = static_cast<GLenum>(luaL_checkinteger(L, 1));
  auto level = static_cast<GLint>(luaL_checkinteger(L, 2));
  auto internalformat = static_cast<GLint>(luaL_checkinteger(L, 3));
  auto width = static_cast<GLsizei>(luaL_checkinteger(L, 4));
  auto height = static_cast<GLsizei>(luaL_checkinteger(L, 5));
  auto border = static_cast<GLint>(luaL_checkinteger(L, 6));
  auto format = static_cast<GLenum>(luaL_checkinteger(L, 7));
  auto type = static_cast<GLenum>(luaL_checkinteger(L, 8));

  const void *pixels = nullptr;
  if (!lua_isnoneornil(L, 9)) {
    if (lua_isstring(L, 9)) {
      size_t size;
      auto data = luaL_checklstring(L, 9, &size);
      luaL_argcheck(L, size > 0, 9, "size must be breater than 0");
      pixels = data;
    } else {
      auto pudSurface = hello::lua::sdl2_image::get(L, 9);
      luaL_argcheck(L, pudSurface->surface != nullptr, 9,
                    "specify SDL_Surface or Buffer");
      pixels = pudSurface->surface->pixels;
    }
  }

  glTexImage2D(target, level, internalformat, width, height, border, format,
               type, pixels);
  return 0;
}

int L_glTexParameteri(lua_State *L) {
  auto target = static_cast<GLenum>(luaL_checkinteger(L, 1));
  auto pname = static_cast<GLenum>(luaL_checkinteger(L, 2));
  auto param = static_cast<GLint>(luaL_checkinteger(L, 3));
  glTexParameteri(target, pname, param);
  return 0;
}

int L_glPixelStorei(lua_State *L) {
  auto pname = static_cast<GLenum>(luaL_checkinteger(L, 1));
  auto param = static_cast<GLint>(luaL_checkinteger(L, 2));
  glPixelStorei(pname, param);
  return 0;
}

int L_glBindTexture(lua_State *L) {
  auto target = static_cast<GLenum>(luaL_checkinteger(L, 1));
  auto texture = static_cast<GLuint>(luaL_checkinteger(L, 2));
  glBindTexture(target, texture);
  return 0;
}

int L_glActivateTexture(lua_State *L) {
  auto texture = static_cast<GLenum>(luaL_checkinteger(L, 1));
  glActiveTexture(texture);
  return 0;
}

int L_glDeleteTexture(lua_State *L) {
  auto texture = static_cast<GLuint>(luaL_checkinteger(L, 1));
  GLuint textures[] = {texture};
  glDeleteTextures(1, textures);
  return 0;
}

int L_glGenFramebuffer(lua_State *L) {
  GLuint fbos[] = {0};
  glGenFramebuffers(1, fbos);
  lua_pushinteger(L, fbos[0]);
  return 1;
}

int L_glBindFramebuffer(lua_State *L) {
  auto target = static_cast<GLenum>(luaL_checkinteger(L, 1));
  auto framebuffer = static_cast<GLuint>(luaL_checkinteger(L, 2));
  glBindFramebuffer(target, framebuffer);
  return 0;
}

int L_glDeleteFramebuffer(lua_State *L) {
  auto fbo = static_cast<GLuint>(luaL_checkinteger(L, 1));
  GLuint framebuffers[] = {fbo};
  glDeleteFramebuffers(1, framebuffers);
  return 0;
}

int L_glGenRenderbuffer(lua_State *L) {
  GLuint rbos[] = {0};
  glGenRenderbuffers(1, rbos);
  lua_pushinteger(L, rbos[0]);
  return 1;
}

int L_glBindRenderbuffer(lua_State *L) {
  auto target = static_cast<GLenum>(luaL_checkinteger(L, 1));
  auto renderbuffer = static_cast<GLuint>(luaL_checkinteger(L, 2));
  glBindRenderbuffer(target, renderbuffer);
  return 1;
}

int L_glRenderbufferStorage(lua_State *L) {
  auto target = static_cast<GLenum>(luaL_checkinteger(L, 1));
  auto internalFormat = static_cast<GLenum>(luaL_checkinteger(L, 2));
  auto width = static_cast<GLsizei>(luaL_checkinteger(L, 3));
  auto height = static_cast<GLsizei>(luaL_checkinteger(L, 4));
  glRenderbufferStorage(target, internalFormat, width, height);
  return 1;
}

int L_glDeleteRenderbuffer(lua_State *L) {
  auto rbo = static_cast<GLuint>(luaL_checkinteger(L, 1));
  GLuint rbos[] = {rbo};
  glDeleteRenderbuffers(1, rbos);
  return 0;
}

int L_glFramebufferRenderbuffer(lua_State *L) {
  auto target = static_cast<GLenum>(luaL_checkinteger(L, 1));
  auto attachment = static_cast<GLenum>(luaL_checkinteger(L, 2));
  auto renderbuffertarget = static_cast<GLenum>(luaL_checkinteger(L, 3));
  auto renderbuffer = static_cast<GLuint>(luaL_checkinteger(L, 4));
  glFramebufferRenderbuffer(target, attachment, renderbuffertarget,
                            renderbuffer);
  return 0;
}

int L_glFramebufferTexture2D(lua_State *L) {
  auto target = static_cast<GLenum>(luaL_checkinteger(L, 1));
  auto attachment = static_cast<GLenum>(luaL_checkinteger(L, 2));
  auto textarget = static_cast<GLenum>(luaL_checkinteger(L, 3));
  auto texture = static_cast<GLuint>(luaL_checkinteger(L, 4));
  auto level = static_cast<GLint>(luaL_checkinteger(L, 5));
  glFramebufferTexture2D(target, attachment, textarget, texture, level);
  return 0;
}

int lua_pushConstants(lua_State *L, int idx) {
  if (idx < 0) {
    idx = idx - 1;
  }

  lua_pushinteger(L, GL_DEPTH_BUFFER_BIT);
  lua_setfield(L, idx, "DEPTH_BUFFER_BIT");

  lua_pushinteger(L, GL_STENCIL_BUFFER_BIT);
  lua_setfield(L, idx, "STENCIL_BUFFER_BIT");

  lua_pushinteger(L, GL_COLOR_BUFFER_BIT);
  lua_setfield(L, idx, "COLOR_BUFFER_BIT");

  lua_pushinteger(L, GL_FALSE);
  lua_setfield(L, idx, "FALSE");

  lua_pushinteger(L, GL_TRUE);
  lua_setfield(L, idx, "TRUE");

  lua_pushinteger(L, GL_POINTS);
  lua_setfield(L, idx, "POINTS");

  lua_pushinteger(L, GL_LINES);
  lua_setfield(L, idx, "LINES");

  lua_pushinteger(L, GL_LINE_LOOP);
  lua_setfield(L, idx, "LINE_LOOP");

  lua_pushinteger(L, GL_LINE_STRIP);
  lua_setfield(L, idx, "LINE_STRIP");

  lua_pushinteger(L, GL_TRIANGLES);
  lua_setfield(L, idx, "TRIANGLES");

  lua_pushinteger(L, GL_TRIANGLE_STRIP);
  lua_setfield(L, idx, "TRIANGLE_STRIP");

  lua_pushinteger(L, GL_TRIANGLE_FAN);
  lua_setfield(L, idx, "TRIANGLE_FAN");

  lua_pushinteger(L, GL_NEVER);
  lua_setfield(L, idx, "NEVER");

  lua_pushinteger(L, GL_LESS);
  lua_setfield(L, idx, "LESS");

  lua_pushinteger(L, GL_EQUAL);
  lua_setfield(L, idx, "EQUAL");

  lua_pushinteger(L, GL_LEQUAL);
  lua_setfield(L, idx, "LEQUAL");

  lua_pushinteger(L, GL_GREATER);
  lua_setfield(L, idx, "GREATER");

  lua_pushinteger(L, GL_NOTEQUAL);
  lua_setfield(L, idx, "NOTEQUAL");

  lua_pushinteger(L, GL_GEQUAL);
  lua_setfield(L, idx, "GEQUAL");

  lua_pushinteger(L, GL_ALWAYS);
  lua_setfield(L, idx, "ALWAYS");

  lua_pushinteger(L, GL_ZERO);
  lua_setfield(L, idx, "ZERO");

  lua_pushinteger(L, GL_ONE);
  lua_setfield(L, idx, "ONE");

  lua_pushinteger(L, GL_SRC_COLOR);
  lua_setfield(L, idx, "SRC_COLOR");

  lua_pushinteger(L, GL_ONE_MINUS_SRC_COLOR);
  lua_setfield(L, idx, "ONE_MINUS_SRC_COLOR");

  lua_pushinteger(L, GL_SRC_ALPHA);
  lua_setfield(L, idx, "SRC_ALPHA");

  lua_pushinteger(L, GL_ONE_MINUS_SRC_ALPHA);
  lua_setfield(L, idx, "ONE_MINUS_SRC_ALPHA");

  lua_pushinteger(L, GL_DST_ALPHA);
  lua_setfield(L, idx, "DST_ALPHA");

  lua_pushinteger(L, GL_ONE_MINUS_DST_ALPHA);
  lua_setfield(L, idx, "ONE_MINUS_DST_ALPHA");

  lua_pushinteger(L, GL_DST_COLOR);
  lua_setfield(L, idx, "DST_COLOR");

  lua_pushinteger(L, GL_ONE_MINUS_DST_COLOR);
  lua_setfield(L, idx, "ONE_MINUS_DST_COLOR");

  lua_pushinteger(L, GL_SRC_ALPHA_SATURATE);
  lua_setfield(L, idx, "SRC_ALPHA_SATURATE");

  lua_pushinteger(L, GL_NONE);
  lua_setfield(L, idx, "NONE");

  lua_pushinteger(L, GL_FRONT);
  lua_setfield(L, idx, "FRONT");

  lua_pushinteger(L, GL_BACK);
  lua_setfield(L, idx, "BACK");

  lua_pushinteger(L, GL_FRONT_AND_BACK);
  lua_setfield(L, idx, "FRONT_AND_BACK");

  lua_pushinteger(L, GL_NO_ERROR);
  lua_setfield(L, idx, "NO_ERROR");

  lua_pushinteger(L, GL_INVALID_ENUM);
  lua_setfield(L, idx, "INVALID_ENUM");

  lua_pushinteger(L, GL_INVALID_VALUE);
  lua_setfield(L, idx, "INVALID_VALUE");

  lua_pushinteger(L, GL_INVALID_OPERATION);
  lua_setfield(L, idx, "INVALID_OPERATION");

  lua_pushinteger(L, GL_OUT_OF_MEMORY);
  lua_setfield(L, idx, "OUT_OF_MEMORY");

  lua_pushinteger(L, GL_CW);
  lua_setfield(L, idx, "CW");

  lua_pushinteger(L, GL_CCW);
  lua_setfield(L, idx, "CCW");

  lua_pushinteger(L, GL_CULL_FACE);
  lua_setfield(L, idx, "CULL_FACE");

  lua_pushinteger(L, GL_CULL_FACE_MODE);
  lua_setfield(L, idx, "CULL_FACE_MODE");

  lua_pushinteger(L, GL_FRONT_FACE);
  lua_setfield(L, idx, "FRONT_FACE");

  lua_pushinteger(L, GL_DEPTH_RANGE);
  lua_setfield(L, idx, "DEPTH_RANGE");

  lua_pushinteger(L, GL_DEPTH_TEST);
  lua_setfield(L, idx, "DEPTH_TEST");

  lua_pushinteger(L, GL_DEPTH_WRITEMASK);
  lua_setfield(L, idx, "DEPTH_WRITEMASK");

  lua_pushinteger(L, GL_DEPTH_CLEAR_VALUE);
  lua_setfield(L, idx, "DEPTH_CLEAR_VALUE");

  lua_pushinteger(L, GL_DEPTH_FUNC);
  lua_setfield(L, idx, "DEPTH_FUNC");

  lua_pushinteger(L, GL_STENCIL_TEST);
  lua_setfield(L, idx, "STENCIL_TEST");

  lua_pushinteger(L, GL_STENCIL_CLEAR_VALUE);
  lua_setfield(L, idx, "STENCIL_CLEAR_VALUE");

  lua_pushinteger(L, GL_STENCIL_FUNC);
  lua_setfield(L, idx, "STENCIL_FUNC");

  lua_pushinteger(L, GL_STENCIL_VALUE_MASK);
  lua_setfield(L, idx, "STENCIL_VALUE_MASK");

  lua_pushinteger(L, GL_STENCIL_FAIL);
  lua_setfield(L, idx, "STENCIL_FAIL");

  lua_pushinteger(L, GL_STENCIL_PASS_DEPTH_FAIL);
  lua_setfield(L, idx, "STENCIL_PASS_DEPTH_FAIL");

  lua_pushinteger(L, GL_STENCIL_PASS_DEPTH_PASS);
  lua_setfield(L, idx, "STENCIL_PASS_DEPTH_PASS");

  lua_pushinteger(L, GL_STENCIL_REF);
  lua_setfield(L, idx, "STENCIL_REF");

  lua_pushinteger(L, GL_STENCIL_WRITEMASK);
  lua_setfield(L, idx, "STENCIL_WRITEMASK");

  lua_pushinteger(L, GL_VIEWPORT);
  lua_setfield(L, idx, "VIEWPORT");

  lua_pushinteger(L, GL_DITHER);
  lua_setfield(L, idx, "DITHER");

  lua_pushinteger(L, GL_BLEND);
  lua_setfield(L, idx, "BLEND");

  lua_pushinteger(L, GL_READ_BUFFER);
  lua_setfield(L, idx, "READ_BUFFER");

  lua_pushinteger(L, GL_SCISSOR_BOX);
  lua_setfield(L, idx, "SCISSOR_BOX");

  lua_pushinteger(L, GL_SCISSOR_TEST);
  lua_setfield(L, idx, "SCISSOR_TEST");

  lua_pushinteger(L, GL_COLOR_CLEAR_VALUE);
  lua_setfield(L, idx, "COLOR_CLEAR_VALUE");

  lua_pushinteger(L, GL_COLOR_WRITEMASK);
  lua_setfield(L, idx, "COLOR_WRITEMASK");

  lua_pushinteger(L, GL_UNPACK_ROW_LENGTH);
  lua_setfield(L, idx, "UNPACK_ROW_LENGTH");

  lua_pushinteger(L, GL_UNPACK_SKIP_ROWS);
  lua_setfield(L, idx, "UNPACK_SKIP_ROWS");

  lua_pushinteger(L, GL_UNPACK_SKIP_PIXELS);
  lua_setfield(L, idx, "UNPACK_SKIP_PIXELS");

  lua_pushinteger(L, GL_UNPACK_ALIGNMENT);
  lua_setfield(L, idx, "UNPACK_ALIGNMENT");

  lua_pushinteger(L, GL_PACK_ROW_LENGTH);
  lua_setfield(L, idx, "PACK_ROW_LENGTH");

  lua_pushinteger(L, GL_PACK_SKIP_ROWS);
  lua_setfield(L, idx, "PACK_SKIP_ROWS");

  lua_pushinteger(L, GL_PACK_SKIP_PIXELS);
  lua_setfield(L, idx, "PACK_SKIP_PIXELS");

  lua_pushinteger(L, GL_PACK_ALIGNMENT);
  lua_setfield(L, idx, "PACK_ALIGNMENT");

  lua_pushinteger(L, GL_MAX_TEXTURE_SIZE);
  lua_setfield(L, idx, "MAX_TEXTURE_SIZE");

  lua_pushinteger(L, GL_MAX_VIEWPORT_DIMS);
  lua_setfield(L, idx, "MAX_VIEWPORT_DIMS");

  lua_pushinteger(L, GL_SUBPIXEL_BITS);
  lua_setfield(L, idx, "SUBPIXEL_BITS");

  lua_pushinteger(L, GL_TEXTURE_2D);
  lua_setfield(L, idx, "TEXTURE_2D");

  lua_pushinteger(L, GL_DONT_CARE);
  lua_setfield(L, idx, "DONT_CARE");

  lua_pushinteger(L, GL_FASTEST);
  lua_setfield(L, idx, "FASTEST");

  lua_pushinteger(L, GL_NICEST);
  lua_setfield(L, idx, "NICEST");

  lua_pushinteger(L, GL_BYTE);
  lua_setfield(L, idx, "BYTE");

  lua_pushinteger(L, GL_UNSIGNED_BYTE);
  lua_setfield(L, idx, "UNSIGNED_BYTE");

  lua_pushinteger(L, GL_SHORT);
  lua_setfield(L, idx, "SHORT");

  lua_pushinteger(L, GL_UNSIGNED_SHORT);
  lua_setfield(L, idx, "UNSIGNED_SHORT");

  lua_pushinteger(L, GL_INT);
  lua_setfield(L, idx, "INT");

  lua_pushinteger(L, GL_UNSIGNED_INT);
  lua_setfield(L, idx, "UNSIGNED_INT");

  lua_pushinteger(L, GL_FLOAT);
  lua_setfield(L, idx, "FLOAT");

  lua_pushinteger(L, GL_INVERT);
  lua_setfield(L, idx, "INVERT");

  lua_pushinteger(L, GL_TEXTURE);
  lua_setfield(L, idx, "TEXTURE");

  lua_pushinteger(L, GL_COLOR);
  lua_setfield(L, idx, "COLOR");

  lua_pushinteger(L, GL_DEPTH);
  lua_setfield(L, idx, "DEPTH");

  lua_pushinteger(L, GL_STENCIL);
  lua_setfield(L, idx, "STENCIL");

  lua_pushinteger(L, GL_DEPTH_COMPONENT);
  lua_setfield(L, idx, "DEPTH_COMPONENT");

  lua_pushinteger(L, GL_RED);
  lua_setfield(L, idx, "RED");

  lua_pushinteger(L, GL_GREEN);
  lua_setfield(L, idx, "GREEN");

  lua_pushinteger(L, GL_BLUE);
  lua_setfield(L, idx, "BLUE");

  lua_pushinteger(L, GL_ALPHA);
  lua_setfield(L, idx, "ALPHA");

  lua_pushinteger(L, GL_RGB);
  lua_setfield(L, idx, "RGB");

  lua_pushinteger(L, GL_RGBA);
  lua_setfield(L, idx, "RGBA");

  lua_pushinteger(L, GL_KEEP);
  lua_setfield(L, idx, "KEEP");

  lua_pushinteger(L, GL_REPLACE);
  lua_setfield(L, idx, "REPLACE");

  lua_pushinteger(L, GL_INCR);
  lua_setfield(L, idx, "INCR");

  lua_pushinteger(L, GL_DECR);
  lua_setfield(L, idx, "DECR");

  lua_pushinteger(L, GL_VENDOR);
  lua_setfield(L, idx, "VENDOR");

  lua_pushinteger(L, GL_RENDERER);
  lua_setfield(L, idx, "RENDERER");

  lua_pushinteger(L, GL_VERSION);
  lua_setfield(L, idx, "VERSION");

  lua_pushinteger(L, GL_EXTENSIONS);
  lua_setfield(L, idx, "EXTENSIONS");

  lua_pushinteger(L, GL_NEAREST);
  lua_setfield(L, idx, "NEAREST");

  lua_pushinteger(L, GL_LINEAR);
  lua_setfield(L, idx, "LINEAR");

  lua_pushinteger(L, GL_NEAREST_MIPMAP_NEAREST);
  lua_setfield(L, idx, "NEAREST_MIPMAP_NEAREST");

  lua_pushinteger(L, GL_LINEAR_MIPMAP_NEAREST);
  lua_setfield(L, idx, "LINEAR_MIPMAP_NEAREST");

  lua_pushinteger(L, GL_NEAREST_MIPMAP_LINEAR);
  lua_setfield(L, idx, "NEAREST_MIPMAP_LINEAR");

  lua_pushinteger(L, GL_LINEAR_MIPMAP_LINEAR);
  lua_setfield(L, idx, "LINEAR_MIPMAP_LINEAR");

  lua_pushinteger(L, GL_TEXTURE_MAG_FILTER);
  lua_setfield(L, idx, "TEXTURE_MAG_FILTER");

  lua_pushinteger(L, GL_TEXTURE_MIN_FILTER);
  lua_setfield(L, idx, "TEXTURE_MIN_FILTER");

  lua_pushinteger(L, GL_TEXTURE_WRAP_S);
  lua_setfield(L, idx, "TEXTURE_WRAP_S");

  lua_pushinteger(L, GL_TEXTURE_WRAP_T);
  lua_setfield(L, idx, "TEXTURE_WRAP_T");

  lua_pushinteger(L, GL_REPEAT);
  lua_setfield(L, idx, "REPEAT");

  lua_pushinteger(L, GL_POLYGON_OFFSET_UNITS);
  lua_setfield(L, idx, "POLYGON_OFFSET_UNITS");

  lua_pushinteger(L, GL_POLYGON_OFFSET_FILL);
  lua_setfield(L, idx, "POLYGON_OFFSET_FILL");

  lua_pushinteger(L, GL_POLYGON_OFFSET_FACTOR);
  lua_setfield(L, idx, "POLYGON_OFFSET_FACTOR");

  lua_pushinteger(L, GL_TEXTURE_BINDING_2D);
  lua_setfield(L, idx, "TEXTURE_BINDING_2D");
  lua_pushinteger(L, GL_RGB8);
  lua_setfield(L, idx, "RGB8");

  lua_pushinteger(L, GL_RGBA4);
  lua_setfield(L, idx, "RGBA4");

  lua_pushinteger(L, GL_RGB5_A1);
  lua_setfield(L, idx, "RGB5_A1");

  lua_pushinteger(L, GL_RGBA8);
  lua_setfield(L, idx, "RGBA8");

  lua_pushinteger(L, GL_RGB10_A2);
  lua_setfield(L, idx, "RGB10_A2");

  lua_pushinteger(L, GL_UNSIGNED_SHORT_4_4_4_4);
  lua_setfield(L, idx, "UNSIGNED_SHORT_4_4_4_4");

  lua_pushinteger(L, GL_UNSIGNED_SHORT_5_5_5_1);
  lua_setfield(L, idx, "UNSIGNED_SHORT_5_5_5_1");

  lua_pushinteger(L, GL_TEXTURE_BINDING_3D);
  lua_setfield(L, idx, "TEXTURE_BINDING_3D");

  lua_pushinteger(L, GL_UNPACK_SKIP_IMAGES);
  lua_setfield(L, idx, "UNPACK_SKIP_IMAGES");

  lua_pushinteger(L, GL_UNPACK_IMAGE_HEIGHT);
  lua_setfield(L, idx, "UNPACK_IMAGE_HEIGHT");

  lua_pushinteger(L, GL_TEXTURE_3D);
  lua_setfield(L, idx, "TEXTURE_3D");

  lua_pushinteger(L, GL_TEXTURE_WRAP_R);
  lua_setfield(L, idx, "TEXTURE_WRAP_R");

  lua_pushinteger(L, GL_MAX_3D_TEXTURE_SIZE);
  lua_setfield(L, idx, "MAX_3D_TEXTURE_SIZE");

  lua_pushinteger(L, GL_UNSIGNED_SHORT_5_6_5);
  lua_setfield(L, idx, "UNSIGNED_SHORT_5_6_5");

  lua_pushinteger(L, GL_UNSIGNED_INT_2_10_10_10_REV);
  lua_setfield(L, idx, "UNSIGNED_INT_2_10_10_10_REV");

  lua_pushinteger(L, GL_MAX_ELEMENTS_VERTICES);
  lua_setfield(L, idx, "MAX_ELEMENTS_VERTICES");

  lua_pushinteger(L, GL_MAX_ELEMENTS_INDICES);
  lua_setfield(L, idx, "MAX_ELEMENTS_INDICES");

  lua_pushinteger(L, GL_CLAMP_TO_EDGE);
  lua_setfield(L, idx, "CLAMP_TO_EDGE");

  lua_pushinteger(L, GL_TEXTURE_MIN_LOD);
  lua_setfield(L, idx, "TEXTURE_MIN_LOD");

  lua_pushinteger(L, GL_TEXTURE_MAX_LOD);
  lua_setfield(L, idx, "TEXTURE_MAX_LOD");

  lua_pushinteger(L, GL_TEXTURE_BASE_LEVEL);
  lua_setfield(L, idx, "TEXTURE_BASE_LEVEL");

  lua_pushinteger(L, GL_TEXTURE_MAX_LEVEL);
  lua_setfield(L, idx, "TEXTURE_MAX_LEVEL");

  lua_pushinteger(L, GL_ALIASED_LINE_WIDTH_RANGE);
  lua_setfield(L, idx, "ALIASED_LINE_WIDTH_RANGE");

  lua_pushinteger(L, GL_TEXTURE0);
  lua_setfield(L, idx, "TEXTURE0");

  lua_pushinteger(L, GL_TEXTURE1);
  lua_setfield(L, idx, "TEXTURE1");

  lua_pushinteger(L, GL_TEXTURE2);
  lua_setfield(L, idx, "TEXTURE2");

  lua_pushinteger(L, GL_TEXTURE3);
  lua_setfield(L, idx, "TEXTURE3");

  lua_pushinteger(L, GL_TEXTURE4);
  lua_setfield(L, idx, "TEXTURE4");

  lua_pushinteger(L, GL_TEXTURE5);
  lua_setfield(L, idx, "TEXTURE5");

  lua_pushinteger(L, GL_TEXTURE6);
  lua_setfield(L, idx, "TEXTURE6");

  lua_pushinteger(L, GL_TEXTURE7);
  lua_setfield(L, idx, "TEXTURE7");

  lua_pushinteger(L, GL_TEXTURE8);
  lua_setfield(L, idx, "TEXTURE8");

  lua_pushinteger(L, GL_TEXTURE9);
  lua_setfield(L, idx, "TEXTURE9");

  lua_pushinteger(L, GL_TEXTURE10);
  lua_setfield(L, idx, "TEXTURE10");

  lua_pushinteger(L, GL_TEXTURE11);
  lua_setfield(L, idx, "TEXTURE11");

  lua_pushinteger(L, GL_TEXTURE12);
  lua_setfield(L, idx, "TEXTURE12");

  lua_pushinteger(L, GL_TEXTURE13);
  lua_setfield(L, idx, "TEXTURE13");

  lua_pushinteger(L, GL_TEXTURE14);
  lua_setfield(L, idx, "TEXTURE14");

  lua_pushinteger(L, GL_TEXTURE15);
  lua_setfield(L, idx, "TEXTURE15");

  lua_pushinteger(L, GL_TEXTURE16);
  lua_setfield(L, idx, "TEXTURE16");

  lua_pushinteger(L, GL_TEXTURE17);
  lua_setfield(L, idx, "TEXTURE17");

  lua_pushinteger(L, GL_TEXTURE18);
  lua_setfield(L, idx, "TEXTURE18");

  lua_pushinteger(L, GL_TEXTURE19);
  lua_setfield(L, idx, "TEXTURE19");

  lua_pushinteger(L, GL_TEXTURE20);
  lua_setfield(L, idx, "TEXTURE20");

  lua_pushinteger(L, GL_TEXTURE21);
  lua_setfield(L, idx, "TEXTURE21");

  lua_pushinteger(L, GL_TEXTURE22);
  lua_setfield(L, idx, "TEXTURE22");

  lua_pushinteger(L, GL_TEXTURE23);
  lua_setfield(L, idx, "TEXTURE23");

  lua_pushinteger(L, GL_TEXTURE24);
  lua_setfield(L, idx, "TEXTURE24");

  lua_pushinteger(L, GL_TEXTURE25);
  lua_setfield(L, idx, "TEXTURE25");

  lua_pushinteger(L, GL_TEXTURE26);
  lua_setfield(L, idx, "TEXTURE26");

  lua_pushinteger(L, GL_TEXTURE27);
  lua_setfield(L, idx, "TEXTURE27");

  lua_pushinteger(L, GL_TEXTURE28);
  lua_setfield(L, idx, "TEXTURE28");

  lua_pushinteger(L, GL_TEXTURE29);
  lua_setfield(L, idx, "TEXTURE29");

  lua_pushinteger(L, GL_TEXTURE30);
  lua_setfield(L, idx, "TEXTURE30");

  lua_pushinteger(L, GL_TEXTURE31);
  lua_setfield(L, idx, "TEXTURE31");

  lua_pushinteger(L, GL_ACTIVE_TEXTURE);
  lua_setfield(L, idx, "ACTIVE_TEXTURE");

  lua_pushinteger(L, GL_SAMPLE_ALPHA_TO_COVERAGE);
  lua_setfield(L, idx, "SAMPLE_ALPHA_TO_COVERAGE");

  lua_pushinteger(L, GL_SAMPLE_COVERAGE);
  lua_setfield(L, idx, "SAMPLE_COVERAGE");

  lua_pushinteger(L, GL_SAMPLE_BUFFERS);
  lua_setfield(L, idx, "SAMPLE_BUFFERS");

  lua_pushinteger(L, GL_SAMPLES);
  lua_setfield(L, idx, "SAMPLES");

  lua_pushinteger(L, GL_SAMPLE_COVERAGE_VALUE);
  lua_setfield(L, idx, "SAMPLE_COVERAGE_VALUE");

  lua_pushinteger(L, GL_SAMPLE_COVERAGE_INVERT);
  lua_setfield(L, idx, "SAMPLE_COVERAGE_INVERT");

  lua_pushinteger(L, GL_TEXTURE_CUBE_MAP);
  lua_setfield(L, idx, "TEXTURE_CUBE_MAP");

  lua_pushinteger(L, GL_TEXTURE_BINDING_CUBE_MAP);
  lua_setfield(L, idx, "TEXTURE_BINDING_CUBE_MAP");

  lua_pushinteger(L, GL_TEXTURE_CUBE_MAP_POSITIVE_X);
  lua_setfield(L, idx, "TEXTURE_CUBE_MAP_POSITIVE_X");

  lua_pushinteger(L, GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
  lua_setfield(L, idx, "TEXTURE_CUBE_MAP_NEGATIVE_X");

  lua_pushinteger(L, GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
  lua_setfield(L, idx, "TEXTURE_CUBE_MAP_POSITIVE_Y");

  lua_pushinteger(L, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
  lua_setfield(L, idx, "TEXTURE_CUBE_MAP_NEGATIVE_Y");

  lua_pushinteger(L, GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
  lua_setfield(L, idx, "TEXTURE_CUBE_MAP_POSITIVE_Z");

  lua_pushinteger(L, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
  lua_setfield(L, idx, "TEXTURE_CUBE_MAP_NEGATIVE_Z");

  lua_pushinteger(L, GL_MAX_CUBE_MAP_TEXTURE_SIZE);
  lua_setfield(L, idx, "MAX_CUBE_MAP_TEXTURE_SIZE");

  lua_pushinteger(L, GL_NUM_COMPRESSED_TEXTURE_FORMATS);
  lua_setfield(L, idx, "NUM_COMPRESSED_TEXTURE_FORMATS");

  lua_pushinteger(L, GL_COMPRESSED_TEXTURE_FORMATS);
  lua_setfield(L, idx, "COMPRESSED_TEXTURE_FORMATS");

  lua_pushinteger(L, GL_BLEND_DST_RGB);
  lua_setfield(L, idx, "BLEND_DST_RGB");

  lua_pushinteger(L, GL_BLEND_SRC_RGB);
  lua_setfield(L, idx, "BLEND_SRC_RGB");

  lua_pushinteger(L, GL_BLEND_DST_ALPHA);
  lua_setfield(L, idx, "BLEND_DST_ALPHA");

  lua_pushinteger(L, GL_BLEND_SRC_ALPHA);
  lua_setfield(L, idx, "BLEND_SRC_ALPHA");

  lua_pushinteger(L, GL_DEPTH_COMPONENT16);
  lua_setfield(L, idx, "DEPTH_COMPONENT16");

  lua_pushinteger(L, GL_DEPTH_COMPONENT24);
  lua_setfield(L, idx, "DEPTH_COMPONENT24");

  lua_pushinteger(L, GL_MIRRORED_REPEAT);
  lua_setfield(L, idx, "MIRRORED_REPEAT");

  lua_pushinteger(L, GL_MAX_TEXTURE_LOD_BIAS);
  lua_setfield(L, idx, "MAX_TEXTURE_LOD_BIAS");

  lua_pushinteger(L, GL_INCR_WRAP);
  lua_setfield(L, idx, "INCR_WRAP");

  lua_pushinteger(L, GL_DECR_WRAP);
  lua_setfield(L, idx, "DECR_WRAP");

  lua_pushinteger(L, GL_TEXTURE_COMPARE_MODE);
  lua_setfield(L, idx, "TEXTURE_COMPARE_MODE");

  lua_pushinteger(L, GL_TEXTURE_COMPARE_FUNC);
  lua_setfield(L, idx, "TEXTURE_COMPARE_FUNC");

  lua_pushinteger(L, GL_BLEND_COLOR);
  lua_setfield(L, idx, "BLEND_COLOR");

  lua_pushinteger(L, GL_BLEND_EQUATION);
  lua_setfield(L, idx, "BLEND_EQUATION");

  lua_pushinteger(L, GL_CONSTANT_COLOR);
  lua_setfield(L, idx, "CONSTANT_COLOR");

  lua_pushinteger(L, GL_ONE_MINUS_CONSTANT_COLOR);
  lua_setfield(L, idx, "ONE_MINUS_CONSTANT_COLOR");

  lua_pushinteger(L, GL_CONSTANT_ALPHA);
  lua_setfield(L, idx, "CONSTANT_ALPHA");

  lua_pushinteger(L, GL_ONE_MINUS_CONSTANT_ALPHA);
  lua_setfield(L, idx, "ONE_MINUS_CONSTANT_ALPHA");

  lua_pushinteger(L, GL_FUNC_ADD);
  lua_setfield(L, idx, "FUNC_ADD");

  lua_pushinteger(L, GL_FUNC_REVERSE_SUBTRACT);
  lua_setfield(L, idx, "FUNC_REVERSE_SUBTRACT");

  lua_pushinteger(L, GL_FUNC_SUBTRACT);
  lua_setfield(L, idx, "FUNC_SUBTRACT");

  lua_pushinteger(L, GL_MIN);
  lua_setfield(L, idx, "MIN");

  lua_pushinteger(L, GL_MAX);
  lua_setfield(L, idx, "MAX");

  lua_pushinteger(L, GL_BUFFER_SIZE);
  lua_setfield(L, idx, "BUFFER_SIZE");

  lua_pushinteger(L, GL_BUFFER_USAGE);
  lua_setfield(L, idx, "BUFFER_USAGE");

  lua_pushinteger(L, GL_CURRENT_QUERY);
  lua_setfield(L, idx, "CURRENT_QUERY");

  lua_pushinteger(L, GL_QUERY_RESULT);
  lua_setfield(L, idx, "QUERY_RESULT");

  lua_pushinteger(L, GL_QUERY_RESULT_AVAILABLE);
  lua_setfield(L, idx, "QUERY_RESULT_AVAILABLE");

  lua_pushinteger(L, GL_ARRAY_BUFFER);
  lua_setfield(L, idx, "ARRAY_BUFFER");

  lua_pushinteger(L, GL_ELEMENT_ARRAY_BUFFER);
  lua_setfield(L, idx, "ELEMENT_ARRAY_BUFFER");

  lua_pushinteger(L, GL_ARRAY_BUFFER_BINDING);
  lua_setfield(L, idx, "ARRAY_BUFFER_BINDING");

  lua_pushinteger(L, GL_ELEMENT_ARRAY_BUFFER_BINDING);
  lua_setfield(L, idx, "ELEMENT_ARRAY_BUFFER_BINDING");

  lua_pushinteger(L, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING);
  lua_setfield(L, idx, "VERTEX_ATTRIB_ARRAY_BUFFER_BINDING");

  lua_pushinteger(L, GL_BUFFER_MAPPED);
  lua_setfield(L, idx, "BUFFER_MAPPED");

  lua_pushinteger(L, GL_BUFFER_MAP_POINTER);
  lua_setfield(L, idx, "BUFFER_MAP_POINTER");

  lua_pushinteger(L, GL_STREAM_DRAW);
  lua_setfield(L, idx, "STREAM_DRAW");

  lua_pushinteger(L, GL_STREAM_READ);
  lua_setfield(L, idx, "STREAM_READ");

  lua_pushinteger(L, GL_STREAM_COPY);
  lua_setfield(L, idx, "STREAM_COPY");

  lua_pushinteger(L, GL_STATIC_DRAW);
  lua_setfield(L, idx, "STATIC_DRAW");

  lua_pushinteger(L, GL_STATIC_READ);
  lua_setfield(L, idx, "STATIC_READ");

  lua_pushinteger(L, GL_STATIC_COPY);
  lua_setfield(L, idx, "STATIC_COPY");

  lua_pushinteger(L, GL_DYNAMIC_DRAW);
  lua_setfield(L, idx, "DYNAMIC_DRAW");

  lua_pushinteger(L, GL_DYNAMIC_READ);
  lua_setfield(L, idx, "DYNAMIC_READ");

  lua_pushinteger(L, GL_DYNAMIC_COPY);
  lua_setfield(L, idx, "DYNAMIC_COPY");

  lua_pushinteger(L, GL_BLEND_EQUATION_RGB);
  lua_setfield(L, idx, "BLEND_EQUATION_RGB");

  lua_pushinteger(L, GL_VERTEX_ATTRIB_ARRAY_ENABLED);
  lua_setfield(L, idx, "VERTEX_ATTRIB_ARRAY_ENABLED");

  lua_pushinteger(L, GL_VERTEX_ATTRIB_ARRAY_SIZE);
  lua_setfield(L, idx, "VERTEX_ATTRIB_ARRAY_SIZE");

  lua_pushinteger(L, GL_VERTEX_ATTRIB_ARRAY_STRIDE);
  lua_setfield(L, idx, "VERTEX_ATTRIB_ARRAY_STRIDE");

  lua_pushinteger(L, GL_VERTEX_ATTRIB_ARRAY_TYPE);
  lua_setfield(L, idx, "VERTEX_ATTRIB_ARRAY_TYPE");

  lua_pushinteger(L, GL_CURRENT_VERTEX_ATTRIB);
  lua_setfield(L, idx, "CURRENT_VERTEX_ATTRIB");

  lua_pushinteger(L, GL_VERTEX_ATTRIB_ARRAY_POINTER);
  lua_setfield(L, idx, "VERTEX_ATTRIB_ARRAY_POINTER");

  lua_pushinteger(L, GL_STENCIL_BACK_FUNC);
  lua_setfield(L, idx, "STENCIL_BACK_FUNC");

  lua_pushinteger(L, GL_STENCIL_BACK_FAIL);
  lua_setfield(L, idx, "STENCIL_BACK_FAIL");

  lua_pushinteger(L, GL_STENCIL_BACK_PASS_DEPTH_FAIL);
  lua_setfield(L, idx, "STENCIL_BACK_PASS_DEPTH_FAIL");

  lua_pushinteger(L, GL_STENCIL_BACK_PASS_DEPTH_PASS);
  lua_setfield(L, idx, "STENCIL_BACK_PASS_DEPTH_PASS");

  lua_pushinteger(L, GL_MAX_DRAW_BUFFERS);
  lua_setfield(L, idx, "MAX_DRAW_BUFFERS");

  lua_pushinteger(L, GL_DRAW_BUFFER0);
  lua_setfield(L, idx, "DRAW_BUFFER0");

  lua_pushinteger(L, GL_DRAW_BUFFER1);
  lua_setfield(L, idx, "DRAW_BUFFER1");

  lua_pushinteger(L, GL_DRAW_BUFFER2);
  lua_setfield(L, idx, "DRAW_BUFFER2");

  lua_pushinteger(L, GL_DRAW_BUFFER3);
  lua_setfield(L, idx, "DRAW_BUFFER3");

  lua_pushinteger(L, GL_DRAW_BUFFER4);
  lua_setfield(L, idx, "DRAW_BUFFER4");

  lua_pushinteger(L, GL_DRAW_BUFFER5);
  lua_setfield(L, idx, "DRAW_BUFFER5");

  lua_pushinteger(L, GL_DRAW_BUFFER6);
  lua_setfield(L, idx, "DRAW_BUFFER6");

  lua_pushinteger(L, GL_DRAW_BUFFER7);
  lua_setfield(L, idx, "DRAW_BUFFER7");

  lua_pushinteger(L, GL_DRAW_BUFFER8);
  lua_setfield(L, idx, "DRAW_BUFFER8");

  lua_pushinteger(L, GL_DRAW_BUFFER9);
  lua_setfield(L, idx, "DRAW_BUFFER9");

  lua_pushinteger(L, GL_DRAW_BUFFER10);
  lua_setfield(L, idx, "DRAW_BUFFER10");

  lua_pushinteger(L, GL_DRAW_BUFFER11);
  lua_setfield(L, idx, "DRAW_BUFFER11");

  lua_pushinteger(L, GL_DRAW_BUFFER12);
  lua_setfield(L, idx, "DRAW_BUFFER12");

  lua_pushinteger(L, GL_DRAW_BUFFER13);
  lua_setfield(L, idx, "DRAW_BUFFER13");

  lua_pushinteger(L, GL_DRAW_BUFFER14);
  lua_setfield(L, idx, "DRAW_BUFFER14");

  lua_pushinteger(L, GL_DRAW_BUFFER15);
  lua_setfield(L, idx, "DRAW_BUFFER15");

  lua_pushinteger(L, GL_BLEND_EQUATION_ALPHA);
  lua_setfield(L, idx, "BLEND_EQUATION_ALPHA");

  lua_pushinteger(L, GL_MAX_VERTEX_ATTRIBS);
  lua_setfield(L, idx, "MAX_VERTEX_ATTRIBS");

  lua_pushinteger(L, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED);
  lua_setfield(L, idx, "VERTEX_ATTRIB_ARRAY_NORMALIZED");

  lua_pushinteger(L, GL_MAX_TEXTURE_IMAGE_UNITS);
  lua_setfield(L, idx, "MAX_TEXTURE_IMAGE_UNITS");

  lua_pushinteger(L, GL_FRAGMENT_SHADER);
  lua_setfield(L, idx, "FRAGMENT_SHADER");

  lua_pushinteger(L, GL_VERTEX_SHADER);
  lua_setfield(L, idx, "VERTEX_SHADER");

  lua_pushinteger(L, GL_MAX_FRAGMENT_UNIFORM_COMPONENTS);
  lua_setfield(L, idx, "MAX_FRAGMENT_UNIFORM_COMPONENTS");

  lua_pushinteger(L, GL_MAX_VERTEX_UNIFORM_COMPONENTS);
  lua_setfield(L, idx, "MAX_VERTEX_UNIFORM_COMPONENTS");

  lua_pushinteger(L, GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS);
  lua_setfield(L, idx, "MAX_VERTEX_TEXTURE_IMAGE_UNITS");

  lua_pushinteger(L, GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
  lua_setfield(L, idx, "MAX_COMBINED_TEXTURE_IMAGE_UNITS");

  lua_pushinteger(L, GL_SHADER_TYPE);
  lua_setfield(L, idx, "SHADER_TYPE");

  lua_pushinteger(L, GL_FLOAT_VEC2);
  lua_setfield(L, idx, "FLOAT_VEC2");

  lua_pushinteger(L, GL_FLOAT_VEC3);
  lua_setfield(L, idx, "FLOAT_VEC3");

  lua_pushinteger(L, GL_FLOAT_VEC4);
  lua_setfield(L, idx, "FLOAT_VEC4");

  lua_pushinteger(L, GL_INT_VEC2);
  lua_setfield(L, idx, "INT_VEC2");

  lua_pushinteger(L, GL_INT_VEC3);
  lua_setfield(L, idx, "INT_VEC3");

  lua_pushinteger(L, GL_INT_VEC4);
  lua_setfield(L, idx, "INT_VEC4");

  lua_pushinteger(L, GL_BOOL);
  lua_setfield(L, idx, "BOOL");

  lua_pushinteger(L, GL_BOOL_VEC2);
  lua_setfield(L, idx, "BOOL_VEC2");

  lua_pushinteger(L, GL_BOOL_VEC3);
  lua_setfield(L, idx, "BOOL_VEC3");

  lua_pushinteger(L, GL_BOOL_VEC4);
  lua_setfield(L, idx, "BOOL_VEC4");

  lua_pushinteger(L, GL_FLOAT_MAT2);
  lua_setfield(L, idx, "FLOAT_MAT2");

  lua_pushinteger(L, GL_FLOAT_MAT3);
  lua_setfield(L, idx, "FLOAT_MAT3");

  lua_pushinteger(L, GL_FLOAT_MAT4);
  lua_setfield(L, idx, "FLOAT_MAT4");

  lua_pushinteger(L, GL_SAMPLER_2D);
  lua_setfield(L, idx, "SAMPLER_2D");

  lua_pushinteger(L, GL_SAMPLER_3D);
  lua_setfield(L, idx, "SAMPLER_3D");

  lua_pushinteger(L, GL_SAMPLER_CUBE);
  lua_setfield(L, idx, "SAMPLER_CUBE");

  lua_pushinteger(L, GL_SAMPLER_2D_SHADOW);
  lua_setfield(L, idx, "SAMPLER_2D_SHADOW");

  lua_pushinteger(L, GL_DELETE_STATUS);
  lua_setfield(L, idx, "DELETE_STATUS");

  lua_pushinteger(L, GL_COMPILE_STATUS);
  lua_setfield(L, idx, "COMPILE_STATUS");

  lua_pushinteger(L, GL_LINK_STATUS);
  lua_setfield(L, idx, "LINK_STATUS");

  lua_pushinteger(L, GL_VALIDATE_STATUS);
  lua_setfield(L, idx, "VALIDATE_STATUS");

  lua_pushinteger(L, GL_INFO_LOG_LENGTH);
  lua_setfield(L, idx, "INFO_LOG_LENGTH");

  lua_pushinteger(L, GL_ATTACHED_SHADERS);
  lua_setfield(L, idx, "ATTACHED_SHADERS");

  lua_pushinteger(L, GL_ACTIVE_UNIFORMS);
  lua_setfield(L, idx, "ACTIVE_UNIFORMS");

  lua_pushinteger(L, GL_ACTIVE_UNIFORM_MAX_LENGTH);
  lua_setfield(L, idx, "ACTIVE_UNIFORM_MAX_LENGTH");

  lua_pushinteger(L, GL_SHADER_SOURCE_LENGTH);
  lua_setfield(L, idx, "SHADER_SOURCE_LENGTH");

  lua_pushinteger(L, GL_ACTIVE_ATTRIBUTES);
  lua_setfield(L, idx, "ACTIVE_ATTRIBUTES");

  lua_pushinteger(L, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH);
  lua_setfield(L, idx, "ACTIVE_ATTRIBUTE_MAX_LENGTH");

  lua_pushinteger(L, GL_FRAGMENT_SHADER_DERIVATIVE_HINT);
  lua_setfield(L, idx, "FRAGMENT_SHADER_DERIVATIVE_HINT");

  lua_pushinteger(L, GL_SHADING_LANGUAGE_VERSION);
  lua_setfield(L, idx, "SHADING_LANGUAGE_VERSION");

  lua_pushinteger(L, GL_CURRENT_PROGRAM);
  lua_setfield(L, idx, "CURRENT_PROGRAM");

  lua_pushinteger(L, GL_STENCIL_BACK_REF);
  lua_setfield(L, idx, "STENCIL_BACK_REF");

  lua_pushinteger(L, GL_STENCIL_BACK_VALUE_MASK);
  lua_setfield(L, idx, "STENCIL_BACK_VALUE_MASK");

  lua_pushinteger(L, GL_STENCIL_BACK_WRITEMASK);
  lua_setfield(L, idx, "STENCIL_BACK_WRITEMASK");

  lua_pushinteger(L, GL_PIXEL_PACK_BUFFER);
  lua_setfield(L, idx, "PIXEL_PACK_BUFFER");

  lua_pushinteger(L, GL_PIXEL_UNPACK_BUFFER);
  lua_setfield(L, idx, "PIXEL_UNPACK_BUFFER");

  lua_pushinteger(L, GL_PIXEL_PACK_BUFFER_BINDING);
  lua_setfield(L, idx, "PIXEL_PACK_BUFFER_BINDING");

  lua_pushinteger(L, GL_PIXEL_UNPACK_BUFFER_BINDING);
  lua_setfield(L, idx, "PIXEL_UNPACK_BUFFER_BINDING");

  lua_pushinteger(L, GL_FLOAT_MAT2x3);
  lua_setfield(L, idx, "FLOAT_MAT2x3");

  lua_pushinteger(L, GL_FLOAT_MAT2x4);
  lua_setfield(L, idx, "FLOAT_MAT2x4");

  lua_pushinteger(L, GL_FLOAT_MAT3x2);
  lua_setfield(L, idx, "FLOAT_MAT3x2");

  lua_pushinteger(L, GL_FLOAT_MAT3x4);
  lua_setfield(L, idx, "FLOAT_MAT3x4");

  lua_pushinteger(L, GL_FLOAT_MAT4x2);
  lua_setfield(L, idx, "FLOAT_MAT4x2");

  lua_pushinteger(L, GL_FLOAT_MAT4x3);
  lua_setfield(L, idx, "FLOAT_MAT4x3");

  lua_pushinteger(L, GL_SRGB);
  lua_setfield(L, idx, "SRGB");

  lua_pushinteger(L, GL_SRGB8);
  lua_setfield(L, idx, "SRGB8");

  lua_pushinteger(L, GL_SRGB8_ALPHA8);
  lua_setfield(L, idx, "SRGB8_ALPHA8");

  lua_pushinteger(L, GL_COMPARE_REF_TO_TEXTURE);
  lua_setfield(L, idx, "COMPARE_REF_TO_TEXTURE");

  lua_pushinteger(L, GL_MAJOR_VERSION);
  lua_setfield(L, idx, "MAJOR_VERSION");

  lua_pushinteger(L, GL_MINOR_VERSION);
  lua_setfield(L, idx, "MINOR_VERSION");

  lua_pushinteger(L, GL_NUM_EXTENSIONS);
  lua_setfield(L, idx, "NUM_EXTENSIONS");

  lua_pushinteger(L, GL_RGBA32F);
  lua_setfield(L, idx, "RGBA32F");

  lua_pushinteger(L, GL_RGB32F);
  lua_setfield(L, idx, "RGB32F");

  lua_pushinteger(L, GL_RGBA16F);
  lua_setfield(L, idx, "RGBA16F");

  lua_pushinteger(L, GL_RGB16F);
  lua_setfield(L, idx, "RGB16F");

  lua_pushinteger(L, GL_VERTEX_ATTRIB_ARRAY_INTEGER);
  lua_setfield(L, idx, "VERTEX_ATTRIB_ARRAY_INTEGER");

  lua_pushinteger(L, GL_MAX_ARRAY_TEXTURE_LAYERS);
  lua_setfield(L, idx, "MAX_ARRAY_TEXTURE_LAYERS");

  lua_pushinteger(L, GL_MIN_PROGRAM_TEXEL_OFFSET);
  lua_setfield(L, idx, "MIN_PROGRAM_TEXEL_OFFSET");

  lua_pushinteger(L, GL_MAX_PROGRAM_TEXEL_OFFSET);
  lua_setfield(L, idx, "MAX_PROGRAM_TEXEL_OFFSET");

  lua_pushinteger(L, GL_MAX_VARYING_COMPONENTS);
  lua_setfield(L, idx, "MAX_VARYING_COMPONENTS");

  lua_pushinteger(L, GL_TEXTURE_2D_ARRAY);
  lua_setfield(L, idx, "TEXTURE_2D_ARRAY");

  lua_pushinteger(L, GL_TEXTURE_BINDING_2D_ARRAY);
  lua_setfield(L, idx, "TEXTURE_BINDING_2D_ARRAY");

  lua_pushinteger(L, GL_R11F_G11F_B10F);
  lua_setfield(L, idx, "R11F_G11F_B10F");

  lua_pushinteger(L, GL_UNSIGNED_INT_10F_11F_11F_REV);
  lua_setfield(L, idx, "UNSIGNED_INT_10F_11F_11F_REV");

  lua_pushinteger(L, GL_RGB9_E5);
  lua_setfield(L, idx, "RGB9_E5");

  lua_pushinteger(L, GL_UNSIGNED_INT_5_9_9_9_REV);
  lua_setfield(L, idx, "UNSIGNED_INT_5_9_9_9_REV");

  lua_pushinteger(L, GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH);
  lua_setfield(L, idx, "TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH");

  lua_pushinteger(L, GL_TRANSFORM_FEEDBACK_BUFFER_MODE);
  lua_setfield(L, idx, "TRANSFORM_FEEDBACK_BUFFER_MODE");

  lua_pushinteger(L, GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS);
  lua_setfield(L, idx, "MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS");

  lua_pushinteger(L, GL_TRANSFORM_FEEDBACK_VARYINGS);
  lua_setfield(L, idx, "TRANSFORM_FEEDBACK_VARYINGS");

  lua_pushinteger(L, GL_TRANSFORM_FEEDBACK_BUFFER_START);
  lua_setfield(L, idx, "TRANSFORM_FEEDBACK_BUFFER_START");

  lua_pushinteger(L, GL_TRANSFORM_FEEDBACK_BUFFER_SIZE);
  lua_setfield(L, idx, "TRANSFORM_FEEDBACK_BUFFER_SIZE");

  lua_pushinteger(L, GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
  lua_setfield(L, idx, "TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN");

  lua_pushinteger(L, GL_RASTERIZER_DISCARD);
  lua_setfield(L, idx, "RASTERIZER_DISCARD");

  lua_pushinteger(L, GL_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS);
  lua_setfield(L, idx, "MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS");

  lua_pushinteger(L, GL_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS);
  lua_setfield(L, idx, "MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS");

  lua_pushinteger(L, GL_INTERLEAVED_ATTRIBS);
  lua_setfield(L, idx, "INTERLEAVED_ATTRIBS");

  lua_pushinteger(L, GL_SEPARATE_ATTRIBS);
  lua_setfield(L, idx, "SEPARATE_ATTRIBS");

  lua_pushinteger(L, GL_TRANSFORM_FEEDBACK_BUFFER);
  lua_setfield(L, idx, "TRANSFORM_FEEDBACK_BUFFER");

  lua_pushinteger(L, GL_TRANSFORM_FEEDBACK_BUFFER_BINDING);
  lua_setfield(L, idx, "TRANSFORM_FEEDBACK_BUFFER_BINDING");

  lua_pushinteger(L, GL_RGBA32UI);
  lua_setfield(L, idx, "RGBA32UI");

  lua_pushinteger(L, GL_RGB32UI);
  lua_setfield(L, idx, "RGB32UI");

  lua_pushinteger(L, GL_RGBA16UI);
  lua_setfield(L, idx, "RGBA16UI");

  lua_pushinteger(L, GL_RGB16UI);
  lua_setfield(L, idx, "RGB16UI");

  lua_pushinteger(L, GL_RGBA8UI);
  lua_setfield(L, idx, "RGBA8UI");

  lua_pushinteger(L, GL_RGB8UI);
  lua_setfield(L, idx, "RGB8UI");

  lua_pushinteger(L, GL_RGBA32I);
  lua_setfield(L, idx, "RGBA32I");

  lua_pushinteger(L, GL_RGB32I);
  lua_setfield(L, idx, "RGB32I");

  lua_pushinteger(L, GL_RGBA16I);
  lua_setfield(L, idx, "RGBA16I");

  lua_pushinteger(L, GL_RGB16I);
  lua_setfield(L, idx, "RGB16I");

  lua_pushinteger(L, GL_RGBA8I);
  lua_setfield(L, idx, "RGBA8I");

  lua_pushinteger(L, GL_RGB8I);
  lua_setfield(L, idx, "RGB8I");

  lua_pushinteger(L, GL_RED_INTEGER);
  lua_setfield(L, idx, "RED_INTEGER");

  lua_pushinteger(L, GL_RGB_INTEGER);
  lua_setfield(L, idx, "RGB_INTEGER");

  lua_pushinteger(L, GL_RGBA_INTEGER);
  lua_setfield(L, idx, "RGBA_INTEGER");

  lua_pushinteger(L, GL_SAMPLER_2D_ARRAY);
  lua_setfield(L, idx, "SAMPLER_2D_ARRAY");

  lua_pushinteger(L, GL_SAMPLER_2D_ARRAY_SHADOW);
  lua_setfield(L, idx, "SAMPLER_2D_ARRAY_SHADOW");

  lua_pushinteger(L, GL_SAMPLER_CUBE_SHADOW);
  lua_setfield(L, idx, "SAMPLER_CUBE_SHADOW");

  lua_pushinteger(L, GL_UNSIGNED_INT_VEC2);
  lua_setfield(L, idx, "UNSIGNED_INT_VEC2");

  lua_pushinteger(L, GL_UNSIGNED_INT_VEC3);
  lua_setfield(L, idx, "UNSIGNED_INT_VEC3");

  lua_pushinteger(L, GL_UNSIGNED_INT_VEC4);
  lua_setfield(L, idx, "UNSIGNED_INT_VEC4");

  lua_pushinteger(L, GL_INT_SAMPLER_2D);
  lua_setfield(L, idx, "INT_SAMPLER_2D");

  lua_pushinteger(L, GL_INT_SAMPLER_3D);
  lua_setfield(L, idx, "INT_SAMPLER_3D");

  lua_pushinteger(L, GL_INT_SAMPLER_CUBE);
  lua_setfield(L, idx, "INT_SAMPLER_CUBE");

  lua_pushinteger(L, GL_INT_SAMPLER_2D_ARRAY);
  lua_setfield(L, idx, "INT_SAMPLER_2D_ARRAY");

  lua_pushinteger(L, GL_UNSIGNED_INT_SAMPLER_2D);
  lua_setfield(L, idx, "UNSIGNED_INT_SAMPLER_2D");

  lua_pushinteger(L, GL_UNSIGNED_INT_SAMPLER_3D);
  lua_setfield(L, idx, "UNSIGNED_INT_SAMPLER_3D");

  lua_pushinteger(L, GL_UNSIGNED_INT_SAMPLER_CUBE);
  lua_setfield(L, idx, "UNSIGNED_INT_SAMPLER_CUBE");

  lua_pushinteger(L, GL_UNSIGNED_INT_SAMPLER_2D_ARRAY);
  lua_setfield(L, idx, "UNSIGNED_INT_SAMPLER_2D_ARRAY");

  lua_pushinteger(L, GL_BUFFER_ACCESS_FLAGS);
  lua_setfield(L, idx, "BUFFER_ACCESS_FLAGS");

  lua_pushinteger(L, GL_BUFFER_MAP_LENGTH);
  lua_setfield(L, idx, "BUFFER_MAP_LENGTH");

  lua_pushinteger(L, GL_BUFFER_MAP_OFFSET);
  lua_setfield(L, idx, "BUFFER_MAP_OFFSET");

  lua_pushinteger(L, GL_DEPTH_COMPONENT32F);
  lua_setfield(L, idx, "DEPTH_COMPONENT32F");

  lua_pushinteger(L, GL_DEPTH32F_STENCIL8);
  lua_setfield(L, idx, "DEPTH32F_STENCIL8");

  lua_pushinteger(L, GL_FLOAT_32_UNSIGNED_INT_24_8_REV);
  lua_setfield(L, idx, "FLOAT_32_UNSIGNED_INT_24_8_REV");

  lua_pushinteger(L, GL_INVALID_FRAMEBUFFER_OPERATION);
  lua_setfield(L, idx, "INVALID_FRAMEBUFFER_OPERATION");

  lua_pushinteger(L, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING);
  lua_setfield(L, idx, "FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING");

  lua_pushinteger(L, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE);
  lua_setfield(L, idx, "FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE");

  lua_pushinteger(L, GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE);
  lua_setfield(L, idx, "FRAMEBUFFER_ATTACHMENT_RED_SIZE");

  lua_pushinteger(L, GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE);
  lua_setfield(L, idx, "FRAMEBUFFER_ATTACHMENT_GREEN_SIZE");

  lua_pushinteger(L, GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE);
  lua_setfield(L, idx, "FRAMEBUFFER_ATTACHMENT_BLUE_SIZE");

  lua_pushinteger(L, GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE);
  lua_setfield(L, idx, "FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE");

  lua_pushinteger(L, GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE);
  lua_setfield(L, idx, "FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE");

  lua_pushinteger(L, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE);
  lua_setfield(L, idx, "FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE");

  lua_pushinteger(L, GL_FRAMEBUFFER_DEFAULT);
  lua_setfield(L, idx, "FRAMEBUFFER_DEFAULT");

  lua_pushinteger(L, GL_FRAMEBUFFER_UNDEFINED);
  lua_setfield(L, idx, "FRAMEBUFFER_UNDEFINED");

  lua_pushinteger(L, GL_DEPTH_STENCIL_ATTACHMENT);
  lua_setfield(L, idx, "DEPTH_STENCIL_ATTACHMENT");

  lua_pushinteger(L, GL_MAX_RENDERBUFFER_SIZE);
  lua_setfield(L, idx, "MAX_RENDERBUFFER_SIZE");

  lua_pushinteger(L, GL_DEPTH_STENCIL);
  lua_setfield(L, idx, "DEPTH_STENCIL");

  lua_pushinteger(L, GL_UNSIGNED_INT_24_8);
  lua_setfield(L, idx, "UNSIGNED_INT_24_8");

  lua_pushinteger(L, GL_DEPTH24_STENCIL8);
  lua_setfield(L, idx, "DEPTH24_STENCIL8");

  lua_pushinteger(L, GL_UNSIGNED_NORMALIZED);
  lua_setfield(L, idx, "UNSIGNED_NORMALIZED");

  lua_pushinteger(L, GL_FRAMEBUFFER_BINDING);
  lua_setfield(L, idx, "FRAMEBUFFER_BINDING");

  lua_pushinteger(L, GL_DRAW_FRAMEBUFFER_BINDING);
  lua_setfield(L, idx, "DRAW_FRAMEBUFFER_BINDING");

  lua_pushinteger(L, GL_RENDERBUFFER_BINDING);
  lua_setfield(L, idx, "RENDERBUFFER_BINDING");

  lua_pushinteger(L, GL_READ_FRAMEBUFFER);
  lua_setfield(L, idx, "READ_FRAMEBUFFER");

  lua_pushinteger(L, GL_DRAW_FRAMEBUFFER);
  lua_setfield(L, idx, "DRAW_FRAMEBUFFER");

  lua_pushinteger(L, GL_READ_FRAMEBUFFER_BINDING);
  lua_setfield(L, idx, "READ_FRAMEBUFFER_BINDING");

  lua_pushinteger(L, GL_RENDERBUFFER_SAMPLES);
  lua_setfield(L, idx, "RENDERBUFFER_SAMPLES");

  lua_pushinteger(L, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE);
  lua_setfield(L, idx, "FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE");

  lua_pushinteger(L, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME);
  lua_setfield(L, idx, "FRAMEBUFFER_ATTACHMENT_OBJECT_NAME");

  lua_pushinteger(L, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL);
  lua_setfield(L, idx, "FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL");

  lua_pushinteger(L, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE);
  lua_setfield(L, idx, "FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE");

  lua_pushinteger(L, GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER);
  lua_setfield(L, idx, "FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER");

  lua_pushinteger(L, GL_FRAMEBUFFER_COMPLETE);
  lua_setfield(L, idx, "FRAMEBUFFER_COMPLETE");

  lua_pushinteger(L, GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
  lua_setfield(L, idx, "FRAMEBUFFER_INCOMPLETE_ATTACHMENT");

  lua_pushinteger(L, GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
  lua_setfield(L, idx, "FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");

  lua_pushinteger(L, GL_FRAMEBUFFER_UNSUPPORTED);
  lua_setfield(L, idx, "FRAMEBUFFER_UNSUPPORTED");

  lua_pushinteger(L, GL_MAX_COLOR_ATTACHMENTS);
  lua_setfield(L, idx, "MAX_COLOR_ATTACHMENTS");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT0);
  lua_setfield(L, idx, "COLOR_ATTACHMENT0");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT1);
  lua_setfield(L, idx, "COLOR_ATTACHMENT1");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT2);
  lua_setfield(L, idx, "COLOR_ATTACHMENT2");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT3);
  lua_setfield(L, idx, "COLOR_ATTACHMENT3");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT4);
  lua_setfield(L, idx, "COLOR_ATTACHMENT4");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT5);
  lua_setfield(L, idx, "COLOR_ATTACHMENT5");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT6);
  lua_setfield(L, idx, "COLOR_ATTACHMENT6");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT7);
  lua_setfield(L, idx, "COLOR_ATTACHMENT7");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT8);
  lua_setfield(L, idx, "COLOR_ATTACHMENT8");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT9);
  lua_setfield(L, idx, "COLOR_ATTACHMENT9");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT10);
  lua_setfield(L, idx, "COLOR_ATTACHMENT10");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT11);
  lua_setfield(L, idx, "COLOR_ATTACHMENT11");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT12);
  lua_setfield(L, idx, "COLOR_ATTACHMENT12");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT13);
  lua_setfield(L, idx, "COLOR_ATTACHMENT13");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT14);
  lua_setfield(L, idx, "COLOR_ATTACHMENT14");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT15);
  lua_setfield(L, idx, "COLOR_ATTACHMENT15");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT16);
  lua_setfield(L, idx, "COLOR_ATTACHMENT16");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT17);
  lua_setfield(L, idx, "COLOR_ATTACHMENT17");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT18);
  lua_setfield(L, idx, "COLOR_ATTACHMENT18");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT19);
  lua_setfield(L, idx, "COLOR_ATTACHMENT19");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT20);
  lua_setfield(L, idx, "COLOR_ATTACHMENT20");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT21);
  lua_setfield(L, idx, "COLOR_ATTACHMENT21");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT22);
  lua_setfield(L, idx, "COLOR_ATTACHMENT22");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT23);
  lua_setfield(L, idx, "COLOR_ATTACHMENT23");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT24);
  lua_setfield(L, idx, "COLOR_ATTACHMENT24");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT25);
  lua_setfield(L, idx, "COLOR_ATTACHMENT25");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT26);
  lua_setfield(L, idx, "COLOR_ATTACHMENT26");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT27);
  lua_setfield(L, idx, "COLOR_ATTACHMENT27");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT28);
  lua_setfield(L, idx, "COLOR_ATTACHMENT28");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT29);
  lua_setfield(L, idx, "COLOR_ATTACHMENT29");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT30);
  lua_setfield(L, idx, "COLOR_ATTACHMENT30");

  lua_pushinteger(L, GL_COLOR_ATTACHMENT31);
  lua_setfield(L, idx, "COLOR_ATTACHMENT31");

  lua_pushinteger(L, GL_DEPTH_ATTACHMENT);
  lua_setfield(L, idx, "DEPTH_ATTACHMENT");

  lua_pushinteger(L, GL_STENCIL_ATTACHMENT);
  lua_setfield(L, idx, "STENCIL_ATTACHMENT");

  lua_pushinteger(L, GL_FRAMEBUFFER);
  lua_setfield(L, idx, "FRAMEBUFFER");

  lua_pushinteger(L, GL_RENDERBUFFER);
  lua_setfield(L, idx, "RENDERBUFFER");

  lua_pushinteger(L, GL_RENDERBUFFER_WIDTH);
  lua_setfield(L, idx, "RENDERBUFFER_WIDTH");

  lua_pushinteger(L, GL_RENDERBUFFER_HEIGHT);
  lua_setfield(L, idx, "RENDERBUFFER_HEIGHT");

  lua_pushinteger(L, GL_RENDERBUFFER_INTERNAL_FORMAT);
  lua_setfield(L, idx, "RENDERBUFFER_INTERNAL_FORMAT");

  lua_pushinteger(L, GL_STENCIL_INDEX8);
  lua_setfield(L, idx, "STENCIL_INDEX8");

  lua_pushinteger(L, GL_RENDERBUFFER_RED_SIZE);
  lua_setfield(L, idx, "RENDERBUFFER_RED_SIZE");

  lua_pushinteger(L, GL_RENDERBUFFER_GREEN_SIZE);
  lua_setfield(L, idx, "RENDERBUFFER_GREEN_SIZE");

  lua_pushinteger(L, GL_RENDERBUFFER_BLUE_SIZE);
  lua_setfield(L, idx, "RENDERBUFFER_BLUE_SIZE");

  lua_pushinteger(L, GL_RENDERBUFFER_ALPHA_SIZE);
  lua_setfield(L, idx, "RENDERBUFFER_ALPHA_SIZE");

  lua_pushinteger(L, GL_RENDERBUFFER_DEPTH_SIZE);
  lua_setfield(L, idx, "RENDERBUFFER_DEPTH_SIZE");

  lua_pushinteger(L, GL_RENDERBUFFER_STENCIL_SIZE);
  lua_setfield(L, idx, "RENDERBUFFER_STENCIL_SIZE");

  lua_pushinteger(L, GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE);
  lua_setfield(L, idx, "FRAMEBUFFER_INCOMPLETE_MULTISAMPLE");

  lua_pushinteger(L, GL_MAX_SAMPLES);
  lua_setfield(L, idx, "MAX_SAMPLES");

  lua_pushinteger(L, GL_HALF_FLOAT);
  lua_setfield(L, idx, "HALF_FLOAT");

  lua_pushinteger(L, GL_MAP_READ_BIT);
  lua_setfield(L, idx, "MAP_READ_BIT");

  lua_pushinteger(L, GL_MAP_WRITE_BIT);
  lua_setfield(L, idx, "MAP_WRITE_BIT");

  lua_pushinteger(L, GL_MAP_INVALIDATE_RANGE_BIT);
  lua_setfield(L, idx, "MAP_INVALIDATE_RANGE_BIT");

  lua_pushinteger(L, GL_MAP_INVALIDATE_BUFFER_BIT);
  lua_setfield(L, idx, "MAP_INVALIDATE_BUFFER_BIT");

  lua_pushinteger(L, GL_MAP_FLUSH_EXPLICIT_BIT);
  lua_setfield(L, idx, "MAP_FLUSH_EXPLICIT_BIT");

  lua_pushinteger(L, GL_MAP_UNSYNCHRONIZED_BIT);
  lua_setfield(L, idx, "MAP_UNSYNCHRONIZED_BIT");

  lua_pushinteger(L, GL_RG);
  lua_setfield(L, idx, "RG");

  lua_pushinteger(L, GL_RG_INTEGER);
  lua_setfield(L, idx, "RG_INTEGER");

  lua_pushinteger(L, GL_R8);
  lua_setfield(L, idx, "R8");

  lua_pushinteger(L, GL_RG8);
  lua_setfield(L, idx, "RG8");

  lua_pushinteger(L, GL_R16F);
  lua_setfield(L, idx, "R16F");

  lua_pushinteger(L, GL_R32F);
  lua_setfield(L, idx, "R32F");

  lua_pushinteger(L, GL_RG16F);
  lua_setfield(L, idx, "RG16F");

  lua_pushinteger(L, GL_RG32F);
  lua_setfield(L, idx, "RG32F");

  lua_pushinteger(L, GL_R8I);
  lua_setfield(L, idx, "R8I");

  lua_pushinteger(L, GL_R8UI);
  lua_setfield(L, idx, "R8UI");

  lua_pushinteger(L, GL_R16I);
  lua_setfield(L, idx, "R16I");

  lua_pushinteger(L, GL_R16UI);
  lua_setfield(L, idx, "R16UI");

  lua_pushinteger(L, GL_R32I);
  lua_setfield(L, idx, "R32I");

  lua_pushinteger(L, GL_R32UI);
  lua_setfield(L, idx, "R32UI");

  lua_pushinteger(L, GL_RG8I);
  lua_setfield(L, idx, "RG8I");

  lua_pushinteger(L, GL_RG8UI);
  lua_setfield(L, idx, "RG8UI");

  lua_pushinteger(L, GL_RG16I);
  lua_setfield(L, idx, "RG16I");

  lua_pushinteger(L, GL_RG16UI);
  lua_setfield(L, idx, "RG16UI");

  lua_pushinteger(L, GL_RG32I);
  lua_setfield(L, idx, "RG32I");

  lua_pushinteger(L, GL_RG32UI);
  lua_setfield(L, idx, "RG32UI");

  lua_pushinteger(L, GL_VERTEX_ARRAY_BINDING);
  lua_setfield(L, idx, "VERTEX_ARRAY_BINDING");

  return 0;
}

int L_require(lua_State *L) {
  lua_newtable(L);

  lua_pushConstants(L, -1);

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

  lua_pushcfunction(L, L_glDrawElements);
  lua_setfield(L, -2, "drawElements");

  lua_pushcfunction(L, L_glGenBuffer);
  lua_setfield(L, -2, "genBuffer");

  lua_pushcfunction(L, L_glDeleteBuffer);
  lua_setfield(L, -2, "deleteBuffer");

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

  lua_pushcfunction(L, L_glUseProgram);
  lua_setfield(L, -2, "useProgram");

  lua_pushcfunction(L, L_glUniform1i);
  lua_setfield(L, -2, "uniform1i");

  lua_pushcfunction(L, L_glGenTexture);
  lua_setfield(L, -2, "genTexture");

  lua_pushcfunction(L, L_glTexImage2D);
  lua_setfield(L, -2, "texImage2D");

  lua_pushcfunction(L, L_glPixelStorei);
  lua_setfield(L, -2, "pixelStorei");

  lua_pushcfunction(L, L_glTexParameteri);
  lua_setfield(L, -2, "texParameteri");

  lua_pushcfunction(L, L_glDeleteTexture);
  lua_setfield(L, -2, "deleteTexture");

  lua_pushcfunction(L, L_glBindTexture);
  lua_setfield(L, -2, "bindTexture");

  lua_pushcfunction(L, L_glActivateTexture);
  lua_setfield(L, -2, "activateTexture");

  lua_pushcfunction(L, L_glGenFramebuffer);
  lua_setfield(L, -2, "genFramebuffer");

  lua_pushcfunction(L, L_glBindFramebuffer);
  lua_setfield(L, -2, "bindFramebuffer");

  lua_pushcfunction(L, L_glDeleteFramebuffer);
  lua_setfield(L, -2, "deleteFramebuffer");

  lua_pushcfunction(L, L_glGenRenderbuffer);
  lua_setfield(L, -2, "genRenderbuffer");

  lua_pushcfunction(L, L_glRenderbufferStorage);
  lua_setfield(L, -2, "renderbufferStorage");

  lua_pushcfunction(L, L_glBindRenderbuffer);
  lua_setfield(L, -2, "bindRenderbuffer");

  lua_pushcfunction(L, L_glDeleteRenderbuffer);
  lua_setfield(L, -2, "deleteRenderbuffer");

  lua_pushcfunction(L, L_glFramebufferRenderbuffer);
  lua_setfield(L, -2, "framebufferRenderbuffer");

  lua_pushcfunction(L, L_glFramebufferTexture2D);
  lua_setfield(L, -2, "framebufferTexture2D");

  return 1;
}
} // namespace

namespace hello::lua::opengl {
void openlibs(lua_State *L) {
  luaL_requiref(L, "opengl", L_require, false);
  lua_pop(L, 1);
}
} // namespace hello::lua::opengl