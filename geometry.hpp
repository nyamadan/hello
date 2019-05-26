#pragma once

#include <vector>

#include <glm/glm.hpp>

#include <embree3/rtcore.h>
#include <tiny_gltf.h>

int addSphere(const RTCDevice device, const RTCScene scene, float radius = 1.0f,
              uint32_t widthSegments = 8, uint32_t heightSegments = 6,
              const glm::mat4 transform = glm::mat4(1.0f));

/* adds a cube to the scene */
unsigned int addCube(RTCDevice device, RTCScene scene,
                     glm::mat4 transform = glm::mat4(1.0f));

/* adds a ground plane to the scene */
unsigned int addGroundPlane(RTCDevice device, RTCScene scene,
                            const glm::mat4 transform = glm::mat4(1.0f));

static void addMesh(const RTCDevice device, const RTCScene rtcScene,
                    const tinygltf::Model &model,
                    const tinygltf::Mesh &gltfMesh, const glm::mat4 &world,
                    std::vector<int> &geomIds);

static void addNode(const RTCDevice device, const RTCScene scene,
                    const tinygltf::Model &model, const tinygltf::Node &node,
                    const glm::mat4 world, std::vector<int> &geomIds);

std::vector<int> addModel(const RTCDevice device, const RTCScene rtcScene,
                          const tinygltf::Model &model);