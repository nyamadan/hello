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

    glm::vec3 getCameraSide() const {
        return glm::normalize(glm::cross(this->dir, this->up));
    }

    glm::vec3 getCameraUp(const glm::vec3 &side) const {
        return glm::normalize(glm::cross(side, this->dir));
    }

    glm::vec3 getRayDir(float x, float y) const {
        const auto side = getCameraSide();
        const auto up = getCameraUp(side);
        const auto t = tanf(glm::radians(this->fov) * 0.5f);
        return glm::normalize(t * width / height * (x / width - 0.5f) * side +
                              t * (y / height - 0.5f) * up + this->dir);
    }

    const glm::vec3 &getCameraOrigin() const { return origin; }
    const glm::vec3 &getCameraDir() const { return dir; }
    const glm::vec3 &getCameraUp() const { return up; }

    const void setCameraOrigin(const glm::vec3 &origin) {
        this->origin = origin;
    }
    const void setCameraDir(const glm::vec3 &dir) { this->dir = dir; }
    const void setCameraUp(const glm::vec3 &up) { this->up = up; }

    const void lookAt(const glm::vec3 &eye, const glm::vec3 &target,
                      const glm::vec3 &up);
};
