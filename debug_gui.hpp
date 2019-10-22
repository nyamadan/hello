#pragma once

#include <memory>

#include <GLFW/glfw3.h>

#include "image_buffer.hpp"

class DebugGUI {
  public:
    DebugGUI();

    void setup(GLFWwindow *window, std::shared_ptr<const ImageBuffer> image);
    void beginFrame();
    void renderFrame();

  private:
    std::shared_ptr<const ImageBuffer> image = nullptr;
    void onSaveImage();
};
