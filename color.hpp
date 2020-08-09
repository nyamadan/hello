#pragma once

#include <glm/glm.hpp>

inline glm::vec3 toneMapping(const glm::vec3 &x) noexcept {
    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    constexpr auto a = 2.51f;
    constexpr auto b = 0.03f;
    constexpr auto c = 2.43f;
    constexpr auto d = 0.59f;
    constexpr auto e = 0.14f;
    return (x * (a * x + b)) / (x * (c * x + d) + e);
}

inline glm::vec3 linearToGamma(const glm::vec3 &src) noexcept {
    return glm::vec3(glm::pow(src.x, 0.5f), glm::pow(src.y, 0.5f),
                     glm::pow(src.z, 0.5f));
}
