#pragma once

#include <iostream>
#include <list>
#include <memory>

#include <glm/glm.hpp>

#include <embree3/rtcore.h>
#include <tiny_gltf.h>

#include "alloc.hpp"

#include "texture.hpp"
#include "material.hpp"

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
