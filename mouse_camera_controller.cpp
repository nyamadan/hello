#include "mouse_camera_controller.hpp"

#include <algorithm>

#include <glm/ext.hpp>

bool controllCameraMouse(GLFWwindow *window, RayTracerCamera &camera,
                         float deltaT, const glm::vec2 &mouseDelta,
                         const glm::vec2 &scrollDelta) {
    const auto rbtn = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);

    const auto side = camera.getCameraSide();
    const auto up = camera.getCameraUp();

    auto origin = camera.getCameraOrigin();
    auto length = glm::length(origin) +
                  glm::sign(scrollDelta.y) * 0.05f * camera.getFar();

    length = glm::max(length, camera.getNear());

    origin = glm::normalize(origin) * length;

    if (rbtn == GLFW_PRESS) {
        camera.setCameraOrigin(glm::rotate(
            glm::rotate(origin, mouseDelta.y, side), -mouseDelta.x, up));
        camera.setCameraUp(glm::vec3(0.0f, 1.0f, 0.0f));
        camera.setCameraDir(-glm::normalize(camera.getCameraOrigin()));
    } else {
        camera.setCameraOrigin(origin);
    }

    return scrollDelta.y != 0 ||
           ((mouseDelta.x != 0 || mouseDelta.y != 0) && rbtn == GLFW_PRESS);
}
