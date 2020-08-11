#pragma once

#include <iostream>
#include <vector>
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

class Primitive {
  private:
    uint32_t geomId;
    ConstantPMaterial material;

  public:
    Primitive() {
        this->geomId = 0;
        this->material = nullptr;
    }

    void setGeometryId(uint32_t geomId) { this->geomId = geomId; }
    uint32_t getGeometryId() const { return this->geomId; }

    void setMaterial(ConstantPMaterial material) { this->material = material; }
    const ConstantPMaterial getMaterial() const { return this->material; }
};

class Mesh {
  private:
    std::vector<std::shared_ptr<const Primitive>> primitives;

  public:
    Mesh() {}
    void addPrimitive(std::shared_ptr<const Primitive> primitive) {
        this->primitives.push_back(primitive);
    }
    const std::vector<std::shared_ptr<const Primitive>> &getPrimitives() const {
        return this->primitives;
    }
};

class Node {
  private:
    glm::mat4 worldMatrix;
    glm::mat4 worldInverseTransposeMatrix;

    std::vector<std::shared_ptr<const Node>> children;

    std::shared_ptr<const Mesh> mesh;

  public:
    Node() {
        this->worldMatrix = glm::mat4(1.0f);
        this->worldInverseTransposeMatrix = glm::mat4(1.0f);
    }

    void addNode(std::shared_ptr<const Node> node) {
        this->children.push_back(node);
    }

    void setMesh(std::shared_ptr<const Mesh> mesh) { this->mesh = mesh; }
    std::shared_ptr<const Mesh> const getMesh() const { return this->mesh; }

    void setWorldInverseTransposeMatrix(
        const glm::mat4 &worldInverseTranspose) {
        this->worldInverseTransposeMatrix = worldInverseTranspose;
    }

    void setWorldMatrix(const glm::mat4 &worldMatrix) {
        this->worldMatrix = worldMatrix;
    }

    const glm::mat4 &getWorldInverseTransposeMatrix() const {
        return this->worldInverseTransposeMatrix;
    }
    const glm::mat4 &getWorldMatrix() const { return this->worldMatrix; }

    const std::vector<std::shared_ptr<const Node>> &getChildren() const {
        return this->children;
    }
};

using PNode = std::shared_ptr<Node>;
using ConstantPNode = std::shared_ptr<const Node>;
using ConstantPNodeList = std::vector<ConstantPNode>;

using PMesh = std::shared_ptr<Mesh>;
using ConstantPMesh = std::shared_ptr<const Mesh>;

using PPrimitive = std::shared_ptr<Primitive>;
using ConstantPPrimitive = std::shared_ptr<const Primitive>;

ConstantPNode addSphere(const RTCDevice device, const RTCScene scene,
                        ConstantPMaterial material, uint32_t widthSegments = 8,
                        uint32_t heightSegments = 6,
                        const glm::mat4 transform = glm::mat4(1.0f));

/* adds a cube to the scene */
ConstantPNode addCube(RTCDevice device, RTCScene scene,
                      ConstantPMaterial material,
                      glm::mat4 transform = glm::mat4(1.0f));

/* adds a ground plane to the scene */
ConstantPNode addGroundPlane(RTCDevice device, RTCScene scene,
                             ConstantPMaterial material,
                             const glm::mat4 transform = glm::mat4(1.0f));

ConstantPMesh addMesh(const RTCDevice device, const RTCScene scene,
                      const tinygltf::Model &model,
                      const std::vector<std::shared_ptr<const Texture>> &images,
                      const tinygltf::Mesh &gltfMesh, const glm::mat4 &world);

ConstantPNode addNode(const RTCDevice device, const RTCScene scene,
                      const tinygltf::Model &model,
                      const std::vector<std::shared_ptr<const Texture>> &images,
                      const tinygltf::Node &node, const glm::mat4 &world);

ConstantPNodeList addGltfModel(const RTCDevice device, const RTCScene scene,
                               const tinygltf::Model &model);

ConstantPNodeList addObjModel(const RTCDevice device, const RTCScene scene,
                              const std::string &filename);