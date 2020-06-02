#pragma once
#include <embree3/rtcore.h>
#include <glm/glm.hpp>

#include "image_buffer.hpp"
#include "ray_tracer_camera.hpp"
#include "xorshift128plus.hpp"

class RayTracer {
  private:
    const int32_t TILE_SIZE_X = 256;
    const int32_t TILE_SIZE_Y = 256;

    const int32_t aoSample = 0;

    glm::vec3 renderPixelClassic(RTCScene scene, const RayTracerCamera &camera,
                                 xorshift128plus_state &randomState, float x,
                                 float y);
    glm::vec3 renderPixel(RTCScene scene, const RayTracerCamera &camera,
                          xorshift128plus_state &randomState, float x, float y);
    glm::vec3 radiance(RTCScene scene, const RayTracerCamera &camera,
                       xorshift128plus_state &randomState, RTCRayHit &ray, int32_t depth);
    void renderTile(RTCScene scene, const RayTracerCamera &camera,
                    xorshift128plus_state &randomState, ImageBuffer &image,
                    int tileIndex, const int numTilesX, const int numTilesY);

  public:
    void render(RTCScene scene, const RayTracerCamera &camera,
                ImageBuffer &image);
};
