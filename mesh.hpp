#pragma once

#include <iostream>
#include <list>
#include <memory>

#include <glm/glm.hpp>

#include <embree3/rtcore.h>
#include <tiny_gltf.h>
#include <tiny_obj_loader.h>

#include "alloc.hpp"

#include "texture.hpp"
#include "material.hpp"

enum VertexAttributeSlot {
    SLOT_NORMAL = 0,
    SLOT_TEXCOORD_0,
    SLOT_TANGENT,
    SLOT_BITANGENT,
    NUM_VERTEX_ATTRIBUTE_SLOTS
};

class Mesh {
  public:
    void *operator new(size_t size) { return aligned_alloc(size, 16); }
    void operator delete(void *ptr) { aligned_free(ptr); }
    void *operator new[](size_t size) { return aligned_alloc(size, 16); }
    void operator delete[](void *ptr) { aligned_free(ptr); }

  private:
    uint32_t geomId;
    glm::mat4 worldMatrix;
    glm::mat4 worldInverseTransposeMatrix;
    ConstantPMaterial material;

  public:
    Mesh() {
        this->geomId = 0;
        this->material = nullptr;
        this->worldMatrix = glm::mat4(1.0f);
        this->worldInverseTransposeMatrix = glm::mat4(1.0f);
    }

    void setGeometryId(uint32_t geomId) { this->geomId = geomId; }
    void setWorldInverseTransposeMatrix(const glm::mat4 &worldInverseTranspose) {
        this->worldInverseTransposeMatrix = worldInverseTranspose;
    }
    void setWorldMatrix(const glm::mat4 &worldMatrix) {
        this->worldMatrix = worldMatrix;
    }
    uint32_t getGeometryId() const { return this->geomId; }
    const glm::mat4 &getWorldInverseTransposeMatrix() const {
        return this->worldInverseTransposeMatrix;
    }
    const glm::mat4 &getWorldMatrix() const { return this->worldMatrix; }

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

ConstantPMeshList addGlbModel(const RTCDevice device, const RTCScene scene,
                              const tinygltf::Model &model);

ConstantPMeshList addObjModel(const RTCDevice device, const RTCScene scene,
                              const std::string &filename);