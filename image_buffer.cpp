#include "image_buffer.hpp"

ImageBuffer::ImageBuffer(uint32_t width, uint32_t height) {
    this->width = width;
    this->height = height;
    this->buffer = std::shared_ptr<glm::u8vec3[]>(
        new glm::u8vec3[(uint64_t)width * height]);
}