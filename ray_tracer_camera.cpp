#pragma once
#include "ray_tracer_camera.hpp"

RayTracerCamera::RayTracerCamera(int32_t width, int32_t height, float fov,
                                 float near, float far) {
    this->width = width;
    this->height = height;
    this->fov = fov;
    this->tnear = tnear;
    this->tfar = tfar;
}