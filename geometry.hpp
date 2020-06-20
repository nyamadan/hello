#pragma once

#include <iostream>
#include <list>
#include <memory>

#include <glm/glm.hpp>

#include <embree3/rtcore.h>
#include <tiny_gltf.h>

#include "alloc.hpp"

class Texture {
  private:
    std::shared_ptr<glm::u8vec4[]> buffer;
    int32_t width;
    int32_t height;

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
            int32_t height) {
        this->buffer = buffer;
        this->width = width;
        this->height = height;
    }
};

class Material {
  public:
    void *operator new(size_t size) { return aligned_alloc(size, 16); }
    void operator delete(void *ptr) { aligned_free(ptr); }
    void *operator new[](size_t size) { return aligned_alloc(size, 16); }
    void operator delete[](void *ptr) { aligned_free(ptr); }

    glm::vec4 baseColorFactor;
    std::shared_ptr<const Texture> baseColorTexture;
    float metallicFactor;
    glm::vec3 emissiveFactor;
    std::shared_ptr<const Texture> emissiveTexture;
    bool isLight;

    Material() {
        this->baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        this->baseColorTexture = nullptr;
        this->metallicFactor = 0.0f;
        isLight = false;
    }

    Material(const glm::vec4 &baseColorFactor,
             std::shared_ptr<const Texture> baseColorTexture,
             float metallicFactor,
             const glm::vec3 &emissiveFactor,
             std::shared_ptr<const Texture> emissiveTexture,
             bool isLight) {
        this->baseColorFactor = baseColorFactor;
        this->baseColorTexture = baseColorTexture;
        this->metallicFactor = metallicFactor;
        this->emissiveFactor = emissiveFactor;
        this->emissiveTexture = emissiveTexture;
        this->isLight = isLight;
    }
};

using PMaterial = std::shared_ptr<Material>;
using ConstantPMaterial = std::shared_ptr<const Material>;
using ConstantPMaterialList = std::list<ConstantPMaterial>;

class Mesh {
  public:
    void *operator new(size_t size) { return aligned_alloc(size, 16); }
    void operator delete(void *ptr) { aligned_free(ptr); }
    void *operator new[](size_t size) { return aligned_alloc(size, 16); }
    void operator delete[](void *ptr) { aligned_free(ptr); }

  private:
    uint32_t geomId;
    ConstantPMaterial material;

  public:
    Mesh() {
        this->geomId = 0;
        this->material = nullptr;
    }

    void setGeometryId(uint32_t geomId) { this->geomId = geomId; }
    uint32_t getGeometryId() const { return this->geomId; }

    void setMaterial(ConstantPMaterial material) { this->material = material; }
    const ConstantPMaterial getMaterial() const { return this->material; }
};

using PMesh = std::shared_ptr<Mesh>;
using ConstantPMesh = std::shared_ptr<const Mesh>;
using ConstantPMeshList = std::list<ConstantPMesh>;

ConstantPMesh addSphere(const RTCDevice device, const RTCScene scene,
                        ConstantPMaterial material, float radius = 1.0f,
                        uint32_t widthSegments = 8, uint32_t heightSegments = 6,
                        const glm::mat4 transform = glm::mat4(1.0f));

/* adds a cube to the scene */
ConstantPMesh addCube(RTCDevice device, RTCScene scene,
                      ConstantPMaterial material,
                      glm::mat4 transform = glm::mat4(1.0f));

/* adds a ground plane to the scene */
ConstantPMesh addGroundPlane(RTCDevice device, RTCScene scene,
                             ConstantPMaterial material,
                             const glm::mat4 transform = glm::mat4(1.0f));

void addMesh(const RTCDevice device, const RTCScene scene,
             const tinygltf::Model &model,
             const std::vector<std::shared_ptr<const Texture>> images,
             const tinygltf::Mesh &gltfMesh, const glm::mat4 &world,
             std::list<PMesh> &meshs);

void addNode(const RTCDevice device, const RTCScene scene,
             const tinygltf::Model &model,
             const std::vector<std::shared_ptr<const Texture>> images,
             const tinygltf::Node &node, const glm::mat4 world,
             std::list<PMesh> &meshs);

ConstantPMeshList addModel(const RTCDevice device, const RTCScene scene,
                           const tinygltf::Model &model);
