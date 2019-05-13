#pragma once

#include <stdint.h>

class RayTracerCamera {
  private:
    int32_t width;
    int32_t height;
    float fov;
    float tnear;
    float tfar;

  public:
    RayTracerCamera(int32_t width, int32_t height, float fov = 120.0f,
                    float near = 0.001f, float far = 1000.0f);

  public:
    int32_t getWidth() { return width; }
    int32_t getHeight() { return height; }
    float getFov() { return fov; }
    float getNear() { return tnear; }
    float getFar() { return tfar; }
    float getAspect() { return static_cast<float>(width) / height; }
};
