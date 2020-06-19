#include "mouse_camera_controller.hpp"

#include <algorithm>

#include <glm/ext.hpp>

bool controllCameraMouse(GLFWwindow *window, RayTracerCamera &camera,
                         float deltaT, const glm::vec2 &mouseDelta,
                         const glm::vec2 &scrollDelta) {
    const auto rbtn = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);

    const auto side = camera.getCameraSide();
    const auto up = camera.getCameraUp();
    const auto scrollSpeed = 0.5f;
    const auto mouseSpeed = 1.0f;

    auto origin = camera.getCameraOrigin();
    auto length = glm::length(origin) + scrollDelta.y * scrollSpeed;

    length = std::max(length, 0.01f);

    origin = glm::normalize(origin) * length;

    if (rbtn == GLFW_PRESS) {
        camera.setCameraOrigin(
            glm::rotate(glm::rotate(origin, mouseDelta.y * mouseSpeed, side),
                        -mouseDelta.x * mouseSpeed, up));
        camera.setCameraUp(glm::vec3(0.0f, 1.0f, 0.0f));
        camera.setCameraDir(-glm::normalize(camera.getCameraOrigin()));
    } else {
        camera.setCameraOrigin(origin);
    }

    return scrollDelta.y != 0 ||
           ((mouseDelta.x != 0 || mouseDelta.y != 0) && rbtn == GLFW_PRESS);
}
