#pragma once

#include <memory>
#include <array>

#include <GLFW/glfw3.h>

#include "image_buffer.hpp"

class DebugGUI {
  public:
    DebugGUI();

    void setup(GLFWwindow *window);
    void beginFrame();
    void renderFrame();

    std::string pullSavingImagePath();
    std::string pullOpeningGLBPath();

  private:
    static const int32_t DeltaTimesBufferSize = 90;

    static bool openFileDialog(std::string &path, const char *const filter);
    static bool saveFileDialog(std::string &path, const char *const filter,
                               const char *const defExt);
    std::array<float, DeltaTimesBufferSize> deltaTimes {};
    int32_t deltaTimesOffset = 0;

    std::string savingImagePath = "";
    std::string openingGLBPath = "";

    void onSaveImage();
    void onOpenGLB();
};
