#pragma once

#include <stdint.h>
#include <glm/glm.hpp>

class RayTracerCamera {
  private:
    uint32_t width;
    uint32_t height;
    float fov;
    float tnear;
    float tfar;

	glm::vec3 up;
	glm::vec3 origin;
	glm::vec3 dir;

  public:
    RayTracerCamera(int32_t width, int32_t height, float fov = 120.0f,
                    float tnear = 0.001f, float tfar = 1000.0f);

  public:
    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }
    float getFov() const { return fov; }
    float getNear() const { return tnear; }
    float getFar() const { return tfar; }
    float getAspect() const { return static_cast<float>(width) / height; }

	const glm::vec3 &getCameraOrigin() const { return origin; }
    const glm::vec3 &getCameraDir() const { return dir; }
    const glm::vec3 &getCameraUp() const { return up; }

    const void lookAt(const glm::vec3 &eye, const glm::vec3 &target,
                      const glm::vec3 &up);
};
