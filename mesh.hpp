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
    ConstantPMaterial material;

    std::vector<glm::u32vec3> triangles;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords0;
    std::vector<glm::vec4> tangents;

  public:
    Primitive() { this->material = nullptr; }

    void setTriangles(std::vector<glm::u32vec3> &triangles) {
        this->triangles = triangles;
    }
    const std::vector<glm::u32vec3> &getTriangles() const {
        return this->triangles;
    }

    void setPositions(std::vector<glm::vec3> &positions) {
        this->positions = positions;
    }
    const std::vector<glm::vec3> &getPositions() const {
        return this->positions;
    }

    void setNormals(std::vector<glm::vec3> &normals) {
        this->normals = normals;
    }
    const std::vector<glm::vec3> &getNormals() const { return this->normals; }

    void setTexCoords0(std::vector<glm::vec2> &texCoords0) {
        this->texCoords0 = texCoords0;
    }
    const std::vector<glm::vec2> &getTexCoords0() const {
        return this->texCoords0;
    }

    void setTangents(std::vector<glm::vec4> &tangents) {
        this->tangents = tangents;
    }
    const std::vector<glm::vec4> &getTangents() const { return this->tangents; }

    void setMaterial(ConstantPMaterial material) { this->material = material; }
    ConstantPMaterial getMaterial() const { return this->material; }
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
    glm::mat4 matrix;
    glm::mat4 worldMatrix;
    glm::mat4 worldInverseTransposeMatrix;

    std::vector<std::shared_ptr<const Node>> children;

    uint32_t geomId;

    std::shared_ptr<const Mesh> mesh;

  public:
    Node() {
        this->worldMatrix = glm::mat4(1.0f);
        this->worldInverseTransposeMatrix = glm::mat4(1.0f);
    }

    void addChild(std::shared_ptr<const Node> node) {
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

    void setMatrix(const glm::mat4 &matrix) { this->matrix = matrix; }

    const glm::mat4 &getWorldInverseTransposeMatrix() const {
        return this->worldInverseTransposeMatrix;
    }
    const glm::mat4 &getWorldMatrix() const { return this->worldMatrix; }
    const glm::mat4 &getMatrix() const { return this->matrix; }

    const std::vector<std::shared_ptr<const Node>> &getChildren() const {
        return this->children;
    }

    void setGeometryId(uint32_t geomId) { this->geomId = geomId; }
    uint32_t getGeometryId() const { return this->geomId; }
};

class Model {
  private:
    std::vector<std::shared_ptr<const Material>> materials;
    std::vector<std::shared_ptr<const Mesh>> meshs;
    std::vector<std::shared_ptr<const Node>> nodes;
    std::vector<std::shared_ptr<const Texture>> textures;

  public:
    void addMaterial(std::shared_ptr<const Material> material) {
        this->materials.push_back(material);
    }
    const std::vector<std::shared_ptr<const Material>> &getMaterials() const {
        return this->materials;
    }

    void addMesh(std::shared_ptr<const Mesh> mesh) {
        this->meshs.push_back(mesh);
    }
    const std::vector<std::shared_ptr<const Mesh>> &getMeshs() const {
        return this->meshs;
    }

    void addNode(std::shared_ptr<const Node> node) {
        this->nodes.push_back(node);
    }
    const std::vector<std::shared_ptr<const Node>> &getNodes() const {
        return this->nodes;
    }

    void addTexture(std::shared_ptr<const Texture> texture) {
        this->textures.push_back(texture);
    }
    const std::vector<std::shared_ptr<const Texture>> &getTextures() const {
        return this->textures;
    }
};

using PMesh = std::shared_ptr<Mesh>;
using ConstantPMesh = std::shared_ptr<const Mesh>;
using PPrimitive = std::shared_ptr<Primitive>;
using ConstantPPrimitive = std::shared_ptr<const Primitive>;
using PNode = std::shared_ptr<Node>;
using ConstantPNode = std::shared_ptr<const Node>;
using ConstantPNodeList = std::vector<ConstantPNode>;
using PModel = std::shared_ptr<Model>;
using ConstantPModel = std::shared_ptr<const Model>;
using ConstantPModelList = std::vector<std::shared_ptr<const Model>>;

ConstantPModel addSphere(const RTCDevice device, const RTCScene scene,
                         ConstantPMaterial material, uint32_t widthSegments = 8,
                         uint32_t heightSegments = 6,
                         const glm::mat4 transform = glm::mat4(1.0f));

/* adds a cube to the scene */
ConstantPModel addCube(RTCDevice device, RTCScene scene,
                       ConstantPMaterial material,
                       glm::mat4 transform = glm::mat4(1.0f));

/* adds a ground plane to the scene */
ConstantPModel addGroundPlane(RTCDevice device, RTCScene scene,
                              ConstantPMaterial material,
                              const glm::mat4 transform = glm::mat4(1.0f));

ConstantPMesh addMesh(
    const RTCDevice device, const RTCScene scene, const tinygltf::Model &model,
    const std::vector<std::shared_ptr<const Material>> &materials,
    const tinygltf::Mesh &gltfMesh, const glm::mat4 &world);

ConstantPNode addNode(
    const RTCDevice device, const RTCScene scene, const tinygltf::Model &model,
    const std::vector<std::shared_ptr<const Material>> &materials,
    const tinygltf::Node &node, const glm::mat4 &world);

ConstantPModel addGltfModel(const RTCDevice device, const RTCScene scene,
                            const tinygltf::Model &gltfModel);

ConstantPModel addObjModel(const RTCDevice device, const RTCScene scene,
                           const std::string &filename);