#include <GL/gl3w.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <OpenImageDenoise/oidn.hpp>
#include <libyuv.h>

#include <lua.hpp>

#include <random>

#include "debug_gui.hpp"
#include "fps_camera_controller.hpp"
#include "image_buffer.hpp"
#include "mesh.hpp"
#include "mouse_camera_controller.hpp"
#include "ray_tracer.hpp"

auto wheelDelta = glm::dvec2(0.0, 0.0);

static RayTracer g_rayTracer;
static RayTracerCamera g_camera;
static oidn::DeviceRef g_denoiser;

static RTCScene g_scene;
static RTCDevice g_device;
static ConstantPGeometryList g_geometries;

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

int32_t getYUV420Size(int32_t bufferWidth, int32_t bufferHeight) {
    const int32_t ySize = bufferWidth * bufferHeight;
    const int32_t uSize = ySize / 4;
    const int32_t vSize = uSize;
    return ySize + uSize + vSize;
}

std::shared_ptr<uint8_t[]> createYUV420(const ImageBuffer &image) {
    const auto rgbaBuffer = image.GetTextureBuffer();
    const int32_t bufferWidth = image.getWidth();
    const int32_t bufferHeight = image.getHeight();

    return std::shared_ptr<uint8_t[]>(
        new uint8_t[getYUV420Size(bufferWidth, bufferHeight)]);
}

void encodeYUV420(const uint8_t *rgbaBuffer, uint8_t *yuvBuffer,
                  int32_t bufferWidth, int32_t bufferHeight) {
    const int32_t ySize = bufferWidth * bufferHeight;
    const int32_t uSize = ySize / 4;
    const int32_t vSize = uSize;

    const int32_t yStride = bufferWidth;
    const int32_t uStride = bufferWidth / 2;
    const int32_t vStride = uStride;

    uint8_t *yBuffer = yuvBuffer;
    uint8_t *uBuffer = yBuffer + ySize;
    uint8_t *vBuffer = uBuffer + uSize;

    libyuv::RAWToI420((const uint8_t *)rgbaBuffer, bufferWidth * 3, yBuffer,
                      yStride, uBuffer, uStride, vBuffer, vStride, bufferWidth,
                      -bufferHeight);
}

static void dumpStack(lua_State *L) {
    int i;
    int stackSize = lua_gettop(L);
    for (i = stackSize; i >= 1; i--) {
        int type = lua_type(L, i);
        printf("Stack[%2d-%10s] : ", i, lua_typename(L, type));

        switch (type) {
            case LUA_TNUMBER:
                printf("%f", lua_tonumber(L, i));
                break;
            case LUA_TBOOLEAN:
                if (lua_toboolean(L, i)) {
                    printf("true");
                } else {
                    printf("false");
                }
                break;
            case LUA_TSTRING:
                printf("%s", lua_tostring(L, i));
                break;
            case LUA_TNIL:
                break;
            default:
                printf("%s", lua_typename(L, type));
                break;
        }
        printf("\n");
    }
    printf("\n");
}

static int L_loadPlane(lua_State *L) {
    ConstantPModelList models;

    auto idx = 1;

    auto type = static_cast<MaterialType>(lua_tointeger(L, idx++));

    glm::vec4 baseColorFactor;
    baseColorFactor.x = lua_tonumber(L, idx++);
    baseColorFactor.y = lua_tonumber(L, idx++);
    baseColorFactor.z = lua_tonumber(L, idx++);
    baseColorFactor.w = lua_tonumber(L, idx++);

    auto baseColorTexture = lua_tostring(L, idx++);
    auto normalTexture = lua_tostring(L, idx++);
    auto roughness = lua_tonumber(L, idx++);
    auto metalness = lua_tonumber(L, idx++);
    auto roughnessMetalnessTexture = lua_tostring(L, idx++);

    glm::vec3 emissive;
    emissive.x = lua_tonumber(L, idx++);
    emissive.y = lua_tonumber(L, idx++);
    emissive.z = lua_tonumber(L, idx++);
    auto emissiveTexture = lua_tostring(L, idx++);

    glm::vec3 position;
    position.x = lua_tonumber(L, idx++);
    position.y = lua_tonumber(L, idx++);
    position.z = lua_tonumber(L, idx++);

    glm::vec3 scale;
    scale.x = lua_tonumber(L, idx++);
    scale.y = lua_tonumber(L, idx++);
    scale.z = lua_tonumber(L, idx++);

    auto qx = lua_tonumber(L, idx++);
    auto qy = lua_tonumber(L, idx++);
    auto qz = lua_tonumber(L, idx++);
    auto qw = lua_tonumber(L, idx++);
    auto quat = glm::quat(qw, qx, qy, qz);

    auto model = loadPlane(
        PMaterial(new Material(type, baseColorFactor, nullptr, nullptr,
                               roughness, metalness, nullptr, emissive,
                               nullptr)),
        glm::translate(position) * glm::toMat4(quat) * glm::scale(scale));

    ConstantPGeometryList &geometries = g_geometries;

    for (auto node : model->getScene()->getNodes()) {
        geometries.splice(geometries.cend(), Geometry::generateGeometries(
                                                 g_device, g_scene, node));
    }

    lua_settop(L, 0);

    return 0;
}

static int L_loadCube(lua_State *L) {
    ConstantPModelList models;

    auto idx = 1;

    auto type = static_cast<MaterialType>(lua_tointeger(L, idx++));

    glm::vec4 baseColorFactor;
    baseColorFactor.x = lua_tonumber(L, idx++);
    baseColorFactor.y = lua_tonumber(L, idx++);
    baseColorFactor.z = lua_tonumber(L, idx++);
    baseColorFactor.w = lua_tonumber(L, idx++);

    auto baseColorTexture = lua_tostring(L, idx++);
    auto normalTexture = lua_tostring(L, idx++);
    auto roughness = lua_tonumber(L, idx++);
    auto metalness = lua_tonumber(L, idx++);
    auto roughnessMetalnessTexture = lua_tostring(L, idx++);

    glm::vec3 emissive;
    emissive.x = lua_tonumber(L, idx++);
    emissive.y = lua_tonumber(L, idx++);
    emissive.z = lua_tonumber(L, idx++);
    auto emissiveTexture = lua_tostring(L, idx++);

    glm::vec3 position;
    position.x = lua_tonumber(L, idx++);
    position.y = lua_tonumber(L, idx++);
    position.z = lua_tonumber(L, idx++);

    glm::vec3 scale;
    scale.x = lua_tonumber(L, idx++);
    scale.y = lua_tonumber(L, idx++);
    scale.z = lua_tonumber(L, idx++);

    auto qx = lua_tonumber(L, idx++);
    auto qy = lua_tonumber(L, idx++);
    auto qz = lua_tonumber(L, idx++);
    auto qw = lua_tonumber(L, idx++);
    auto quat = glm::quat(qw, qx, qy, qz);

    auto model = loadCube(
        PMaterial(new Material(type, baseColorFactor, nullptr, nullptr,
                               roughness, metalness, nullptr, emissive,
                               nullptr)),
        glm::translate(position) * glm::toMat4(quat) * glm::scale(scale));

    ConstantPGeometryList &geometries = g_geometries;

    for (auto node : model->getScene()->getNodes()) {
        geometries.splice(geometries.cend(), Geometry::generateGeometries(
                                                 g_device, g_scene, node));
    }

    lua_settop(L, 0);

    return 0;
}

static int L_loadSphere(lua_State *L) {
    ConstantPModelList models;

    auto idx = 1;

    auto type = static_cast<MaterialType>(lua_tointeger(L, idx++));

    glm::vec4 baseColorFactor;
    baseColorFactor.x = lua_tonumber(L, idx++);
    baseColorFactor.y = lua_tonumber(L, idx++);
    baseColorFactor.z = lua_tonumber(L, idx++);
    baseColorFactor.w = lua_tonumber(L, idx++);

    auto baseColorTexture = lua_tostring(L, idx++);
    auto normalTexture = lua_tostring(L, idx++);
    auto roughness = lua_tonumber(L, idx++);
    auto metalness = lua_tonumber(L, idx++);
    auto roughnessMetalnessTexture = lua_tostring(L, idx++);

    glm::vec3 emissive;
    emissive.x = lua_tonumber(L, idx++);
    emissive.y = lua_tonumber(L, idx++);
    emissive.z = lua_tonumber(L, idx++);
    auto emissiveTexture = lua_tostring(L, idx++);

    auto segU = lua_tointeger(L, idx++);
    auto segV = lua_tointeger(L, idx++);

    glm::vec3 position;
    position.x = lua_tonumber(L, idx++);
    position.y = lua_tonumber(L, idx++);
    position.z = lua_tonumber(L, idx++);

    glm::vec3 scale;
    scale.x = lua_tonumber(L, idx++);
    scale.y = lua_tonumber(L, idx++);
    scale.z = lua_tonumber(L, idx++);

    auto qx = lua_tonumber(L, idx++);
    auto qy = lua_tonumber(L, idx++);
    auto qz = lua_tonumber(L, idx++);
    auto qw = lua_tonumber(L, idx++);
    auto quat = glm::quat(qw, qx, qy, qz);

    auto model = loadSphere(
        PMaterial(new Material(type, baseColorFactor, nullptr, nullptr,
                               roughness, metalness, nullptr, emissive,
                               nullptr)),
        segU, segV,
        glm::translate(position) * glm::toMat4(quat) * glm::scale(scale));

    ConstantPGeometryList &geometries = g_geometries;

    for (auto node : model->getScene()->getNodes()) {
        geometries.splice(geometries.cend(), Geometry::generateGeometries(
                                                 g_device, g_scene, node));
    }

    lua_settop(L, 0);

    return 0;
}

static int L_setRenderMode(lua_State *L) {
    auto mode = static_cast<RenderingMode>(lua_tointeger(L, 1));
    g_rayTracer.setRenderingMode(mode);
    lua_settop(L, 0);
    return 0;
}

static int L_setCameraDir(lua_State *L) {
    auto dir = glm::normalize(
        glm::vec3(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3)));
    g_camera.setCameraOrigin(dir);
    lua_settop(L, 0);
    return 0;
}

static int L_setCameraOrigin(lua_State *L) {
    auto pos =
        glm::vec3(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3));
    g_camera.setCameraOrigin(pos);
    lua_settop(L, 0);
    return 0;
}

static int L_setMaxSamples(lua_State *L) {
    auto samples = static_cast<RenderingMode>(lua_tointeger(L, 1));
    g_rayTracer.setMaxSamples(samples);
    lua_settop(L, 0);
    return 0;
}

static int L_reset(lua_State *L) {
    g_rayTracer.reset();
    lua_settop(L, 0);
    return 0;
}

static int L_commitScene(lua_State *L) {
    rtcCommitScene(g_scene);
    lua_settop(L, 0);
    return 0;
}

static int L_denoise(lua_State *L) {
    g_rayTracer.denoise(g_denoiser);
    lua_settop(L, 0);
    return 0;
}

static int L_render(lua_State *L) {
    lua_settop(L, 0);

    lua_pushboolean(L, g_rayTracer.render(g_scene, g_camera) ? 1 : 0);
    return 1;
}

static int L_finish(lua_State *L) {
    auto b = false;
    if (lua_gettop(L) >= 1) {
        auto type = lua_type(L, 1);
        if (type == LUA_TBOOLEAN) {
            b = !!lua_toboolean(L, 1);
        }
    }

    g_rayTracer.finish(b);

    lua_settop(L, 0);

    return 0;
}

int main(void) {
    auto windowSize = glm::i32vec2(640, 480);

    RTCDevice &device = g_device;
    RTCScene &scene = g_scene;
    ConstantPGeometryList &geometries = g_geometries;
    RayTracer &raytracer = g_rayTracer;
    RayTracerCamera &camera = g_camera;

    // Create an Intel Open Image Denoise device
    oidn::DeviceRef &denoiser = g_denoiser;
    denoiser = oidn::newDevice();
    denoiser.commit();

#ifdef NDEBUG
    device = rtcNewDevice(nullptr);
#else
    device = rtcNewDevice("verbose=1");
#endif
    scene = rtcNewScene(device);

    auto model = ConstantPModel(nullptr);

    auto debugGui = DebugGUI();

    raytracer.resize(windowSize / debugGui.getBufferScale());
    raytracer.loadSkybox("./assets/small_rural_road_2k.hdr");

    auto yuvBuffer = createYUV420(raytracer.getImage());

    FILE *fY4m = nullptr;

    lua_State *L = nullptr;

    geometries = addDefaultMeshToScene(device, scene);

    rtcCommitScene(scene);

    RTCBounds bb;
    rtcGetSceneBounds(scene, &bb);

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
        auto model = loadPlane(
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

        if (L == NULL) {
            bool needResize = false;
            bool needUpdate = false;
            bool needLoadModel = false;
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

            auto debugCommands = debugGui.beginFrame(raytracer, model);

            needUpdate = needUpdate ||
                         std::find(debugCommands.cbegin(), debugCommands.cend(),
                                   DebugUpdate) != debugCommands.cend();
            needResize = needResize ||
                         std::find(debugCommands.cbegin(), debugCommands.cend(),
                                   DebugResize) != debugCommands.cend();
            needLoadModel =
                needLoadModel ||
                std::find(debugCommands.cbegin(), debugCommands.cend(),
                          DebugLoadModel) != debugCommands.cend();
            needGeometryUpdate =
                needGeometryUpdate ||
                std::find(debugCommands.cbegin(), debugCommands.cend(),
                          DebugGeometryUpdate) != debugCommands.cend();

            if (!debugGui.isActive() &&
                debugGui.getRenderingMode() != PATHTRACING) {
                switch (debugGui.getCameraMode()) {
                    case ORBIT:
                        needUpdate =
                            controllCameraMouse(window, camera, (float)dt,
                                                mouseDelta, wheelDelta) ||
                            needUpdate;
                        break;
                    case FPS:
                        needUpdate = controllCameraFPS(window, camera,
                                                       static_cast<float>(dt),
                                                       mouseDelta) ||
                                     needUpdate;
                        break;
                }
            }

            needUpdate =
                needUpdate || needResize || needLoadModel || needGeometryUpdate;

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
                yuvBuffer = createYUV420(raytracer.getImage());

                if (fY4m != nullptr) {
                    fclose(fY4m);
                    fY4m = nullptr;
                }
            }

            if (needLoadModel) {
                auto path = debugGui.getGlbPath();
                if (!path.empty()) {
                    detachGeometries(scene, geometries);
                    model = loadModel(path.c_str());
                    geometries =
                        addModel(device, scene, model, raytracer, camera);
                }
            }

            if (needGeometryUpdate) {
                if (model.get() != nullptr) {
                    auto anims = model->getAnimations();

                    if (anims.size() > 0) {
                        auto anim = anims[debugGui.getAnimIndex()];
                        Geometry::updateGeometries(device, scene, geometries,
                                                   anim, debugGui.getAnimTime(),
                                                   glm::mat4(1.0f));
                        rtcCommitScene(scene);
                    }
                }
            }

            if (needUpdate) {
                camera.setLensRadius(debugGui.getLensRadius());
                camera.setFocusDistance(debugGui.getFocusDistance());

                raytracer.setEnableSuperSampling(
                    debugGui.getEnableSuperSampling());
                raytracer.setMaxSamples(debugGui.getSamples());
                raytracer.setRenderingMode(debugGui.getRenderingMode());
                raytracer.reset();

                if (debugGui.getRenderingMode() != PATHTRACING) {
                    debugGui.setIsRendering(true);
                }
            }

            if (std::find(debugCommands.cbegin(), debugCommands.cend(),
                          DebugSaveMovie) != debugCommands.cend()) {
                const auto &path = debugGui.getY4mPath();

                const auto &image = raytracer.getImage();
                const auto width = image.getWidth();
                const auto height = image.getHeight();

                fY4m = fopen(path.c_str(), "wb");
                if (fY4m != nullptr) {
                    fprintf(fY4m,
                            "YUV4MPEG2 W%d H%d F30000:1001 Ip A0:0 C420 "
                            "XYSCSS=420\n",
                            width, height);
                }
            }

            if (std::find(debugCommands.cbegin(), debugCommands.cend(),
                          DebugCancelSaveMovie) != debugCommands.cend()) {
                if (fY4m != nullptr) {
                    fclose(fY4m);
                    fY4m = nullptr;
                }
            }

            if (std::find(debugCommands.cbegin(), debugCommands.cend(),
                          DebugOpenLua) != debugCommands.cend()) {
                if (fY4m != nullptr) {
                    fclose(fY4m);
                    fY4m = nullptr;
                }
                detachGeometries(scene, geometries);
                debugGui.setIsRendering(false);
                model = nullptr;
                rtcCommitScene(scene);

                raytracer.reset();

                L = luaL_newstate();

                luaL_openlibs(L);

                lua_register(L, "_render", L_render);
                lua_register(L, "_finish", L_finish);
                lua_register(L, "_denoise", L_denoise);
                lua_register(L, "_reset", L_reset);
                lua_register(L, "_commitScene", L_commitScene);

                lua_register(L, "_setCameraDir", L_setCameraDir);
                lua_register(L, "_setCameraOrigin", L_setCameraOrigin);

                lua_register(L, "_setMaxSamples", L_setMaxSamples);

                lua_register(L, "_setRenderMode", L_setRenderMode);

                lua_register(L, "_loadSphere", L_loadSphere);
                lua_register(L, "_loadCube", L_loadCube);
                lua_register(L, "_loadPlane", L_loadPlane);

                luaL_loadfile(L, debugGui.getLuaPath().c_str());
            }

            if (debugGui.getIsRendering()) {
                bool finished = raytracer.render(scene, camera);
                bool nextFrame = false;

                if (finished) {
                    if (raytracer.getRenderingMode() == PATHTRACING) {
                        raytracer.denoise(denoiser);
                    }

                    if (debugGui.getIsMovie()) {
                        nextFrame = debugGui.nextFrame(
                            model->getAnimations()[debugGui.getAnimIndex()]);
                        if (!nextFrame) {
                            debugGui.setIsRendering(false);
                            if (fY4m != nullptr) {
                                fclose(fY4m);
                                fY4m = nullptr;
                            }
                        }
                    } else {
                        debugGui.setIsRendering(false);
                    }
                }

                raytracer.finish(raytracer.getRenderingMode() == PATHTRACING);

                copyPixelsToTexture(raytracer.getImage(), fbo, texture);

                if (finished) {
                    if (fY4m != nullptr) {
                        const auto width = raytracer.getImage().getWidth();
                        const auto height = raytracer.getImage().getHeight();
                        encodeYUV420(
                            reinterpret_cast<const uint8_t *>(
                                raytracer.getImage().GetTextureBuffer()),
                            yuvBuffer.get(), width, height);
                        fputs("FRAME\n", fY4m);
                        fwrite(yuvBuffer.get(), getYUV420Size(width, height), 1,
                               fY4m);
                    }

                    if (nextFrame) {
                        Geometry::updateGeometries(
                            device, scene, geometries,
                            model->getAnimations()[debugGui.getAnimIndex()],
                            debugGui.getAnimTime(), glm::mat4(1.0f));
                        rtcCommitScene(scene);
                        raytracer.reset();
                    }
                }
            }

            wheelDelta = glm::dvec2(0.0, 0.0);
            prevMousePos = mousePos;
        } else {
            int res;
            int status;
            status = lua_resume(L, nullptr, 0, &res);

            switch (status) {
                case LUA_OK: {
                    lua_close(L);
                    L = nullptr;
                } break;

                case LUA_YIELD: {
                } break;

                default: {
                    fprintf(stderr, "%s\n", lua_tostring(L, -1));
                    lua_close(L);
                    L = nullptr;
                } break;
            }

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

        if (L == nullptr) {
            debugGui.renderFrame();
        }

        glfwSwapBuffers(window);

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
