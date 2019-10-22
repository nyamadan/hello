#pragma once

#include <memory>

#include <glm/glm.hpp>

#include <glm/ext.hpp>

class ImageBuffer {
  private:
    std::shared_ptr<glm::u8vec3[]> buffer;
    uint32_t width;
    uint32_t height;

  public:
    ImageBuffer(uint32_t width, uint32_t height);

    auto getWidth() const { return this->width; }

    auto getHeight() const { return this->height; }

    auto getBuffer() { return this->buffer.get(); }

    const auto GetReadonlyBuffer() const { return this->buffer.get(); }
};
