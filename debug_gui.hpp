#pragma once

#include <array>
#include <memory>
#include <list>

#include <GLFW/glfw3.h>

#include "image_buffer.hpp"
#include "ray_tracer.hpp"
#include "mesh.hpp"

enum CameraMode { ORBIT = 0, FPS };

static const char *const CameraModeNames[] = {"Orbit", "FPS"};

enum DebugGuiCommand {
    DebugUpdate,
    DebugResize,
    DebugLoadModel,
    DebugGeometryUpdate,
    DebugSaveMovie,
    DebugCancelSaveMovie,
    DebugOpenLua
};

class DebugGUI {
  public:
    DebugGUI();

    void setup(GLFWwindow *window);
    std::list<DebugGuiCommand> beginFrame(const RayTracer &raytracer,
                                          const RayTracerCamera &camera,
                                          ConstantPModel model);
    bool nextFrame(ConstantPAnimation anim);
    void renderFrame() const;
    RenderingMode getRenderingMode() const;
    bool getEnableSuperSampling() const;

    CameraMode getCameraMode() const;
    const std::string &getGlbPath() const;
    const std::string &getY4mPath() const;
    const std::string &getLuaPath() const;
    int32_t getBufferScale() const;
    int32_t getSamples() const;
    void setIsRendering(bool x);
    bool getIsRendering() const;
    bool getIsEquirectangular() const;
    float getFocusDistance() const;
    float getLensRadius() const;
    int32_t getAnimIndex() const;
    float getAnimTime() const;
    bool getIsMovie() const;
    bool isActive() const;

  private:
    static const int32_t DeltaTimesBufferSize = 90;

    static bool openFileDialog(std::string &path, const char *const filter);
    static bool saveFileDialog(std::string &path, const char *const filter,
                               const char *const defExt);
    std::array<float, DeltaTimesBufferSize> deltaTimes{};
    int32_t deltaTimesOffset = 0;

    RenderingMode renderingMode = ALBEDO;
    CameraMode cameraMode = ORBIT;
    bool isEquirectangular = false;
    bool enableSuperSampling = true;
    int32_t bufferScale = 4;
    int32_t samples = 200;
    bool isRendering = true;
    bool isMovie = false;
    float lensRadius = 0.0f;
    float focusDistance = 0.0f;
    int32_t currAnimIndex = 0;
    float animTime = 0.0f;
    int32_t currFrame = 0;

    std::string glbPath = "";
    std::string y4mPath = "";
    std::string luaPath = "";
};
