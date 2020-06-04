#include <GL/gl3w.h>

#include <glm/glm.hpp>

#include <glm/ext.hpp>

#include <stb_image_write.h>

#include "debug_gui.hpp"
#include "fps_camera_controller.hpp"
#include "geometry.hpp"
#include "image_buffer.hpp"
#include "mouse_camera_controller.hpp"
#include "ray_tracer.hpp"

auto wheelDelta = glm::dvec2(0.0, 0.0f);

void WINAPI glfwErrorCallback(int error, const char *description) {
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

void copyPixelsToTexture(const ImageBuffer &image, GLuint fbo, GLuint texture) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.getWidth(), image.getHeight(),
                 0, GL_RGB, GL_UNSIGNED_BYTE, image.GetTextureBuffer());
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

const ConstantPMeshList addDefaultMeshToScene(RTCDevice device,
                                              RTCScene scene) {
    ConstantPMeshList meshs;

    auto plane =
        addGroundPlane(device, scene,
                       PMaterial(new Material(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                                              1.0f, glm::vec3(0.0f))),
                       glm::translate(glm::vec3(0.0f, -1.0f, 0.0f)) *
                           glm::scale(glm::vec3(10.0f)));
    meshs.insert(meshs.cend(), plane);

    auto cube =
        addCube(device, scene,
                PMaterial(new Material(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), 1.0f,
                                       glm::vec3(0.0f))),
                glm::translate(glm::vec3(-3.0f, 0.0f, 0.0f)));
    meshs.insert(meshs.cend(), cube);

    auto sphere =
        addSphere(device, scene,
                  PMaterial(new Material(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
                                         1.0f, glm::vec3(0.0f))),
                  1.0f, 8, 6, glm::translate(glm::vec3(3.0f, 0.0f, 0.0f)));
    meshs.insert(meshs.cend(), sphere);

    auto light =
        addSphere(device, scene,
                  PMaterial(new Material(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                                         1.0f, glm::vec3(1.0f))),
                  1.0f, 8, 6, glm::translate(glm::vec3(0.0f, 1.0f, 0.0f)));
    meshs.insert(meshs.cend(), light);

    return meshs;
}

const ConstantPMeshList addMeshsToScene(RTCDevice device, RTCScene scene,
                                        const char *const path) {
    ConstantPMeshList meshs = addDefaultMeshToScene(device, scene);

    // add model
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    const auto ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);
    if (ret) {
        const auto x = addModel(device, scene, model);
        meshs.insert(meshs.cend(), x.cbegin(), x.cend());
    }

    return meshs;
}

void detachMeshs(RTCScene scene, ConstantPMeshList &meshs) {
    for (auto it = meshs.cbegin(); it != meshs.cend(); it++) {
        const auto pMesh = *it;
        auto geomId = pMesh->getGeometryId();
        rtcDetachGeometry(scene, geomId);
    }

    meshs.clear();
}

void handleDebugOperation(RTCDevice device, RTCScene scene, DebugGUI &debugGui,
                          const ImageBuffer image, ConstantPMeshList &meshs) {
    {
        auto path = debugGui.pullSavingImagePath();
        if (!path.empty()) {
            stbi_flip_vertically_on_write(true);
            stbi_write_png(path.c_str(), image.getWidth(), image.getHeight(),
                           image.getChannels(), image.GetReadonlyBuffer(),
                           image.getChannels() * image.getWidth());
        }
    }

    {
        auto path = debugGui.pullOpeningGLBPath();
        if (!path.empty()) {
            detachMeshs(scene, meshs);

            meshs = addMeshsToScene(device, scene, path.c_str());

            rtcCommitScene(scene);
        }
    }
}

int main(void) {
    auto windowSize = glm::i32vec2(640, 480);

    auto debugGui = DebugGUI();

#ifdef NDEBUG
    auto device = rtcNewDevice(nullptr);
#else
    auto device = rtcNewDevice("verbose=1");
#endif
    auto scene = rtcNewScene(device);

    // const auto buggyGLB =
    // "glTF-Sample-Models/2.0/Buggy/glTF-Binary/Buggy.glb"; auto meshs =
    // addMeshsToScene(device, scene, buggyGLB);

    auto meshs = addDefaultMeshToScene(device, scene);

    auto raytracer = RayTracer(windowSize);

    auto camera = RayTracerCamera(120.0f, 0.001f, 1000.0f);
    const auto eyeDistance = 5.0f;
    const auto eye = glm::vec3(1.0f, 1.0f, 1.0f) * eyeDistance;
    const auto target = glm::vec3(0.0f, 0.0f, 0.0f);
    const auto up = glm::vec3(0.0f, 1.0f, 0.0f);
    camera.lookAt(eye, target, up);

    rtcCommitScene(scene);

    raytracer.render(scene, camera);

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifndef NDEBUG
    glfwSetErrorCallback(glfwErrorCallback);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
#endif

    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

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

    debugGui.setup(window);

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
        bool needUpdate = controllCameraMouse(window, camera, (float)dt,
                                              mouseDelta, wheelDelta);

        {
            int32_t w, h;
            glfwGetWindowSize(window, &w, &h);
            needUpdate = needUpdate || w != windowSize.x || h != windowSize.y;
            windowSize = glm::i32vec2(w, h);
        }

        if (needUpdate) {
            raytracer.resize(windowSize);
            glViewport(0, 0, windowSize.x, windowSize.y);
        }

        debugGui.beginFrame();

        raytracer.render(scene, camera);

        copyPixelsToTexture(raytracer.getImage(), fbo, texture);

        // Rendering
        glfwMakeContextCurrent(window);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glBlitFramebuffer(0, 0, raytracer.getImage().getWidth(),
                          raytracer.getImage().getHeight(), 0, 0, windowSize.x,
                          windowSize.y, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        debugGui.renderFrame();

        glfwSwapBuffers(window);

        handleDebugOperation(device, scene, debugGui, raytracer.getImage(),
                             meshs);

        wheelDelta = glm::dvec2(0.0, 0.0);
        prevMousePos = mousePos;
        glfwPollEvents();

        t0 = t;
    }

    detachMeshs(scene, meshs);
    rtcCommitScene(scene);

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);

    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texture);

    glfwTerminate();

    return 0;
}
