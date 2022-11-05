#include "./lua_glslang.hpp"
#include "../buffer/lua_buffer.hpp"

#include <cstdint>
#include <cstdlib>

#include <array>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/SPIRV/disassemble.h>

namespace {
const char *const SHADER_NAME = "glslang_Shader";
const char *const PROGRAM_NAME = "glslang_Program";
const char *const INTERMEDIATE_NAME = "glslang_Intermediate";

struct Shader {
  glslang::TShader *data;
};

struct Program {
  glslang::TProgram *data;
};

struct Intermediate {
  glslang::TIntermediate *data;
};

const TBuiltInResource DefaultTBuiltInResource = {
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV = */ 256,
    /* .maxMeshOutputPrimitivesNV = */ 512,
    /* .maxMeshWorkGroupSizeX_NV = */ 32,
    /* .maxMeshWorkGroupSizeY_NV = */ 1,
    /* .maxMeshWorkGroupSizeZ_NV = */ 1,
    /* .maxTaskWorkGroupSizeX_NV = */ 32,
    /* .maxTaskWorkGroupSizeY_NV = */ 1,
    /* .maxTaskWorkGroupSizeZ_NV = */ 1,
    /* .maxMeshViewCountNV = */ 4,
    /* .maxDualSourceDrawBuffersEXT = */ 1,

    /* .limits = */
    {
        /* .nonInductiveForLoops = */ 1,
        /* .whileLoops = */ 1,
        /* .doWhileLoops = */ 1,
        /* .generalUniformIndexing = */ 1,
        /* .generalAttributeMatrixVectorIndexing = */ 1,
        /* .generalVaryingIndexing = */ 1,
        /* .generalSamplerIndexing = */ 1,
        /* .generalVariableIndexing = */ 1,
        /* .generalConstantMatrixVectorIndexing = */ 1,
    }};

typedef char *(*IncludeCallback)(const char *headerName,
                                 const char *includerName, int depth);
typedef void (*ReleaseCallback)(void *);

class Includer : public glslang::TShader::Includer {
private:
  IncludeCallback includeLocal_;
  IncludeCallback includeSystem_;
  ReleaseCallback release;

  IncludeResult *unpackBuffer(char *buf);

public:
  Includer(IncludeCallback includeLocal, IncludeCallback includeSystem,
           ReleaseCallback release);

  virtual IncludeResult *includeLocal(const char *headerName,
                                      const char *includerName,
                                      size_t inclusionDepth) override;
  virtual IncludeResult *includeSystem(const char *headerName,
                                       const char * /*includerName*/,
                                       size_t /*inclusionDepth*/) override;
  virtual void releaseInclude(IncludeResult *result) override;
  virtual ~Includer() override;
};

Includer::Includer(IncludeCallback includeLocal, IncludeCallback includeSystem,
                   ReleaseCallback release) {
  this->includeLocal_ = includeLocal;
  this->includeSystem_ = includeSystem;
  this->release = release;
}

glslang::TShader::Includer::IncludeResult *Includer::unpackBuffer(char *buf) {
  auto p = buf;
  auto headerNameLength_ = *reinterpret_cast<const int *>(p);
  p += 4;
  auto headerName_ = p;
  p += headerNameLength_;
  auto headerDataLength_ = *reinterpret_cast<const int *>(p);
  p += 4;
  auto headerData_ = p;
  p += headerDataLength_;

  return new IncludeResult(headerName_, headerData_,
                           strnlen(headerData_, headerDataLength_), buf);
}

glslang::TShader::Includer::IncludeResult *
Includer::includeLocal(const char *headerName, const char *includerName,
                       size_t inclusionDepth) {
  auto buf = this->includeLocal_(headerName, includerName,
                                 static_cast<int32_t>(inclusionDepth));
  if (buf == nullptr) {
    return nullptr;
  }

  return unpackBuffer(buf);
}

glslang::TShader::Includer::IncludeResult *
Includer::includeSystem(const char *headerName, const char *includerName,
                        size_t inclusionDepth) {
  auto buf = this->includeSystem_(headerName, includerName,
                                  static_cast<int32_t>(inclusionDepth));
  if (buf == nullptr) {
    return nullptr;
  }

  return unpackBuffer(buf);
}

void Includer::releaseInclude(IncludeResult *result) {
  if (result != nullptr) {
    this->release(result->userData);
    delete result;
  }
}

Includer::~Includer() {}

Includer *Includer_new(IncludeCallback includeLocal,
                       IncludeCallback includeSystem, ReleaseCallback release) {
  return new Includer(includeLocal, includeSystem, release);
}

int L_InitializeProcess(lua_State *L) {
  lua_pushboolean(L, glslang::InitializeProcess());
  return 1;
}

int L_FinalizeProcess(lua_State *) {
  glslang::FinalizeProcess();
  return 0;
}

int L_newProgram(lua_State *L) {
  auto pProgram = static_cast<Program *>(lua_newuserdata(L, sizeof(Program)));
  pProgram->data = new glslang::TProgram();
  luaL_setmetatable(L, PROGRAM_NAME);
  return 1;
}

int L_Program___gc(lua_State *L) {
  auto pProgram = static_cast<Program *>(luaL_checkudata(L, 1, PROGRAM_NAME));
  delete pProgram->data;
  pProgram->data = nullptr;
  return 0;
}

int L_Program_getInfoLog(lua_State *L) {
  auto pProgram = static_cast<Program *>(luaL_checkudata(L, 1, PROGRAM_NAME));
  luaL_argcheck(L, pProgram->data != nullptr, 1, "Program is null value.");
  lua_pushstring(L, pProgram->data->getInfoLog());
  return 1;
}

int L_Program_addShader(lua_State *L) {
  auto pProgram = static_cast<Program *>(luaL_checkudata(L, 1, PROGRAM_NAME));
  luaL_argcheck(L, pProgram->data != nullptr, 1, "Program is null value.");
  auto pShader = static_cast<Shader *>(luaL_checkudata(L, 2, PROGRAM_NAME));
  luaL_argcheck(L, pShader->data != nullptr, 2, "Shader is null value.");
  pProgram->data->addShader(pShader->data);
  return 0;
}

int L_Program_link(lua_State *L) {
  auto pProgram = static_cast<Program *>(luaL_checkudata(L, 1, PROGRAM_NAME));
  luaL_argcheck(L, pProgram->data != nullptr, 1, "Program is null value.");
  auto messages = static_cast<EShMessages>(luaL_checkinteger(L, 2));
  lua_pushboolean(L, pProgram->data->link(messages));
  return 1;
}

int L_Program_getIntermediate(lua_State *L) {
  auto pProgram = static_cast<Program *>(luaL_checkudata(L, 1, PROGRAM_NAME));
  luaL_argcheck(L, pProgram->data != nullptr, 1, "Program is null value.");

  auto stage = static_cast<EShLanguage>(luaL_checkinteger(L, 2));

  auto pIntermediate =
      static_cast<Intermediate *>(lua_newuserdata(L, sizeof(Intermediate)));
  pIntermediate->data = pProgram->data->getIntermediate(stage);
  luaL_setmetatable(L, INTERMEDIATE_NAME);
  return 1;
}

int L_Intermedate___gc(lua_State *L) {
  auto pIntermedate =
      static_cast<Intermediate *>(luaL_checkudata(L, 1, INTERMEDIATE_NAME));
  pIntermedate->data = nullptr;
  return 0;
}

int L_GlslangToSpirv(lua_State *L) {
  auto pProgram = static_cast<Program *>(luaL_checkudata(L, 1, PROGRAM_NAME));
  luaL_argcheck(L, pProgram->data != nullptr, 1, "Program is null value.");

  auto pIntermediate =
      static_cast<Intermediate *>(luaL_checkudata(L, 2, INTERMEDIATE_NAME));
  luaL_argcheck(L, pIntermediate->data != nullptr, 2,
                "Intermediate is null value.");

  auto spirv = std::vector<unsigned int>();
  glslang::GlslangToSpv(*(pIntermediate->data), spirv);
  lua_pushinteger(
      L, static_cast<lua_Integer>(sizeof(unsigned int) * spirv.size()));
  hello::lua::buffer::alloc(L);
  auto ppBuffer = static_cast<hello::lua::buffer::Buffer *>(
      luaL_checkudata(L, -1, "Buffer"));
  auto pBuffer = ppBuffer->p;
  memcpy(pBuffer, spirv.data(), spirv.size() * sizeof(unsigned int));
  return 1;
}

int L_newShader(lua_State *L) {
  auto stage = luaL_checkinteger(L, 1);
  luaL_argcheck(L,
                stage == EShLanguage::EShLangFragment ||
                    stage == EShLanguage::EShLangVertex,
                1, "only support for fragment or vertex");
  auto pShader = static_cast<Shader *>(lua_newuserdata(L, sizeof(Shader)));
  pShader->data = new glslang::TShader(static_cast<EShLanguage>(stage));
  luaL_setmetatable(L, SHADER_NAME);
  return 1;
}

int L_Shader___gc(lua_State *L) {
  auto pShader = static_cast<Shader *>(luaL_checkudata(L, 1, SHADER_NAME));
  delete pShader->data;
  pShader->data = nullptr;
  return 0;
}

int L_Shader_getInfoLog(lua_State *L) {
  auto pShader = static_cast<Shader *>(luaL_checkudata(L, 1, SHADER_NAME));
  luaL_argcheck(L, pShader->data != nullptr, 1, "Shader is null value.");
  lua_pushstring(L, pShader->data->getInfoLog());
  return 1;
}

int L_Shader_setString(lua_State *L) {
  auto pShader = static_cast<Shader *>(luaL_checkudata(L, 1, SHADER_NAME));
  luaL_argcheck(L, pShader->data != nullptr, 1, "Shader is null value.");
  const char *s = luaL_checkstring(L, 2);
  const char *const sources[] = {s};
  pShader->data->setStrings(sources, 1);
  return 0;
}

int L_Shader_setEnvInput(lua_State *L) {
  auto pShader = static_cast<Shader *>(luaL_checkudata(L, 1, SHADER_NAME));
  luaL_argcheck(L, pShader->data != nullptr, 1, "Shader is null value.");
  auto lang = static_cast<glslang::EShSource>(luaL_checkinteger(L, 2));
  auto envStage = static_cast<EShLanguage>(luaL_checkinteger(L, 3));
  auto client = static_cast<glslang::EShClient>(luaL_checkinteger(L, 4));
  auto version = static_cast<int>(luaL_checkinteger(L, 5));
  pShader->data->setEnvInput(lang, envStage, client, version);
  return 0;
}

int L_Shader_setEnvClient(lua_State *L) {
  auto pShader = static_cast<Shader *>(luaL_checkudata(L, 1, SHADER_NAME));
  luaL_argcheck(L, pShader->data != nullptr, 1, "Shader is null value.");
  auto client = static_cast<glslang::EShClient>(luaL_checkinteger(L, 2));
  auto version =
      static_cast<glslang::EshTargetClientVersion>(luaL_checkinteger(L, 3));
  pShader->data->setEnvClient(client, version);
  return 0;
}

int L_Shader_setEnvTarget(lua_State *L) {
  auto pShader = static_cast<Shader *>(luaL_checkudata(L, 1, SHADER_NAME));
  luaL_argcheck(L, pShader->data != nullptr, 1, "Shader is null value.");
  auto lang = static_cast<glslang::EShTargetLanguage>(luaL_checkinteger(L, 2));
  auto version =
      static_cast<glslang::EShTargetLanguageVersion>(luaL_checkinteger(L, 3));
  pShader->data->setEnvTarget(lang, version);
  return 0;
}

bool glslang_TShader_parse(glslang::TShader *shader, int defaultVersion,
                           bool forwardCompatible, EShMessages messages,
                           Includer *includer) {
  if (includer != nullptr) {
    return shader->parse(&DefaultTBuiltInResource, defaultVersion,
                         forwardCompatible, messages, *includer);
  }

  return shader->parse(&DefaultTBuiltInResource, defaultVersion,
                       forwardCompatible, messages);
}

int L_require(lua_State *L) {
  lua_newtable(L);

  lua_pushcfunction(L, L_InitializeProcess);
  lua_setfield(L, -2, "initializeProcess");

  lua_pushcfunction(L, L_FinalizeProcess);
  lua_setfield(L, -2, "finalizeProcess");

  lua_pushcfunction(L, L_newProgram);
  lua_setfield(L, -2, "newProgram");

  lua_pushcfunction(L, L_newShader);
  lua_setfield(L, -2, "newShader");

  lua_pushcfunction(L, L_GlslangToSpirv);
  lua_setfield(L, -2, "glslangToSpirv");

  return 1;
}
} // namespace

namespace hello::lua::glslang {
void openlibs(lua_State *L) {
  luaL_newmetatable(L, SHADER_NAME);
  lua_pushcfunction(L, L_Shader___gc);
  lua_setfield(L, -2, "__gc");
  lua_newtable(L);
  lua_pushcfunction(L, L_Shader_getInfoLog);
  lua_setfield(L, -2, "getInfoLog");
  lua_pushcfunction(L, L_Shader_setString);
  lua_setfield(L, -2, "setString");
  lua_pushcfunction(L, L_Shader_setEnvInput);
  lua_setfield(L, -2, "setEnvInput");
  lua_pushcfunction(L, L_Shader_setEnvClient);
  lua_setfield(L, -2, "setEnvClient");
  lua_pushcfunction(L, L_Shader_setEnvTarget);
  lua_setfield(L, -2, "setEnvTarget");
  lua_setfield(L, -2, "__index");

  luaL_newmetatable(L, PROGRAM_NAME);
  lua_pushcfunction(L, L_Program___gc);
  lua_setfield(L, -2, "__gc");
  lua_newtable(L);
  lua_pushcfunction(L, L_Program_getInfoLog);
  lua_setfield(L, -2, "getInfoLog");
  lua_pushcfunction(L, L_Program_addShader);
  lua_setfield(L, -2, "addShader");
  lua_pushcfunction(L, L_Program_link);
  lua_setfield(L, -2, "link");
  lua_pushcfunction(L, L_Program_getIntermediate);
  lua_setfield(L, -2, "getIntermediate");
  lua_setfield(L, -2, "__index");

  luaL_newmetatable(L, INTERMEDIATE_NAME);
  lua_pushcfunction(L, L_Intermedate___gc);
  lua_setfield(L, -2, "__gc");

  lua_newtable(L);
  luaL_requiref(L, "glslang", L_require, false);

  lua_pop(L, 4);
}
} // namespace hello::lua::glslang