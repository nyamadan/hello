#include "image_buffer.hpp"
#include "color.hpp"

#include <libyuv.h>

ImageBuffer::ImageBuffer() { this->resize(glm::i32vec2(0)); }

ImageBuffer::ImageBuffer(const glm::i32vec2 &size) { this->resize(size); }

void ImageBuffer::resize(const glm::i32vec2 &size) {
    auto arrayLength = (uint64_t)size.x * size.y;
    this->size = size;
    this->normal = std::shared_ptr<glm::vec3[]>(new glm::vec3[arrayLength]);
    this->albedo = std::shared_ptr<glm::vec3[]>(new glm::vec3[arrayLength]);
    this->buffer = std::shared_ptr<glm::vec3[]>(new glm::vec3[arrayLength]);
    this->textureBuffer =
        std::shared_ptr<glm::u8vec3[]>(new glm::u8vec3[arrayLength]);

    const int32_t ySize = size.x * size.y;
    const int32_t uSize = ySize / 4;
    const int32_t vSize = uSize;
    this->yuv420 = std::shared_ptr<uint8_t []>(new uint8_t[ySize + uSize + vSize]);

    this->reset();
}

void ImageBuffer::reset() {
    const auto arrayLength = (uint64_t)size.x * size.y;
    memset(this->normal.get(), 0, sizeof(glm::vec3) * arrayLength);
    memset(this->albedo.get(), 0, sizeof(glm::vec3) * arrayLength);
    memset(this->buffer.get(), 0, sizeof(glm::vec3) * arrayLength);
    memset(this->textureBuffer.get(), 0, sizeof(glm::u8vec3) * arrayLength);

    const int32_t ySize = size.x * size.y;
    const int32_t uSize = ySize / 4;
    const int32_t vSize = uSize;
    memset(this->yuv420.get(), 0, sizeof(uint8_t) * (ySize + uSize + vSize));
}

void ImageBuffer::updateTextureBuffer(bool filtered, bool encodeYUV420) {
    auto width = getWidth();
    auto height = getHeight();
    auto len = width * height;

    for (auto i = 0; i < len; i++) {
        const auto &src = GetReadonlyBuffer()[i];
        if (filtered) {
            textureBuffer[i] =
                glm::u8vec3(toneMapping(linearToGamma(src)) * 255.0f);
        } else {
            textureBuffer[i] = glm::u8vec3(glm::clamp(src) * 255.0f);
        }
    }

    if(encodeYUV420) {
        this->encodeYUV420();
    }
}

void ImageBuffer::encodeYUV420() {
    const auto bufferWidth = getWidth();
    const auto bufferHeight = getHeight();

    const int32_t ySize = bufferWidth * bufferHeight;
    const int32_t uSize = ySize / 4;
    const int32_t vSize = uSize;

    const int32_t yStride = bufferWidth;
    const int32_t uStride = bufferWidth / 2;
    const int32_t vStride = uStride;

    uint8_t *yBuffer = yuv420.get();
    uint8_t *uBuffer = yBuffer + ySize;
    uint8_t *vBuffer = uBuffer + uSize;

    libyuv::RAWToI420((const uint8_t *)textureBuffer.get(), bufferWidth * 3, yBuffer,
                      yStride, uBuffer, uStride, vBuffer, vStride, bufferWidth,
                      -bufferHeight);
}

int32_t ImageBuffer::getYUV420Size() const {
    const auto bufferWidth = getWidth();
    const auto bufferHeight = getHeight();

    const int32_t ySize = bufferWidth * bufferHeight;
    const int32_t uSize = ySize / 4;
    const int32_t vSize = uSize;
    return ySize + uSize + vSize;
}