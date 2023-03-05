#include "./lua_sdl2_test.hpp"
#include <glad/glad.h>

using namespace hello::lua;

void LuaSDL2_Test::SetUpTestSuite() { SDL_Init(SDL_INIT_VIDEO); }

void LuaSDL2_Test::TearDownTestSuite() { SDL_Quit(); }

void LuaSDL2_Test::initLuaState() {
  L = luaL_newstate();
  luaL_openlibs(L);
  sdl2::openlibs(L);
  sdl2_image::openlibs(L);
  opengl::openlibs(L);
}

void LuaSDL2_Test::initOpenGL() {
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  auto context = SDL_GL_CreateContext(this->window);
  if (context == nullptr) {
    throw std::runtime_error("Could not initialize OpenGL");
  }
  SDL_GL_MakeCurrent(this->window, context);
  SDL_GL_SetSwapInterval(1);
#ifndef __EMSCRIPTEN__
  gladLoadGLLoader(SDL_GL_GetProcAddress);
#endif
}

void LuaSDL2_Test::initWindow(Uint32 flags) {
  this->window = SDL_CreateWindow("Hello", SDL_WINDOWPOS_UNDEFINED,
                                  SDL_WINDOWPOS_UNDEFINED, 1280, 720, flags);
}

void LuaSDL2_Test::pushWindow() {
  auto pWindow =
      static_cast<SDL_Window **>(lua_newuserdata(L, sizeof(SDL_Window *)));
  *pWindow = window;
  luaL_setmetatable(L, "SDL_Window");
}

void LuaSDL2_Test::initRenderer() {
  ASSERT_NE(nullptr, this->window);
  this->renderer =
      SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED);
}

void LuaSDL2_Test::pushRenderer() {
  auto pRenderer =
      static_cast<SDL_Renderer **>(lua_newuserdata(L, sizeof(SDL_Renderer *)));
  *pRenderer = renderer;
  luaL_setmetatable(L, "SDL_Renderer");
}

void LuaSDL2_Test::SetUp() { initLuaState(); }

void LuaSDL2_Test::TearDown() {
  if (this->renderer != nullptr) {
    SDL_DestroyRenderer(this->renderer);
    this->renderer = nullptr;
  }

  if (this->window != nullptr) {
    SDL_DestroyWindow(this->window);
    this->window = nullptr;
  }

  if (L != nullptr) {
    lua_close(L);
    L = nullptr;
  }
}
