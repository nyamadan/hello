#pragma once

#include <list>
#include <memory>

#include <glm/glm.hpp>

#include <embree3/rtcore.h>
#include <tiny_gltf.h>

#include "alloc.hpp"

class Material {
  public:
    void *operator new(size_t size) { return aligned_alloc(size, 16); }
    void operator delete(void *ptr) { aligned_free(ptr); }
    void *operator new[](size_t size) { return aligned_alloc(size, 16); }
    void operator delete[](void *ptr) { aligned_free(ptr); }

    glm::vec4 baseColorFactor;
    float metallicFactor;
    glm::vec3 emissiveFactor;

    Material() {
        this->baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        this->metallicFactor = 0.0f;
    }

    Material(const glm::vec4 &baseColorFactor, float metallicFactor, const glm::vec3 &emissiveFactor) {
        this->baseColorFactor = baseColorFactor;
        this->metallicFactor = metallicFactor;
        this->emissiveFactor = emissiveFactor;
    }
};

using PMaterial = std::shared_ptr<Material>;
using ConstantPMaterial = std::shared_ptr<const Material>;

class Mesh {
  private:
    uint32_t geomId;
    ConstantPMaterial material;

  public:
    Mesh() {
        this->geomId = 0;
        this->material = nullptr;
    }

    Mesh(uint32_t geomId) {
        this->geomId = geomId;
        this->material = nullptr;
    }

    Mesh(uint32_t geomId, PMaterial material) {
        this->geomId = geomId;
        this->material = material;
    }

    uint32_t getGeometryId() const { return this->geomId; }
    const ConstantPMaterial getMaterial() const {
        return this->material;
    }
};

using PMesh = std::shared_ptr<Mesh>;
using ConstantPMesh = std::shared_ptr<const Mesh>;
using ConstantPMeshList = std::list<ConstantPMesh>;

ConstantPMesh addSphere(const RTCDevice device, const RTCScene scene,
                        float radius = 1.0f, uint32_t widthSegments = 8,
                        uint32_t heightSegments = 6,
                        const glm::mat4 transform = glm::mat4(1.0f));

/* adds a cube to the scene */
ConstantPMesh addCube(RTCDevice device, RTCScene scene,
                      glm::mat4 transform = glm::mat4(1.0f));

/* adds a ground plane to the scene */
ConstantPMesh addGroundPlane(RTCDevice device, RTCScene scene,
                             const glm::mat4 transform = glm::mat4(1.0f));

void addMesh(const RTCDevice device, const RTCScene rtcScene,
             const tinygltf::Model &model, const tinygltf::Mesh &gltfMesh,
             const glm::mat4 &world, std::list<PMesh> &meshs);

void addNode(const RTCDevice device, const RTCScene scene,
             const tinygltf::Model &model, const tinygltf::Node &node,
             const glm::mat4 world, std::list<PMesh> &meshs);

ConstantPMeshList addModel(const RTCDevice device, const RTCScene rtcScene,
                           const tinygltf::Model &model);
