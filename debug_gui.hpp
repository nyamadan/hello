#pragma once

#include <array>
#include <memory>

#include <GLFW/glfw3.h>

#include "image_buffer.hpp"
#include "ray_tracer.hpp"

class DebugGUI {
  public:
    DebugGUI();

    void setup(GLFWwindow *window);
    void beginFrame(const RayTracer &raytracer, bool &needUpdate,
                    bool &needResize, bool &needRestart);
    void renderFrame() const;
    RenderingMode getRenderingMode() const;
    bool getEnableSuperSampling() const;

    std::string getGlbPath() const;
    int32_t getBufferScale() const;
    int32_t getSamples() const;
    bool getIsRendering() const;

  private:
    static const int32_t DeltaTimesBufferSize = 90;

    static bool openFileDialog(std::string &path, const char *const filter);
    static bool saveFileDialog(std::string &path, const char *const filter,
                               const char *const defExt);
    std::array<float, DeltaTimesBufferSize> deltaTimes{};
    int32_t deltaTimesOffset = 0;

    RenderingMode renderingMode = ALBEDO;
    bool enableSuperSampling = true;
    int32_t bufferScale = 1;
    int32_t samples = 200;
    bool isRendering = true;

    std::string glbPath = "";
};
