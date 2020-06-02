#pragma once

#include <memory>

#include <glm/glm.hpp>

#include <glm/ext.hpp>

class ImageBuffer {
  private:
    std::shared_ptr<glm::u8vec3[]> buffer;
    glm::i32vec2 size;

  public:
    ImageBuffer(const glm::i32vec2 &size);
    void resize(const glm::i32vec2 &size);
    auto getChannels() const { return 3; }
    auto getWidth() const { return this->size.x; }
    auto getHeight() const { return this->size.y; }
    auto getAspect() const {
        return static_cast<float>(getWidth()) / getHeight();
    }
    auto getBuffer() { return this->buffer.get(); }
    const auto GetReadonlyBuffer() const {
        return static_cast<const glm::u8vec3 *>(this->buffer.get());
    }
};

using PImageBuffer = std::shared_ptr<ImageBuffer>;
