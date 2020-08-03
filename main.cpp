#include <GL/gl3w.h>

#include <glm/glm.hpp>

#include <glm/ext.hpp>

#include <OpenImageDenoise/oidn.hpp>

#include <chrono>
#include <random>

#include "debug_gui.hpp"
#include "fps_camera_controller.hpp"
#include "image_buffer.hpp"
#include "mesh.hpp"
#include "mouse_camera_controller.hpp"
#include "ray_tracer.hpp"

auto wheelDelta = glm::dvec2(0.0, 0.0);

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

    if (true) {
        meshs.push_back(addCube(
            device, scene,
            PMaterial(new Material(REFLECTION, glm::vec4(0.0, 0.0f, 1.0f, 1.0f),
                                   nullptr, nullptr, 0.25f, 0.75f, nullptr,
                                   glm::vec3(0.0f), nullptr)),
            glm::translate(glm::vec3(-3.0f, 1.0f, -3.0f))));

        meshs.push_back(addSphere(
            device, scene,
            PMaterial(new Material(REFLECTION, glm::vec4(1.0, 0.0f, 0.0f, 1.0f),
                                   nullptr, nullptr, 0.25f, 1.0, nullptr,
                                   glm::vec3(0.0f), nullptr)),
            80, 60, glm::translate(glm::vec3(+3.0f, 1.0f, -3.0f))));

        meshs.push_back(addSphere(
            device, scene,
            PMaterial(new Material(REFRACTION, glm::vec4(0.0, 1.0f, 0.0f, 1.0f),
                                   nullptr, nullptr, 0.0f, 1.0f, nullptr,
                                   glm::vec3(0.0f), nullptr)),
            80, 60, glm::translate(glm::vec3(0.0f, 1.0f, +3.0f))));
    }

    if(false) {
        std::random_device rng;
        xorshift128plus_state seed;

        seed.a = rng();
        seed.b = ~seed.a;

        for(auto i = 0; i < 100; i++) {
            auto radius = 5.0f * xorshift128plus01f(seed) + 1.0f;
            auto r = xorshift128plus01f(seed);
            auto g = xorshift128plus01f(seed);
            auto b = xorshift128plus01f(seed);
            auto metalness = xorshift128plus01f(seed);
            auto roughness = xorshift128plus01f(seed);
            auto x = 100.0f * (xorshift128plus01f(seed) - 0.5f);
            auto y = 100.0f * xorshift128plus01f(seed) + radius;
            auto z = 100.0f * (xorshift128plus01f(seed) - 0.5f);
            auto material = xorshift128plus01f(seed) > 0.5f ? REFLECTION : REFRACTION;
            meshs.push_back(addSphere(
                device, scene,
                PMaterial(new Material(material, glm::vec4(r, g, b, 1.0f),
                                    nullptr, nullptr, roughness, metalness, nullptr,
                                    glm::vec3(0.0f), nullptr)),
                80, 60, glm::translate(glm::vec3(x, y, z)) * glm::scale(glm::vec3(radius))));
        }
    }
    return meshs;
}

const ConstantPMeshList addMeshsToScene(RTCDevice device, RTCScene scene,
                                        const char *const path) {
    auto meshs = ConstantPMeshList();

    // add model
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    if (loader.LoadBinaryFromFile(&model, &err, &warn, path)) {
        const auto x = addGlbModel(device, scene, model);
        meshs.insert(meshs.cend(), x.cbegin(), x.cend());
    } else {
        std::string warn;
        std::string err;

        const auto x = addObjModel(device, scene, path);
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

void loadGlbModel(RTCDevice device, RTCScene scene, DebugGUI &debugGui,
                  RayTracer &raytracer, RayTracerCamera &camera,
                  ConstantPMeshList &meshs) {
    auto path = debugGui.getGlbPath();
    if (path.empty()) {
        return;
    }
    detachMeshs(scene, meshs);

    meshs = addMeshsToScene(device, scene, path.c_str());

    rtcCommitScene(scene);

    RTCBounds bb;
    rtcGetSceneBounds(scene, &bb);

    const auto eye = glm::vec3(bb.upper_x, bb.upper_y, bb.upper_z);
    const auto target = glm::vec3(0.0f, 0.0f, 0.0f);
    const auto up = glm::vec3(0.0f, 1.0f, 0.0f);
    const auto bbmax = std::max({glm::abs(bb.lower_x), glm::abs(bb.lower_y),
                                 glm::abs(bb.lower_z), glm::abs(bb.upper_x),
                                 glm::abs(bb.upper_y), glm::abs(bb.upper_z)});
    const auto tfar = 4.0f * glm::length(glm::vec3(bbmax));
    camera.setFar(tfar);
    camera.lookAt(eye, target, up);

    // ground
    meshs.push_back(addGroundPlane(
        device, scene,
        PMaterial(new Material(REFLECTION, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                               nullptr, nullptr, 0.5f, 0.5f, nullptr,
                               glm::vec3(0.0f), nullptr)),
        glm::translate(glm::vec3(0.0f, bb.lower_y, 0.0f)) *
            glm::scale(glm::vec3(bbmax * 1.2f))));

    rtcCommitScene(scene);
}

int main(void) {
    auto windowSize = glm::i32vec2(640, 480);

    // Create an Intel Open Image Denoise device
    oidn::DeviceRef denoiser = oidn::newDevice();
    denoiser.commit();

#ifdef NDEBUG
    auto device = rtcNewDevice(nullptr);
#else
    auto device = rtcNewDevice("verbose=1");
#endif
    auto scene = rtcNewScene(device);

    auto meshs = addDefaultMeshToScene(device, scene);

    auto debugGui = DebugGUI();

    auto raytracer = RayTracer(windowSize / debugGui.getBufferScale());

    raytracer.loadSkybox("./assets/small_rural_road_2k.hdr");

    rtcCommitScene(scene);

    RTCBounds bb;
    rtcGetSceneBounds(scene, &bb);

    auto camera = RayTracerCamera();
    {
        RTCBounds bb;
        rtcGetSceneBounds(scene, &bb);

        const auto eye = glm::vec3(bb.upper_x, bb.upper_y, bb.upper_z);
        const auto target = glm::vec3(0.0f, 0.0f, 0.0f);
        const auto up = glm::vec3(0.0f, 1.0f, 0.0f);
        const auto bbmax = std::max(
            {glm::abs(bb.lower_x), glm::abs(bb.lower_y), glm::abs(bb.lower_z),
             glm::abs(bb.upper_x), glm::abs(bb.upper_y), glm::abs(bb.upper_z)});
        const auto tfar = 4.0f * glm::length(glm::vec3(bbmax));
        camera.setFar(tfar);
        camera.lookAt(eye, target, up);

        // ground
        meshs.push_back(addGroundPlane(
            device, scene,
            PMaterial(new Material(
                REFLECTION, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), nullptr, nullptr,
                0.2f, 0.8f, nullptr, glm::vec3(0.0f), nullptr)),
            glm::translate(glm::vec3(0.0f, bb.lower_y, 0.0f)) *
                glm::scale(glm::vec3(bbmax * 1.2f))));

        rtcCommitScene(scene);
    }

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

    raytracer.setRenderingMode(debugGui.getRenderingMode());

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

        bool needResize = false;
        bool needUpdate = false;
        bool needRestart = false;

        {
            int32_t w, h;
            glfwGetWindowSize(window, &w, &h);
            if(w != 0 && h != 0) {
                needResize = (w != windowSize.x || h != windowSize.y) || needResize;
                windowSize = glm::i32vec2(w, h);
            }
        }

        switch (debugGui.getCameraMode()) {
            case ORBIT:
                needUpdate = controllCameraMouse(window, camera, (float)dt,
                                                 mouseDelta, wheelDelta) ||
                             needUpdate;
                break;
            case FPS:
                needUpdate =
                    controllCameraFPS(window, camera, static_cast<float>(dt), mouseDelta) ||
                    needUpdate;
                break;
        }

        debugGui.beginFrame(raytracer, needUpdate, needResize, needRestart);

        needUpdate = needUpdate || needResize || needRestart;

        if (needResize) {
            camera.setIsEquirectangula(debugGui.getIsEquirectangular());

            if(camera.getIsEquirectangula()) {
                camera.setCameraUp(glm::vec3(0.0f, 1.0f, 0.0f));
                camera.setCameraDir(glm::vec3(0.0f, 0.0f, 1.0f));

                if(windowSize.x > 2 * windowSize.y) {
                    windowSize.y = windowSize.x / 2;
                } else {
                    windowSize.x = windowSize.y * 2;
                }

                glfwSetWindowSize(window, windowSize.x, windowSize.y);
            } else {
                camera.lookAt(camera.getCameraOrigin(), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            }

            raytracer.resize(windowSize / debugGui.getBufferScale());

            glViewport(0, 0, windowSize.x, windowSize.y);
        }

        if (needRestart) {
            loadGlbModel(device, scene, debugGui, raytracer, camera, meshs);
        }

        if (needUpdate) {
            raytracer.setEnableSuperSampling(debugGui.getEnableSuperSampling());
            raytracer.setMaxSamples(debugGui.getSamples());
            raytracer.setRenderingMode(debugGui.getRenderingMode());
        }

        if (debugGui.getIsRendering()) {
            raytracer.render(scene, camera, denoiser);
        }

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
