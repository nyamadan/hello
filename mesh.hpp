#pragma once

#include <iostream>
#include <vector>
#include <list>
#include <memory>
#include <utility>
#include <numeric>
#include <algorithm>
#include <functional>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

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
    std::shared_ptr<Primitive> clone() const {
        auto p = std::shared_ptr<Primitive>(new Primitive());
        p->material = this->material->clone();
        p->triangles = triangles;
        p->positions = positions;
        p->normals = normals;
        p->texCoords0 = texCoords0;
        p->tangents = tangents;
        return p;
    }

    template <class T>
    void setTriangles(T &&triangles) {
        this->triangles = std::forward<T>(triangles);
    }
    const std::vector<glm::u32vec3> &getTriangles() const {
        return this->triangles;
    }

    template <class T>
    void setPositions(T &&positions) {
        this->positions = std::forward<T>(positions);
    }
    const std::vector<glm::vec3> &getPositions() const {
        return this->positions;
    }

    template <class T>
    void setNormals(T &&normals) {
        this->normals = std::forward<T>(normals);
    }
    const std::vector<glm::vec3> &getNormals() const { return this->normals; }

    template <class T>
    void setTexCoords0(T &&texCoords0) {
        this->texCoords0 = std::forward<T>(texCoords0);
    }
    const std::vector<glm::vec2> &getTexCoords0() const {
        return this->texCoords0;
    }

    template <class T>
    void setTangents(T &&tangents) {
        this->tangents = std::forward<T>(tangents);
    }
    const std::vector<glm::vec4> &getTangents() const { return this->tangents; }

    void setMaterial(ConstantPMaterial material) { this->material = material; }
    ConstantPMaterial getMaterial() const { return this->material; }
};

using PPrimitive = std::shared_ptr<Primitive>;
using ConstantPPrimitive = std::shared_ptr<const Primitive>;

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

using PMesh = std::shared_ptr<Mesh>;
using ConstantPMesh = std::shared_ptr<const Mesh>;

class Node {
  private:
    int32_t index = 0;

    glm::mat4 matrix = glm::mat4(1.0f);

    std::vector<std::shared_ptr<const Node>> children;

    std::shared_ptr<const Mesh> mesh = std::shared_ptr<const Mesh>(nullptr);

  public:
    Node(int32_t index_) : index(index_), matrix(glm::mat4(1.0f)) {}

    int32_t getIndex() const { return this->index; }

    void setMesh(std::shared_ptr<const Mesh> mesh) { this->mesh = mesh; }
    std::shared_ptr<const Mesh> const getMesh() const { return this->mesh; }

    void setMatrix(const glm::mat4 &matrix) { this->matrix = matrix; }
    const glm::mat4 &getMatrix() const { return this->matrix; }

    void addChild(std::shared_ptr<const Node> node) {
        this->children.push_back(node);
    }
    const std::vector<std::shared_ptr<const Node>> &getChildren() const {
        return this->children;
    }
};

using PNode = std::shared_ptr<Node>;
using ConstantPNode = std::shared_ptr<const Node>;
using ConstantPNodeList = std::vector<ConstantPNode>;

class Scene {
  private:
    std::vector<ConstantPNode> nodes;

  public:
    void addNode(ConstantPNode node) { this->nodes.push_back(node); }
    const std::vector<ConstantPNode> &getNodes() const { return this->nodes; }
};

using PScene = std::shared_ptr<Scene>;
using ConstantPScene = std::shared_ptr<const Scene>;

class AnimationSampler {
  public:
    template <typename T>
    void setTimeline(T &&x) {
        this->timeline = std::forward<T>(x);
    }
    template <typename T>
    void setValues(T &&x) {
        this->values = std::forward<T>(x);
    }

    const std::vector<float> &getValues() const { return this->values; }
    const std::vector<float> &getTimeline() const { return this->timeline; }

  private:
    std::vector<float> timeline;
    std::vector<float> values;
};
using PAnimationSampler = std::shared_ptr<AnimationSampler>;
using ConstantPAnimationSampler = std::shared_ptr<const AnimationSampler>;

class AnimationChannel {
  private:
    ConstantPAnimationSampler sampler;
    ConstantPNode targetNode;
    std::string targetPath;

  public:
    void setSampler(ConstantPAnimationSampler sampler) {
        this->sampler = sampler;
    }

    ConstantPAnimationSampler getSampler() const { return this->sampler; }

    void setTargetNode(ConstantPNode node) { this->targetNode = node; }

    ConstantPNode getTargetNode() const { return this->targetNode; }

    void setTargetPath(const std::string &targetPath) {
        this->targetPath = targetPath;
    }

    const std::string &getTargetPath() const { return this->targetPath; }
};
using PAnimationChannel = std::shared_ptr<AnimationChannel>;
using ConstantPAnimationChannel = std::shared_ptr<const AnimationChannel>;

class Animation {
  private:
    int32_t index;
    std::string name;
    std::vector<ConstantPAnimationSampler> samplers;
    std::vector<ConstantPAnimationChannel> channels;

  public:
    Animation(int32_t index_) : index(index_) {}

    void setName(const std::string name) { this->name = name; }
    const std::string &getName() const { return this->name; }

    int32_t getAnimationIndex() const { return index; }

    void addSampler(ConstantPAnimationSampler sampler) {
        this->samplers.push_back(sampler);
    }
    const std::vector<ConstantPAnimationSampler> &getSamplers() const {
        return this->samplers;
    }

    void addChannel(ConstantPAnimationChannel channel) {
        this->channels.push_back(channel);
    }
    const std::vector<ConstantPAnimationChannel> &getChannels() const {
        return this->channels;
    }

    float getTimelineMin() const {
        auto min = FLT_MAX;

        for (const auto &ch : channels) {
            const auto &timeline = ch->getSampler()->getTimeline();
            auto it = std::min_element(timeline.cbegin(), timeline.cend());
            if (it != timeline.cend() && min > *it) {
                min = *it;
            }
        }

        return min;
    }

    float getTimelineMax() const {
        auto max = -FLT_MAX;

        for (const auto &ch : channels) {
            const auto &timeline = ch->getSampler()->getTimeline();
            auto it = std::max_element(timeline.cbegin(), timeline.cend());
            if (it != timeline.cend() && max < *it) {
                max = *it;
            }
        }

        return max;
    }
};
using PAnimation = std::shared_ptr<Animation>;
using ConstantPAnimation = std::shared_ptr<const Animation>;

class Model {
  private:
    ConstantPScene scene = nullptr;
    std::vector<ConstantPMaterial> materials;
    std::vector<ConstantPMesh> meshs;
    std::vector<ConstantPNode> nodes;
    std::vector<ConstantPScene> scenes;
    std::vector<ConstantPTexture> textures;
    std::vector<ConstantPAnimation> animations;

  public:
    void setScene(ConstantPScene scene) { this->scene = scene; }
    ConstantPScene getScene() const { return this->scene; }

    void addScene(ConstantPScene scene) { this->scenes.push_back(scene); }
    const std::vector<ConstantPScene> &getScenes() const {
        return this->scenes;
    }

    void addMaterial(ConstantPMaterial material) {
        this->materials.push_back(material);
    }
    const std::vector<ConstantPMaterial> &getMaterials() const {
        return this->materials;
    }

    void addMesh(ConstantPMesh mesh) { this->meshs.push_back(mesh); }
    const std::vector<ConstantPMesh> &getMeshs() const { return this->meshs; }

    void addNode(ConstantPNode node) { this->nodes.push_back(node); }
    const std::vector<ConstantPNode> &getNodes() const { return this->nodes; }

    void addTexture(ConstantPTexture texture) {
        this->textures.push_back(texture);
    }
    const std::vector<ConstantPTexture> &getTextures() const {
        return this->textures;
    }

    void addAnimation(ConstantPAnimation animation) {
        this->animations.push_back(animation);
    }
    const std::vector<ConstantPAnimation> &getAnimations() const {
        return this->animations;
    }
};

using PModel = std::shared_ptr<Model>;
using ConstantPModel = std::shared_ptr<const Model>;
using ConstantPModelList = std::vector<std::shared_ptr<const Model>>;

class Transform {
  public:
    glm::vec3 scale = glm::vec3(1.0f);
    glm::vec3 translate = glm::vec3(1.0f);
    glm::quat rotate = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    glm::mat4 toMat4() const {
        return glm::translate(translate) * glm::toMat4(rotate) *
               glm::scale(scale);
    }
};

class Geometry {
  private:
    RTCGeometry geom = nullptr;
    uint32_t geomID = 0;

    glm::mat4 transform = glm::mat4(1.0f);

    ConstantPNodeList nodes;
    ConstantPPrimitive primitive;

    Geometry() {}

    void *getUserData() const;

    static std::list<std::shared_ptr<const Geometry>> generateGeometries(
        RTCDevice device, RTCScene scene, ConstantPNodeList nodes,
        const glm::mat4 &parent);

  public:
    static std::list<std::shared_ptr<const Geometry>> generateGeometries(
        RTCDevice device, RTCScene scene, ConstantPNode node);
    static std::list<std::shared_ptr<const Geometry>> generateGeometries(
        RTCDevice device, RTCScene scene, ConstantPNode node,
        const glm::mat4 &parent);

    static std::list<std::shared_ptr<const Geometry>> updateGeometries(
        RTCDevice device, RTCScene scene,
        std::list<std::shared_ptr<const Geometry>> geometries,
        ConstantPAnimation animation, float timeStep);

    uint32_t getGeomId() const;
    RTCGeometry getGeom() const;
    ConstantPMaterial getMaterial() const;
    std::shared_ptr<Geometry> clone() const;

    void setPrimitive(ConstantPPrimitive primitive);
    ConstantPPrimitive getPrimitive() const;

    void setUserData(void *) const;
    void release(RTCScene scene) const;
};

using PGeometry = std::shared_ptr<Geometry>;
using ConstantPGeometry = std::shared_ptr<const Geometry>;
using ConstantPGeometryList = std::list<std::shared_ptr<const Geometry>>;

ConstantPModel loadSphere(ConstantPMaterial material,
                          uint32_t widthSegments = 8,
                          uint32_t heightSegments = 6,
                          const glm::mat4 transform = glm::mat4(1.0f));

/* adds a cube to the scene */
ConstantPModel loadCube(ConstantPMaterial material,
                        glm::mat4 transform = glm::mat4(1.0f));

/* adds a ground plane to the scene */
ConstantPModel loadPlane(ConstantPMaterial material,
                         const glm::mat4 transform = glm::mat4(1.0f));

ConstantPModel loadGltfModel(const tinygltf::Model &gltfModel);

ConstantPModel loadObjModel(const std::string &filename);
