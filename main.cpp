#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <OpenImageDenoise/oidn.hpp>

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

void detachGeometries(RTCScene scene, ConstantPGeometryList &geometries) {
    for (auto geometry : geometries) {
        geometry->release(scene);
    }
    geometries.clear();
}

ConstantPGeometryList addDefaultMeshToScene(RTCDevice device, RTCScene scene) {
    ConstantPModelList models;

    models.push_back(loadCube(
        PMaterial(new Material(REFLECTION, glm::vec4(0.0, 0.0f, 1.0f, 1.0f),
                               nullptr, nullptr, 0.25f, 0.75f, nullptr,
                               glm::vec3(0.0f), nullptr)),
        glm::translate(glm::vec3(-3.0f, 1.0f, -3.0f))));

    models.push_back(loadSphere(
        PMaterial(new Material(REFLECTION, glm::vec4(1.0, 0.0f, 0.0f, 1.0f),
                               nullptr, nullptr, 0.25f, 1.0, nullptr,
                               glm::vec3(0.0f), nullptr)),
        80, 60, glm::translate(glm::vec3(+3.0f, 1.0f, -3.0f))));

    models.push_back(loadSphere(
        PMaterial(new Material(REFRACTION, glm::vec4(0.0, 1.0f, 0.0f, 1.0f),
                               nullptr, nullptr, 0.0f, 1.0f, nullptr,
                               glm::vec3(0.0f), nullptr)),
        80, 60, glm::translate(glm::vec3(0.0f, 1.0f, +3.0f))));

    ConstantPGeometryList geometries;
    for (auto model : models) {
        for (auto node : model->getScene()->getNodes()) {
            geometries.splice(geometries.cend(), Geometry::generateGeometries(
                                                     device, scene, node));
        }
    }

    return geometries;
}

ConstantPModel loadModel(const char *const path) {
    // add model
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    if (loader.LoadBinaryFromFile(&model, &err, &warn, path)) {
        return loadGltfModel(model);
    } else if (loader.LoadASCIIFromFile(&model, &err, &warn, path)) {
        return loadGltfModel(model);
    }

    return loadObjModel(path);
}

ConstantPGeometryList addModel(RTCDevice device, RTCScene scene,
                               ConstantPModel model) {
    ConstantPGeometryList geometries;
    for (auto node : model->getScene()->getNodes()) {
        geometries.splice(geometries.cend(),
                          Geometry::generateGeometries(device, scene, node));
    }

    return geometries;
}

ConstantPGeometryList addModel(RTCDevice device, RTCScene scene,
                               ConstantPModel model, RayTracer &raytracer,
                               RayTracerCamera &camera) {
    auto geometries = addModel(device, scene, model);

    rtcCommitScene(scene);

    RTCBounds bb;
    rtcGetSceneBounds(scene, &bb);

    const auto eye = 1.1f * glm::vec3(bb.upper_x, bb.upper_y, bb.upper_z);
    const auto target = glm::vec3(0.0f, 0.0f, 0.0f);
    const auto up = glm::vec3(0.0f, 1.0f, 0.0f);
    const auto bbmax = std::max({glm::abs(bb.lower_x), glm::abs(bb.lower_y),
                                 glm::abs(bb.lower_z), glm::abs(bb.upper_x),
                                 glm::abs(bb.upper_y), glm::abs(bb.upper_z)});
    const auto tfar = 10.0f * glm::length(glm::vec3(bbmax));
    camera.setFar(tfar);
    camera.lookAt(eye, target, up);

    return geometries;
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

    auto model = ConstantPModel(nullptr);

    auto geometries = addDefaultMeshToScene(device, scene);

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
        auto model = loadGroundPlane(
            PMaterial(new Material(
                REFLECTION, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), nullptr, nullptr,
                0.2f, 0.8f, nullptr, glm::vec3(0.0f), nullptr)),
            glm::translate(glm::vec3(0.0f, bb.lower_y, 0.0f)) *
                glm::scale(glm::vec3(bbmax * 1.2f)));

        for (auto node : model->getScene()->getNodes()) {
            geometries.splice(geometries.cend(), Geometry::generateGeometries(
                                                     device, scene, node));
        }

        rtcCommitScene(scene);
    }

    camera.setLensRadius(debugGui.getLensRadius());
    camera.setFocusDistance(debugGui.getFocusDistance());

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

        bool needResize = false;
        bool needUpdate = false;
        bool needRestart = false;
        bool needGeometryUpdate = false;

        {
            int32_t w, h;
            glfwGetWindowSize(window, &w, &h);
            if (w != 0 && h != 0) {
                needResize =
                    w != windowSize.x || h != windowSize.y || needResize;
                windowSize = glm::i32vec2(w, h);
                if (needResize) {
                    glViewport(0, 0, w, h);
                }
            }
        }

        glm::dvec2 mousePos = getWindowMousePos(window, windowSize);
        glm::vec2 mouseDelta = mousePos - prevMousePos;

        debugGui.beginFrame(raytracer, model, needUpdate, needResize,
                            needRestart, needGeometryUpdate);

        if (!debugGui.isActive() &&
            debugGui.getRenderingMode() != PATHTRACING) {
            switch (debugGui.getCameraMode()) {
                case ORBIT:
                    needUpdate = controllCameraMouse(window, camera, (float)dt,
                                                     mouseDelta, wheelDelta) ||
                                 needUpdate;
                    break;
                case FPS:
                    needUpdate =
                        controllCameraFPS(window, camera,
                                          static_cast<float>(dt), mouseDelta) ||
                        needUpdate;
                    break;
            }
        }

        needUpdate =
            needUpdate || needResize || needRestart || needGeometryUpdate;

        if (needResize) {
            camera.setIsEquirectangula(debugGui.getIsEquirectangular());

            if (camera.getIsEquirectangula()) {
                camera.setCameraUp(glm::vec3(0.0f, 1.0f, 0.0f));
                camera.setCameraDir(glm::vec3(0.0f, 0.0f, 1.0f));

                if (windowSize.x > 2 * windowSize.y) {
                    windowSize.y = windowSize.x / 2;
                } else {
                    windowSize.x = windowSize.y * 2;
                }

                glfwSetWindowSize(window, windowSize.x, windowSize.y);
            } else {
                camera.lookAt(camera.getCameraOrigin(), glm::vec3(0.0f),
                              glm::vec3(0.0f, 1.0f, 0.0f));
            }

            raytracer.resize(windowSize / debugGui.getBufferScale());
        }

        if (needRestart) {
            auto path = debugGui.getGlbPath();
            if (!path.empty()) {
                detachGeometries(scene, geometries);
                model = loadModel(path.c_str());
                geometries = addModel(device, scene, model, raytracer, camera);
            }
        }

        if (needGeometryUpdate) {
            if (model.get() != nullptr) {
                auto anims = model->getAnimations();

                if (anims.size() > 0) {
                    auto anim = anims[debugGui.getAnimIndex()];
                    Geometry::updateGeometries(device, scene, geometries, anim,
                                               debugGui.getAnimTime(),
                                               glm::mat4(1.0f));
                    rtcCommitScene(scene);
                }
            }
        }

        if (needUpdate) {
            camera.setLensRadius(debugGui.getLensRadius());
            camera.setFocusDistance(debugGui.getFocusDistance());

            raytracer.setEnableSuperSampling(debugGui.getEnableSuperSampling());
            raytracer.setMaxSamples(debugGui.getSamples());
            raytracer.setRenderingMode(debugGui.getRenderingMode());

            if (debugGui.getRenderingMode() != PATHTRACING) {
                debugGui.setIsRendering(true);
            }
        }

        if (debugGui.getIsRendering()) {
            if (raytracer.render(scene, camera)) {
                if (raytracer.getRenderingMode() == PATHTRACING) {
                    raytracer.denoise(denoiser);
                }

                debugGui.setIsRendering(false);
            }

            raytracer.finish(raytracer.getRenderingMode() == PATHTRACING);

            copyPixelsToTexture(raytracer.getImage(), fbo, texture);
        }

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

    detachGeometries(scene, geometries);

    rtcCommitScene(scene);

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);

    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texture);

    glfwTerminate();

    return 0;
}
