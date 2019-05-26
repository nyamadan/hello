#pragma once

#include <memory>
#include <vector>

#include <glm/glm.hpp>

#include <embree3/rtcore.h>
#include <tiny_gltf.h>

class Material {
  public:
    glm::vec4 baseColorFactor;
    float metallicFactor;

    Material() {
        this->baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        this->metallicFactor = 0.0f;
    }

    Material(const glm::vec4& baseColorFactor) {
        this->baseColorFactor = baseColorFactor;
        this->metallicFactor = 0.0f;
    }

    Material(const glm::vec4& baseColorFactor, float metallicFactor) {
        this->baseColorFactor = baseColorFactor;
        this->metallicFactor = metallicFactor;
    }
};

class Mesh {
  private:
    uint32_t geomId;
    std::shared_ptr<Material> material;

  public:
    Mesh() {
        this->geomId = 0;
        this->material = nullptr;
    }

    Mesh(uint32_t geomId) {
        this->geomId = geomId;
        this->material = nullptr;
    }

    Mesh(uint32_t geomId, std::shared_ptr<Material> material) {
        this->geomId = geomId;
        this->material = material;
    }

    uint32_t getGeometryId() const { return this->geomId; }
    const std::shared_ptr<Material> getMaterial() const { return this->material; }
};

std::shared_ptr<Mesh> addSphere(const RTCDevice device, const RTCScene scene,
                                float radius = 1.0f, uint32_t widthSegments = 8,
                                uint32_t heightSegments = 6,
                                const glm::mat4 transform = glm::mat4(1.0f));

/* adds a cube to the scene */
std::shared_ptr<Mesh> addCube(RTCDevice device, RTCScene scene,
                              glm::mat4 transform = glm::mat4(1.0f));

/* adds a ground plane to the scene */
std::shared_ptr<Mesh> addGroundPlane(
    RTCDevice device, RTCScene scene,
    const glm::mat4 transform = glm::mat4(1.0f));

void addMesh(const RTCDevice device, const RTCScene rtcScene,
             const tinygltf::Model &model, const tinygltf::Mesh &gltfMesh,
             const glm::mat4 &world, std::vector<std::shared_ptr<Mesh>> &meshs);

void addNode(const RTCDevice device, const RTCScene scene,
             const tinygltf::Model &model, const tinygltf::Node &node,
             const glm::mat4 world, std::vector<std::shared_ptr<Mesh>> &meshs);

std::vector<std::shared_ptr<Mesh>> addModel(const RTCDevice device,
                                            const RTCScene rtcScene,
                                            const tinygltf::Model &model);
