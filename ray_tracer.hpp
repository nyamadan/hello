#pragma once
#include <embree3/rtcore.h>
#include <glm/glm.hpp>

#include "ray_tracer_camera.hpp"

class RayTracer {
  private:
    const int32_t TILE_SIZE_X = 128;
    const int32_t TILE_SIZE_Y = 128;

    glm::vec3 renderPixel(RTCScene scene, const RayTracerCamera &camera,
                          const glm::vec3 vertex_colors[],
                          const glm::vec3 face_colors[], float x, float y);
    void renderTile(RTCScene scene, const RayTracerCamera &camera,
                    const glm::vec3 vertex_colors[],
                    const glm::vec3 face_colors[], glm::u8vec3 *pixels,
                    int tileIndex, const int numTilesX, const int numTilesY);

  public:
    void render(RTCScene scene, const RayTracerCamera &camera,
                const glm::vec3 vertex_colors[], const glm::vec3 face_colors[],
                glm::u8vec3 *pixels);
};
