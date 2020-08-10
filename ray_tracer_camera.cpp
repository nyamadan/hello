#pragma once

#include "stb.h"

#include "ray_tracer_camera.hpp"

RayTracerCamera::RayTracerCamera() {
    this->fov = 120.0f;
    this->tnear = 1e-3;
    this->tfar = 1e+3;

    this->origin = glm::vec3(1.5f, 1.5f, -1.5f);
    this->dir = glm::vec3(-0.577350259f, -0.577350259f, 0.577350259f);
    this->up = glm::vec3(0.0f, 1.0f, 0.0f);

    this->focusDistance = 6.0f;
    this->lensRadius = 0.4f;
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
    const auto theta = -2.0f * M_PI * x * height / width;
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
    const auto scale = glm::tan(glm::radians(this->fov * 0.5f));
    return glm::normalize(scale * x * side + scale * y * up + this->dir);
}

void RayTracerCamera::getRayInfo(glm::vec3 &rayDir, glm::vec3 &rayOrigin,
                                 float x, float y,
                                 xorshift128plus_state &randomState) const {
    if (focusDistance <= 0 || lensRadius <= 0) {
        rayDir = getRayDir(x, y);
        rayOrigin = getCameraOrigin();
        return;
    }

    const auto side = getCameraSide();
    const auto up = getCameraUp(side);
    const auto scale = glm::tan(glm::radians(getFov() * 0.5f));
    const auto theta = 2.0f * M_PI * xorshift128plus01f(randomState);
    const auto radius = lensRadius * xorshift128plus01f(randomState);
    const auto target =
        focusDistance * (scale * x * side + scale * y * up + getCameraDir()) +
        getCameraOrigin();

    rayOrigin = radius * (glm::cos(theta) * side + glm::sin(theta) * up) +
                getCameraOrigin();

    rayDir = glm::normalize(target - rayOrigin);
}

void RayTracerCamera::getRayInfoEquirectangular(
    glm::vec3 &rayDir, glm::vec3 &rayOrigin, float x, float y,
    xorshift128plus_state &randomState, int32_t width, int32_t height) const {
    if (focusDistance <= 0 || lensRadius <= 0) {
        rayDir = getRayDirEquirectangular(x, y, width, height);
        rayOrigin = getCameraOrigin();
        return;
    }

    const auto theta = -2.0f * M_PI * x * height / width;
    const auto phi = M_PI * y;

    const auto side = glm::normalize(glm::cross(getCameraDir(), getCameraUp()));
    const auto up = glm::normalize(glm::cross(side, getCameraDir()));

    const auto m = glm::mat3(side, up, getCameraDir());
    const auto dir =
        m * glm::vec3(glm::cos(phi) * glm::cos(theta), glm::sin(phi),
                      glm::cos(phi) * glm::sin(theta));

    {
        const auto side = glm::normalize(glm::cross(dir, getCameraUp()));
        const auto up = glm::normalize(glm::cross(side, getCameraDir()));

        const auto theta = 2.0f * M_PI * xorshift128plus01f(randomState);
        const auto radius = lensRadius * xorshift128plus01f(randomState);
        const auto target = focusDistance * dir + getCameraOrigin();

        rayOrigin = radius * (glm::cos(theta) * side + glm::sin(theta) * up) +
                    getCameraOrigin();
        rayDir = glm::normalize(target - rayOrigin);
    }
}

void RayTracerCamera::lookAt(const glm::vec3 &eye, const glm::vec3 &target,
                             const glm::vec3 &up) {
    this->dir = glm::normalize(target - eye);
    this->origin = eye;
    this->up = up;
}