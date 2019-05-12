#pragma once

#include <stdint.h>

class RayTracerCamera {
  public:
    RayTracerCamera(int32_t width, int32_t height, float fov = 90.0f,
                    float near = 0.001f, float far = 1000.0f) {}
};