#pragma once

#include <memory>

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
    static bool openFileDialog(std::string &path, const char *const filter);
    static bool saveFileDialog(std::string &path, const char *const filter,
                               const char *const defExt);

    std::shared_ptr<const ImageBuffer> image = nullptr;
    std::string savingImagePath = "";
    std::string openingGLBPath = "";

    void onSaveImage();
    void onOpenGLB();
};
