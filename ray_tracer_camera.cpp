#pragma once
#include "ray_tracer_camera.hpp"

RayTracerCamera::RayTracerCamera(int32_t width, int32_t height, float fov,
                                 float tnear, float tfar) {
    this->width = width;
    this->height = height;
    this->fov = fov;
    this->tnear = tnear;
    this->tfar = tfar;

    this->origin = glm::vec3(1.5f, 1.5f, -1.5f);
    this->dir = glm::vec3(-0.577350259f, -0.577350259f, 0.577350259f);
    this->up = glm::vec3(0.0f, 1.0f, 0.0f);
}
glm::vec3 RayTracerCamera::getCameraSide() const {
    return glm::normalize(glm::cross(dir, up));
}

glm::vec3 RayTracerCamera::getCameraUp(const glm::vec3 &side) const {
    return glm::normalize(glm::cross(side, dir));
}

glm::vec3 RayTracerCamera::getRayDir(float x, float y) const {
    const auto side = getCameraSide();
    const auto up = getCameraUp(side);
    const auto scale = tanf(glm::radians(this->fov * 0.5f));
    return glm::normalize(scale * x * side + scale * y * up + this->dir);
}

const void RayTracerCamera::lookAt(const glm::vec3 &eye,
                                   const glm::vec3 &target,
                                   const glm::vec3 &up) {
    this->dir = glm::normalize(target - eye);
    this->origin = eye;
    this->up = up;
}