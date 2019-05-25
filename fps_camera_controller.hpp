#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "ray_tracer_camera.hpp"

void controllCameraFPS(GLFWwindow *window, RayTracerCamera &camera,
                       float deltaT, const glm::vec2 &mouseDelta);