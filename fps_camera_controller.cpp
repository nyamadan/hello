#include "fps_camera_controller.hpp"

#include <glm/ext.hpp>

bool controllCameraFPS(GLFWwindow *window, RayTracerCamera &camera,
                       float deltaT, const glm::vec2 &mouseDelta) {
    const auto rbtn = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    const auto side = camera.getCameraSide();

    bool needUpdate = false;

    const auto controlSpeed = camera.getFar();
    if (rbtn == GLFW_PRESS) {
        const auto up = glm::vec3(0.0f, 1.0f, 0.0f);
        camera.setCameraUp(up);
        camera.setCameraDir(
            glm::rotate(glm::rotate(camera.getCameraDir(), -mouseDelta.y, side),
                        mouseDelta.x, up));
        needUpdate = true;
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        const auto dir = camera.getCameraDir();
        camera.setCameraOrigin(camera.getCameraOrigin() +
                               dir * controlSpeed * deltaT);
        needUpdate = true;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        const auto dir = camera.getCameraDir();
        camera.setCameraOrigin(camera.getCameraOrigin() -
                               dir * controlSpeed * deltaT);
        needUpdate = true;
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        const auto dir = camera.getCameraDir();
        camera.setCameraOrigin(camera.getCameraOrigin() +
                               side * controlSpeed * deltaT);
        needUpdate = true;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        const auto dir = camera.getCameraDir();
        camera.setCameraOrigin(camera.getCameraOrigin() -
                               side * controlSpeed * deltaT);
        needUpdate = true;
    }

    return needUpdate;
}