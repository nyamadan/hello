
#include "./lua_glslang.hpp"

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

bool glalang_InitializeProcess() { return glslang::InitializeProcess(); }

void glslang_FinalizeProcess() { glslang::FinalizeProcess(); }

glslang::TProgram *glslang_TProgram_new() { return new glslang::TProgram(); }

void glslang_TProgram_delete(glslang::TProgram *program) { delete program; }

const char *glslang_TProgram_getInfoLog(glslang::TProgram *program) {
  return program->getInfoLog();
}

void glslang_TProgram_addShader(glslang::TProgram *program,
                                glslang::TShader *shader) {
  program->addShader(shader);
}

bool glslang_TProgram_link(glslang::TProgram *program, EShMessages messages) {
  return program->link(messages);
}

glslang::TIntermediate *
glslang_TProgram_getIntermediate(glslang::TProgram *program,
                                 EShLanguage stage) {
  return program->getIntermediate(stage);
}

unsigned int *glslang_GlslangToSpirv(glslang::TIntermediate *intermediate,
                                     int *wordCount) {
  auto spirv = std::vector<unsigned int>();
  glslang::GlslangToSpv(*intermediate, spirv);
  auto pBuffer = reinterpret_cast<unsigned int *>(
      malloc(sizeof(unsigned int) * spirv.size()));
  memcpy(pBuffer, spirv.data(), spirv.size() * sizeof(unsigned int));
  *wordCount = static_cast<int32_t>(spirv.size());
  return pBuffer;
}

glslang::TShader *glslang_TShader_new(EShLanguage stage) {
  return new glslang::TShader(stage);
}

void glslang_TShader_delete(glslang::TShader *shader) { delete shader; }

const char *glslang_TShader_getInfoLog(glslang::TShader *shader) {
  return shader->getInfoLog();
}

void glslang_TShader_setString(glslang::TShader *shader, const char *s) {
  const char *const sources[] = {s};
  shader->setStrings(sources, 1);
}

void glslang_TShader_setEnvInput(glslang::TShader *shader,
                                 glslang::EShSource lang, EShLanguage envStage,
                                 glslang::EShClient client, int version) {
  shader->setEnvInput(lang, envStage, client, version);
}

void glslang_TShader_setEnvClient(glslang::TShader *shader,
                                  glslang::EShClient client,
                                  glslang::EShTargetClientVersion version) {
  shader->setEnvClient(client, version);
}

void glslang_TShader_setEnvTarget(glslang::TShader *shader,
                                  glslang::EShTargetLanguage lang,
                                  glslang::EShTargetLanguageVersion version) {
  shader->setEnvTarget(lang, version);
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
  return 1;
}
} // namespace

namespace hello::lua::glslang {
void openlibs(lua_State *L) {
  lua_newtable(L);
  luaL_requiref(L, "glslang", L_require, false);
  lua_pop(L, 1);
}
} // namespace hello::lua::glslang
