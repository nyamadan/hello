#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "fps_camera_controller.hpp"

bool controllCameraMouse(GLFWwindow *window, RayTracerCamera &camera,
                         float deltaT, const glm::vec2 &mouseDelta,
                         const glm::vec2 &scrollDelta);
