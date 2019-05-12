#pragma once
#include <glm/glm.hpp>

#include <embree3/rtcore.h>

class RayTracer {
  private:
    const int32_t TILE_SIZE_X = 128;
    const int32_t TILE_SIZE_Y = 128;

    glm::vec3 renderPixelStandard(RTCScene scene,
                                  const glm::vec3 vertex_colors[],
                                  const glm::vec3 face_colors[], float x,
                                  float y, float width, float height,
                                  glm::vec3 cameraFrom, glm::vec3 cameraDir);
    void renderTileStandard(int tileIndex, RTCScene scene,
                            const glm::vec3 vertex_colors[],
                            const glm::vec3 face_colors[], glm::u8vec3 *pixels,
                            const unsigned int width, const unsigned int height,
                            const glm::vec3 cameraFrom,
                            const glm::vec3 cameraDir, const int numTilesX,
                            const int numTilesY);

  public:
    void render(RTCScene scene, const glm::vec3 vertex_colors[],
                           const glm::vec3 face_colors[], glm::u8vec3 *pixels,
                           const uint32_t width, const uint32_t height);
};
