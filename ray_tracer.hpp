#pragma once
#include <embree3/rtcore.h>
#include <OpenImageDenoise/oidn.hpp>
#include <glm/glm.hpp>

#include "image_buffer.hpp"
#include "ray_tracer_camera.hpp"
#include "xorshift128plus.hpp"

enum RenderingMode {
    ALBEDO,
    NORMAL,
    EMISSIVE,
    PATHTRACING,
};

const char *const RenderingModeName[] = {
    "Albedo",
    "Normal",
    "Emissive",
    "Path Tracing",
};

struct IntersectContext;

class RayTracer {
  private:
    RenderingMode mode;

    const int32_t TILE_SIZE_X = 4;
    const int32_t TILE_SIZE_Y = 4;

    ImageBuffer image;

    bool enableSuperSamples = true;
    int32_t maxSamples = 200;
    int32_t samples = 0;

    glm::vec3 renderPixel(RTCScene scene, const RayTracerCamera &camera,
                          xorshift128plus_state &randomState, float x, float y);
    glm::vec3 renderNormal(RTCScene scene, const RayTracerCamera &camera,
                           float x, float y);
    glm::vec3 renderEmissive(RTCScene scene, const RayTracerCamera &camera,
                             float x, float y);
    glm::vec3 renderAlbedo(RTCScene scene, const RayTracerCamera &camera,
                           float x, float y);
    glm::vec3 renderPathTrace(RTCScene scene, const RayTracerCamera &camera,
                              xorshift128plus_state &randomState, float x,
                              float y);

    glm::vec3 importanceSampleGGX(const glm::vec2 &Xi, const glm::vec3 &N, float roughness);
    glm::vec3 radiance(RTCScene scene, const RayTracerCamera &camera,
                       xorshift128plus_state &randomState,
                       IntersectContext context, RTCRayHit &ray, int32_t depth);
    void renderTile(RTCScene scene, const RayTracerCamera &camera,
                    xorshift128plus_state &randomState, int tileIndex,
                    const int numTilesX, const int numTilesY);
    void initIntersectContext(IntersectContext *context);

  public:
    RayTracer();
    RayTracer(const glm::i32vec2 &size);
    void setRenderingMode(RenderingMode mode);
    void setEnableSuperSampling(bool enableSuperSampling);
    RenderingMode getRenderingMode() const;
    bool render(RTCScene scene, const RayTracerCamera &camera,
                oidn::DeviceRef denoiser);
    void reset();
    void resize(const glm::i32vec2 &size);
    const ImageBuffer &getImage() const;
    int32_t getSamples() const;
    int32_t getMaxSamples() const;
    void setMaxSamples(int32_t samples);
    void intersectionFilter(const struct RTCFilterFunctionNArguments *args);
};

struct IntersectContext : public RTCIntersectContext {
    int depth;
    RayTracer *raytracer;
};
