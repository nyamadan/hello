#include <stb_image_write.h>
#include <tbb/parallel_for.h>

#include <GL/gl3w.h>

#include <glm/ext.hpp>

#include "fps_camera_controller.hpp"
#include "geometry.hpp"
#include "mouse_camera_controller.hpp"
#include "ray_tracer.hpp"

auto wheelDelta = glm::dvec2(0.0, 0.0f);

void glfwErrorCallback(int error, const char *description) {
    fprintf(stderr, "error %d: %s\n", error, description);
}

void glfwScrollCallback(GLFWwindow *window, double xoffset, double yoffset) {
    wheelDelta.x += xoffset;
    wheelDelta.y += yoffset;
}

void glDebugOutput(GLenum source, GLenum type, GLuint eid, GLenum severity,
                   GLsizei length, const GLchar *message,
                   const void *user_param) {
    switch (severity) {
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            break;
        case GL_DEBUG_SEVERITY_LOW:
        case GL_DEBUG_SEVERITY_MEDIUM:
        case GL_DEBUG_SEVERITY_HIGH:
            fprintf(stderr, "ERROR(%X): %s\n", eid, message);
            break;
    }
}

void EnableOpenGLDebugExtention() {
    GLint flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0,
                              nullptr, GL_TRUE);
    }
}

void copyPixelsToTexture(const glm::u8vec3 pixels[], GLuint fbo, GLuint texture,
                         int32_t width, int32_t height) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::dvec2 getWindowMousePos(GLFWwindow *window, const glm::u32vec2 &size) {
    double x, y;
    double aspect = size.x / size.y;
    glfwGetCursorPos(window, &x, &y);
    return glm::dvec2(aspect * (x / size.x - 0.5f), 1.0f - y / size.y);
}

void addGeometryToScene(RTCDevice device, RTCScene scene) {
     auto plane = addGroundPlane(device, scene,
                                glm::translate(glm::vec3(0.0f, -2.0f, 0.0f)) *
                                    glm::scale(glm::vec3(10.0f)));
    // auto cube =
    //    addCube(device, scene, glm::translate(glm::vec3(-3.0f, 0.0f, 0.0f)));
    // auto sphere = addSphere(device, scene, 1.0f, 8, 6,
    //                        glm::translate(glm::vec3(3.0f, 0.0f, 0.0f)));

    // add model
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    const auto ret =
        loader.LoadBinaryFromFile(&model, &err, &warn, "Box.glb");
    if (ret) {
        const auto box = addModel(device, scene, model);
    }
}

int main(void) {
    auto device = rtcNewDevice("verbose=1");
    auto scene = rtcNewScene(device);

    addGeometryToScene(device, scene);

    auto raytracer = RayTracer();
    const auto width = 640u;
    const auto height = 480u;

    auto camera = RayTracerCamera(width, height, 120.0f, 0.001f, 1000.0f);
    const auto eye = glm::vec3(1.0f, 1.0f, -1.0f);
    const auto target = glm::vec3(0.0f, 0.0f, 0.0f);
    const auto up = glm::vec3(0.0f, 1.0f, 0.0f);
    camera.lookAt(eye, target, up);

    rtcCommitScene(scene);

    auto pixels = std::make_unique<glm::u8vec3[]>(width * height);

    raytracer.render(scene, camera, pixels.get());

    stbi_flip_vertically_on_write(true);
    stbi_write_png("hello_embree.png", width, height, 3, pixels.get(),
                   3 * width);

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifndef NDEBUG
    glfwSetErrorCallback(glfwErrorCallback);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
#endif

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    const auto windowSize = glm::u32vec2(width, height);
    const auto window = glfwCreateWindow(windowSize.x, windowSize.y,
                                         "Hello Embree", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glfwSetScrollCallback(window, glfwScrollCallback);

    // Initialize OpenGL loader
    if (gl3wInit() != 0) {
        return 1;
    }

#ifndef NDEBUG
    EnableOpenGLDebugExtention();
#endif

    GLuint texture = 0;
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &texture);
    glViewport(0, 0, windowSize.x, windowSize.y);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

    glm::dvec2 prevMousePos = getWindowMousePos(window, windowSize);

    auto t0 = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        auto t = glfwGetTime();
        auto dt = t - t0;
        glm::dvec2 mousePos = getWindowMousePos(window, windowSize);
        glm::vec2 mouseDelta = mousePos - prevMousePos;

        // controllCameraFPS(window, camera, dt, mouseDelta);
        controllCameraMouse(window, camera, (float)dt, mouseDelta, wheelDelta);
        raytracer.render(scene, camera, pixels.get());

        copyPixelsToTexture(pixels.get(), fbo, texture, width, height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glBlitFramebuffer(0, 0, width, height, 0, 0, windowSize.x, windowSize.y,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        wheelDelta = glm::dvec2(0.0, 0.0);
        prevMousePos = mousePos;

        glfwSwapBuffers(window);
        glfwPollEvents();

        t0 = t;
    }

    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texture);

    glfwTerminate();

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
    return 0;
}