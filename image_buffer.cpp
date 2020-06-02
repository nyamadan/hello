#include "image_buffer.hpp"

ImageBuffer::ImageBuffer(const glm::i32vec2 &size) {
    this->resize(size);
}

void ImageBuffer::resize(const glm::i32vec2 &size) {
    this->size = size;
    this->buffer = std::shared_ptr<glm::u8vec3[]>(
        new glm::u8vec3[(uint64_t)size.x * size.y]);
}