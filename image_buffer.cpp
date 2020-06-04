#include "image_buffer.hpp"

ImageBuffer::ImageBuffer() {
    this->resize(glm::i32vec2(0));
}

ImageBuffer::ImageBuffer(const glm::i32vec2 &size) {
    this->resize(size);
}

void ImageBuffer::resize(const glm::i32vec2 &size) {
    auto arrayLength = (uint64_t)size.x * size.y;
    this->size = size;
    this->buffer = std::shared_ptr<glm::vec3[]>(new glm::vec3[arrayLength]);
    this->textureBuffer = std::shared_ptr<glm::u8vec3[]>(new glm::u8vec3[arrayLength]);

    memset(this->buffer.get(), 0, sizeof(glm::vec3) * arrayLength);
}

void ImageBuffer::updateTextureBuffer() {
    auto width = getWidth();
    auto height = getHeight();

    for (auto y = 0; y < height; y++) {
        for (auto x = 0; x < width; x++) {
            auto index = y * width + x;
            const auto &src = GetReadonlyBuffer()[index];
            textureBuffer[index] = glm::u8vec3(
                static_cast<uint8_t>(glm::clamp(src.r, 0.0f, 1.0f) * 255.0f),
                static_cast<uint8_t>(glm::clamp(src.g, 0.0f, 1.0f) * 255.0f),
                static_cast<uint8_t>(glm::clamp(src.b, 0.0f, 1.0f) * 255.0f)
            );
        }
    }
}