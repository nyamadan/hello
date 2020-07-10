#pragma once

#include <glm/glm.hpp>

#include <stdint.h>
#include <memory>

class Texture {
  private:
    std::shared_ptr<glm::u8vec4[]> buffer;
    int32_t width;
    int32_t height;

    int32_t wrapS;
    int32_t wrapT;

  public:
    const glm::u8vec4 *getBuffer() const {
      return buffer.get();
    }

    int32_t getWidth() const {
      return this->width;
    }

    int32_t getHeight() const {
      return this->height;
    }

    Texture(std::shared_ptr<glm::u8vec4[]> buffer, int32_t width,
            int32_t height, int32_t wrapS, int32_t wrapT) {
        this->buffer = buffer;
        this->width = width;
        this->height = height;
        this->wrapS = wrapS;
        this->wrapT = wrapT;
    }
};
