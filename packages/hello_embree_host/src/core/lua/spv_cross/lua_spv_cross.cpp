
#include "./lua_spv_cross.hpp"
#include <cstdlib>
#include <spirv_cross/spirv_glsl.hpp>
#include <utility>
#include <vector>

namespace {
int L_compile(lua_State *L) {
  size_t size;
  auto data = luaL_checklstring(L, 1, &size);

  auto es = true;
  auto version = 300;
  if (lua_istable(L, 2)) {
    lua_getfield(L, 2, "es");
    if (lua_isboolean(L, -1)) {
      es = !!lua_toboolean(L, -1);
    }

    lua_getfield(L, 2, "version");
    if (lua_isinteger(L, -1)) {
      version = static_cast<int>(lua_tointeger(L, -1));
    }
  }

  std::string result;

  try {
    spirv_cross::CompilerGLSL glsl(
        reinterpret_cast<const unsigned int *>(data),
        static_cast<size_t>(size / sizeof(unsigned int)));

    // The SPIR-V is now parsed, and we can perform reflection on it.
    spirv_cross::ShaderResources resources = glsl.get_shader_resources();

    // Get all sampled images in the shader.
    for (auto &resource : resources.sampled_images) {
      auto set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
      auto binding = glsl.get_decoration(resource.id, spv::DecorationBinding);

      // Modify the decoration to prepare it for GLSL.
      glsl.unset_decoration(resource.id, spv::DecorationDescriptorSet);

      // Some arbitrary remapping if we want.
      glsl.set_decoration(resource.id, spv::DecorationBinding,
                          set * 16 + binding);
    }

    // Set some options.
    spirv_cross::CompilerGLSL::Options options;
    options.version = version;
    options.es = es;
    glsl.set_common_options(options);

    // Compile to GLSL, ready to give to GL driver.
    result = glsl.compile();
  } catch (spirv_cross::CompilerError &e) {
    luaL_error(L, "spv_cross: %s\n", e.what());
  }

  lua_pushstring(L, result.c_str());
  return 1;
}

int L_require(lua_State *L) {
  lua_newtable(L);

  lua_pushcfunction(L, L_compile);
  lua_setfield(L, -2, "compile");

  return 1;
}
} // namespace

namespace hello::lua::spv_cross {

void openlibs(lua_State *L) {
  luaL_requiref(L, "spv_cross", L_require, false);

  lua_pop(L, 1);
}
} // namespace hello::lua::spv_cross
