#include "fps_camera_controller.hpp"

#include <glm/ext.hpp>

void controllCameraFPS(GLFWwindow *window, RayTracerCamera &camera,
                       float deltaT, const glm::vec2 &mouseDelta) {
    const auto lbtn = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    const auto side = camera.getCameraSide();

    const auto controlSpeed = 1.0f;
    if (lbtn == GLFW_PRESS) {
        const auto up = glm::vec3(0.0f, 1.0f, 0.0f);
        camera.setCameraUp(up);
        camera.setCameraDir(
            glm::rotate(glm::rotate(camera.getCameraDir(), -mouseDelta.y, side),
                        mouseDelta.x, up));
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        const auto dir = camera.getCameraDir();
        camera.setCameraOrigin(camera.getCameraOrigin() +
                               dir * controlSpeed * deltaT);
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        const auto dir = camera.getCameraDir();
        camera.setCameraOrigin(camera.getCameraOrigin() -
                               dir * controlSpeed * deltaT);
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        const auto dir = camera.getCameraDir();
        camera.setCameraOrigin(camera.getCameraOrigin() +
                               side * controlSpeed * deltaT);
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        const auto dir = camera.getCameraDir();
        camera.setCameraOrigin(camera.getCameraOrigin() -
                               side * controlSpeed * deltaT);
    }
}