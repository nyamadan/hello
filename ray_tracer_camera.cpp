#pragma once

#include "stb.h"

#include "ray_tracer_camera.hpp"

RayTracerCamera::RayTracerCamera(float fov, float tnear, float tfar) {
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

glm::vec3 RayTracerCamera::getRayDirEquirectangular(float x, float y,
                                                    int32_t width,
                                                    int32_t height) const {
    const auto theta = 2.0f * M_PI * x * height / width;
    const auto phi = M_PI * y;

    const auto &dir = getCameraDir();
    const auto side = glm::normalize(glm::cross(dir, getCameraUp()));
    const auto m = glm::mat3(side, glm::normalize(glm::cross(side, dir)), dir);

    return m * glm::vec3(glm::cos(phi) * glm::cos(theta), glm::sin(phi),
                      glm::cos(phi) * glm::sin(theta));
}

glm::vec3 RayTracerCamera::getRayDir(float x, float y) const {
    const auto side = getCameraSide();
    const auto up = getCameraUp(side);
    const auto scale = tanf(glm::radians(this->fov * 0.5f));
    return glm::normalize(scale * x * side + scale * y * up + this->dir);
}

void RayTracerCamera::lookAt(const glm::vec3 &eye,
                                   const glm::vec3 &target,
                                   const glm::vec3 &up) {
    this->dir = glm::normalize(target - eye);
    this->origin = eye;
    this->up = up;
}