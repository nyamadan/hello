#pragma once

#include <stdint.h>
#include <glm/glm.hpp>

class RayTracerCamera {
  private:
    float fov;
    float tnear;
    float tfar;

    glm::vec3 up;
    glm::vec3 origin;
    glm::vec3 dir;

  public:
    RayTracerCamera(float fov = 120.0f, float tnear = 0.001f,
                    float tfar = 1000.0f);

  public:
    float getFov() const { return fov; }
    float getNear() const { return tnear; }
    float getFar() const { return tfar; }

    glm::vec3 getCameraSide() const;
    glm::vec3 getCameraUp(const glm::vec3 &side) const;
    glm::vec3 getRayDir(float x, float y) const;

    const glm::vec3 &getCameraOrigin() const { return origin; }
    const glm::vec3 &getCameraDir() const { return dir; }
    const glm::vec3 &getCameraUp() const { return up; }

    void setBufferSize(int32_t width, int32_t height);
    void setCameraOrigin(const glm::vec3 &origin) { this->origin = origin; }
    const void setCameraDir(const glm::vec3 &dir) { this->dir = dir; }
    const void setCameraUp(const glm::vec3 &up) { this->up = up; }

    const void lookAt(const glm::vec3 &eye, const glm::vec3 &target,
                      const glm::vec3 &up);
};
