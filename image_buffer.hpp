#pragma once

#include <memory>

#include <glm/glm.hpp>

#include <glm/ext.hpp>

class ImageBuffer {
  private:
    std::shared_ptr<glm::vec3[]> buffer;
    std::shared_ptr<glm::vec3[]> normal;
    std::shared_ptr<glm::vec3[]> albedo;
    std::shared_ptr<glm::vec3[]> skybox;
    std::shared_ptr<uint8_t []> yuv420;
    std::shared_ptr<glm::u8vec3[]> textureBuffer;
    glm::i32vec2 size;

    void encodeYUV420();

  public:
    ImageBuffer();
    ImageBuffer(const glm::i32vec2 &size);
    void resize(const glm::i32vec2 &size);
    void reset();
    auto getChannels() const { return 3; }
    auto getWidth() const { return this->size.x; }
    auto getHeight() const { return this->size.y; }
    auto getAspect() const {
        return static_cast<float>(getWidth()) / getHeight();
    }
    auto getBuffer() { return this->buffer; }
    auto getNormal() { return this->normal; }
    auto getAlbedo() { return this->albedo; }
    auto getSkybox() { return this->skybox; }
    auto getYUV420() { return this->yuv420; }

    const auto GetReadonlyBuffer() const {
        return static_cast<const glm::vec3 *>(this->buffer.get());
    }
    const auto GetTextureBuffer() const {
        return static_cast<const glm::u8vec3 *>(this->textureBuffer.get());
    }
    const auto GetReadonlyYUV420() const {
        return static_cast<const glm::uint8_t *>(this->yuv420.get());
    }

    int32_t getYUV420Size() const;

    void updateTextureBuffer(bool filtered, bool yuv420);
};

using PImageBuffer = std::shared_ptr<ImageBuffer>;
