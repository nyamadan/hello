#pragma once
#include <embree3/rtcore.h>
#include <OpenImageDenoise/oidn.hpp>
#include <glm/glm.hpp>

#include "image_buffer.hpp"
#include "ray_tracer_camera.hpp"
#include "xorshift128plus.hpp"

enum RenderingMode {
    ALBEDO = 0,
    EMISSIVE,
    NORMAL,
    TANGENT,
    BITANGENT,
    PATHTRACING,
};

const char *const RenderingModeName[] = {
    "Albedo", "Emissive", "Normal", "Tangent", "Bitangent", "Path Tracing",
};

struct IntersectContext;

class RayTracer {
  private:
    RenderingMode mode = static_cast<RenderingMode>(0);

    const int32_t TILE_SIZE_X = 4;
    const int32_t TILE_SIZE_Y = 4;

    ImageBuffer image;

    std::shared_ptr<glm::vec3> skybox;
    int32_t skyboxWidth = 0;
    int32_t skyboxHeight = 0;

    bool enableSuperSamples = true;
    int32_t maxSamples = 200;
    int32_t samples = 0;

    glm::vec3 renderPixel(RTCScene scene, const RayTracerCamera &camera,
                          xorshift128plus_state &randomState, float x, float y);
    glm::vec3 renderNormal(RTCScene scene, const RayTracerCamera &camera,
                           float x, float y);
    glm::vec3 renderTangent(RTCScene scene, const RayTracerCamera &camera,
                            float x, float y);
    glm::vec3 renderBitangent(RTCScene scene, const RayTracerCamera &camera,
                              float x, float y);
    glm::vec3 renderEmissive(RTCScene scene, const RayTracerCamera &camera,
                             float x, float y);
    glm::vec3 renderAlbedo(RTCScene scene, const RayTracerCamera &camera,
                           float x, float y);
    glm::vec3 renderPathTrace(RTCScene scene, const RayTracerCamera &camera,
                              xorshift128plus_state &randomState, float x,
                              float y);

    glm::vec3 computeDiffuse(RTCScene scene, const RayTracerCamera &camera,
                             xorshift128plus_state &randomState,
                             IntersectContext context, int32_t depth,
                             const glm::vec4 &baseColor, const glm::vec3 &p,
                             const glm::vec3 &N);

    glm::vec3 computeSpecular(RTCScene scene, const RayTracerCamera &camera,
                              xorshift128plus_state &randomState,
                              IntersectContext context, int32_t depth,
                              const glm::vec4 &baseColor, float roughness,
                              const glm::vec3 &p, const glm::vec3 &N,
                              const glm::vec3 &V);
    glm::vec3 computeReflection(RTCScene scene, const RayTracerCamera &camera,
                                xorshift128plus_state &randomState,
                                IntersectContext context, int32_t depth,
                                const glm::vec4 &baseColor, float roughness,
                                float metalness, const glm::vec3 &p,
                                const glm::vec3 &N, const glm::vec3 &V);
    glm::vec3 computeRefraction(RTCScene scene, const RayTracerCamera &camera,
                                xorshift128plus_state &randomState,
                                IntersectContext context, int32_t depth,
                                const glm::vec4 &baseColor, float roughness,
                                float metalness, const glm::vec3 &p,
                                const glm::vec3 &normal,
                                const glm::vec3 &orientingNormal,
                                const glm::vec3 &rayDir);
    glm::vec3 importanceSampleGGX(const glm::vec2 &Xi, const glm::vec3 &N,
                                  float roughness);
    glm::vec3 radiance(RTCScene scene, const RayTracerCamera &camera,
                       xorshift128plus_state &randomState,
                       IntersectContext context, RTCRayHit &ray, int32_t depth);
    void renderTile(RTCScene scene, const RayTracerCamera &camera,
                    xorshift128plus_state &randomState, int tileIndex,
                    const int numTilesX, const int numTilesY);
    void initIntersectContext(IntersectContext *context, RTCScene scene,
                              xorshift128plus_state *randomState);
    glm::vec2 toRadialCoords(glm::vec3 coords);

  public:
    RayTracer(){};
    RayTracer(const glm::i32vec2 &size);
    void setRenderingMode(RenderingMode mode);
    void setEnableSuperSampling(bool enableSuperSampling);
    RenderingMode getRenderingMode() const;
    bool render(RTCScene scene, const RayTracerCamera &camera);
    void denoise(oidn::DeviceRef denoiser);
    void finish(bool filtered, bool yuv420);
    void reset();
    void resize(const glm::i32vec2 &size);
    const ImageBuffer &getImage() const;
    int32_t getSamples() const;
    int32_t getMaxSamples() const;
    void setMaxSamples(int32_t samples);
    void intersectionFilter(const struct RTCFilterFunctionNArguments *args);
    void loadSkybox(const std::string &path);
};

struct IntersectContext : public RTCIntersectContext {
    RayTracer *raytracer;
    RTCScene scene;
    xorshift128plus_state *randomState;

    int depth;
    glm::vec4 baseColor;
    glm::vec3 emissive;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    glm::vec2 texcoord0;
    float roughness;
    float metalness;
};
