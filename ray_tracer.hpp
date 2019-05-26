#pragma once
#include <embree3/rtcore.h>
#include <glm/glm.hpp>

#include "ray_tracer_camera.hpp"

class RayTracer {
  private:
    const int32_t TILE_SIZE_X = 256;
    const int32_t TILE_SIZE_Y = 256;

	const int32_t aoSample = 100;

    glm::vec3 renderPixel(RTCScene scene, const RayTracerCamera &camera,
                          float x, float y);
    void renderTile(RTCScene scene, const RayTracerCamera &camera,
                    glm::u8vec3 *pixels, int tileIndex, const int numTilesX,
                    const int numTilesY);

  public:
    void render(RTCScene scene, const RayTracerCamera &camera,
                glm::u8vec3 *pixels);
};
