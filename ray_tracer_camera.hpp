#pragma once

#include <stdint.h>
#include <glm/glm.hpp>

#include "xorshift128plus.hpp"

class RayTracerCamera {
  private:
    float fov;
    float tnear;
    float tfar;

    float lensRadius;
    float focusDistance;

    glm::vec3 up;
    glm::vec3 origin;
    glm::vec3 dir;

    bool isEquirectangula = false;

  public:
    RayTracerCamera();

    float getFov() const { return fov; }
    float getNear() const { return tnear; }
    float getFar() const { return tfar; }

    glm::vec3 getCameraSide() const;
    glm::vec3 getCameraUp(const glm::vec3 &side) const;
    glm::vec3 getRayDir(float x, float y) const;
    glm::vec3 getRayDirEquirectangular(float x, float y, int32_t width,
                                       int32_t height) const;
    void getRayInfo(glm::vec3 &rayDir, glm::vec3 &rayOrigin, float x, float y,
                    xorshift128plus_state &randomState) const;
    void getRayInfoEquirectangular(glm::vec3 &rayDir, glm::vec3 &rayOrigin,
                                   float x, float y,
                                   xorshift128plus_state &randomState,
                                   int32_t width, int32_t height) const;
    bool getIsEquirectangula() const { return isEquirectangula; }

    const glm::vec3 &getCameraOrigin() const { return origin; }
    const glm::vec3 &getCameraDir() const { return dir; }
    const glm::vec3 &getCameraUp() const { return up; }

    void setBufferSize(int32_t width, int32_t height);
    void setNear(float tnear) { this->tnear = tnear; }
    void setFar(float tfar) { this->tfar = tfar; }
    void setCameraFov(float fov) { this->fov = fov; }
    void setCameraOrigin(const glm::vec3 &origin) { this->origin = origin; }
    void setIsEquirectangula(bool isEquirectangula) {
        this->isEquirectangula = isEquirectangula;
    }
    void setCameraDir(const glm::vec3 &dir) { this->dir = dir; }
    void setCameraUp(const glm::vec3 &up) { this->up = up; }

    void lookAt(const glm::vec3 &eye, const glm::vec3 &target,
                const glm::vec3 &up);
};
