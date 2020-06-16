#pragma once

#include <memory>
#include <array>

#include <GLFW/glfw3.h>

#include "ray_tracer.hpp"
#include "image_buffer.hpp"

class DebugGUI {
  public:
    DebugGUI();

    void setup(GLFWwindow *window, RayTracer &raytracer);
    void renderFrame(RayTracer &raytracer);

    std::string pullSavingImagePath();
    std::string pullOpeningGLBPath();

  private:
    static const int32_t DeltaTimesBufferSize = 90;

    static bool openFileDialog(std::string &path, const char *const filter);
    static bool saveFileDialog(std::string &path, const char *const filter,
                               const char *const defExt);
    std::array<float, DeltaTimesBufferSize> deltaTimes {};
    int32_t deltaTimesOffset = 0;

    RenderingMode renderingMode = ALBEDO;

    std::string savingImagePath = "";
    std::string openingGLBPath = "";

    void onSaveImage();
    void onOpenGLB();
};
