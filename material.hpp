#pragma once

#include <glm/glm.hpp>

#include <stdint.h>
#include <memory>

#include "alloc.hpp"

enum MaterialType { REFLECTION = 0, REFRACTION };

class Material {
  public:
    void *operator new(size_t size) { return aligned_alloc(size, 16); }
    void operator delete(void *ptr) { aligned_free(ptr); }
    void *operator new[](size_t size) { return aligned_alloc(size, 16); }
    void operator delete[](void *ptr) { aligned_free(ptr); }

    glm::vec4 baseColorFactor;
    std::shared_ptr<const Texture> baseColorTexture;
    std::shared_ptr<const Texture> normalTexture;
    float roughnessFactor;
    float metalnessFactor;
    glm::vec3 emissiveFactor;
    std::shared_ptr<const Texture> emissiveTexture;
    std::shared_ptr<const Texture> metallicRoughnessTexture;
    MaterialType materialType;

    Material() {
        this->baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        this->baseColorTexture = nullptr;
        this->normalTexture = nullptr;
        this->roughnessFactor = 0.5f;
        this->metalnessFactor = 0.5f;
        this->metallicRoughnessTexture = nullptr;
        this->emissiveFactor = glm::vec3(0.0f, 0.0f, 0.0f);
        this->emissiveTexture = nullptr;
        this->materialType = REFLECTION;
    }

    Material(MaterialType materialType, const glm::vec4 &baseColorFactor,
             std::shared_ptr<const Texture> baseColorTexture,
             std::shared_ptr<const Texture> normalTexture,
             float roughnessFactor, float metalnessFactor,
             std::shared_ptr<const Texture> metallicRoughnessTexture,
             const glm::vec3 &emissiveFactor,
             std::shared_ptr<const Texture> emissiveTexture) {
        this->baseColorFactor = baseColorFactor;
        this->baseColorTexture = baseColorTexture;
        this->normalTexture = normalTexture;
        this->roughnessFactor = roughnessFactor;
        this->metalnessFactor = metalnessFactor;
        this->metallicRoughnessTexture = metallicRoughnessTexture;
        this->emissiveFactor = emissiveFactor;
        this->emissiveTexture = emissiveTexture;
        this->materialType = materialType;
    }

    std::shared_ptr<Material> clone() const {
        auto p = std::shared_ptr<Material>(new Material());

        p->baseColorFactor = this->baseColorFactor;
        p->baseColorTexture = this->baseColorTexture;
        p->normalTexture = this->normalTexture;
        p->roughnessFactor = this->roughnessFactor;
        p->metalnessFactor = this->metalnessFactor;
        p->metallicRoughnessTexture = this->metallicRoughnessTexture;
        p->emissiveFactor = this->emissiveFactor;
        p->emissiveTexture = this->emissiveTexture;
        p->materialType = this->materialType;

        return p;
    }
};

using PMaterial = std::shared_ptr<Material>;
using ConstantPMaterial = std::shared_ptr<const Material>;
using ConstantPMaterialList = std::list<ConstantPMaterial>;
