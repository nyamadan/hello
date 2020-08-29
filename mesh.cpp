#include "mesh.hpp"
#include "ray_tracer.hpp"

#include <mikktspace.h>
#include <filesystem>
#include <stack>
#include <numeric>
#include <limits>

#include <stb.h>
#include <stb_image.h>
#include <glm/ext.hpp>

namespace {

void intersectionFilter(const struct RTCFilterFunctionNArguments *args) {
    auto context = (IntersectContext *)args->context;
    auto raytracer = context->raytracer;
    raytracer->intersectionFilter(args);
}

void calcNormal(float N[3], float v0[3], float v1[3], float v2[3]) {
    float v10[3];
    v10[0] = v1[0] - v0[0];
    v10[1] = v1[1] - v0[1];
    v10[2] = v1[2] - v0[2];

    float v20[3];
    v20[0] = v2[0] - v0[0];
    v20[1] = v2[1] - v0[1];
    v20[2] = v2[2] - v0[2];

    N[0] = v10[1] * v20[2] - v10[2] * v20[1];
    N[1] = v10[2] * v20[0] - v10[0] * v20[2];
    N[2] = v10[0] * v20[1] - v10[1] * v20[0];

    float len2 = N[0] * N[0] + N[1] * N[1] + N[2] * N[2];
    if (len2 > 0.0f) {
        float len = sqrtf(len2);

        N[0] /= len;
        N[1] /= len;
        N[2] /= len;
    }
}

// Check if `mesh_t` contains smoothing group id.
bool hasSmoothingGroup(const tinyobj::shape_t &shape) {
    for (size_t i = 0; i < shape.mesh.smoothing_group_ids.size(); i++) {
        if (shape.mesh.smoothing_group_ids[i] > 0) {
            return true;
        }
    }
    return false;
}

void computeSmoothingNormals(const tinyobj::attrib_t &attrib,
                             const tinyobj::shape_t &shape,
                             std::map<int, glm::vec3> &smoothVertexNormals) {
    smoothVertexNormals.clear();
    std::map<int, glm::vec3>::iterator iter;

    for (size_t f = 0; f < shape.mesh.indices.size() / 3; f++) {
        // Get the three indexes of the face (all faces are triangular)
        tinyobj::index_t idx0 = shape.mesh.indices[3 * f + 0];
        tinyobj::index_t idx1 = shape.mesh.indices[3 * f + 1];
        tinyobj::index_t idx2 = shape.mesh.indices[3 * f + 2];

        // Get the three vertex indexes and coordinates
        int vi[3];      // indexes
        float v[3][3];  // coordinates

        for (int k = 0; k < 3; k++) {
            vi[0] = idx0.vertex_index;
            vi[1] = idx1.vertex_index;
            vi[2] = idx2.vertex_index;
            assert(vi[0] >= 0);
            assert(vi[1] >= 0);
            assert(vi[2] >= 0);

            v[0][k] = attrib.vertices[3 * vi[0] + k];
            v[1][k] = attrib.vertices[3 * vi[1] + k];
            v[2][k] = attrib.vertices[3 * vi[2] + k];
        }

        // Compute the normal of the face
        float normal[3];
        calcNormal(normal, v[0], v[1], v[2]);

        // Add the normal to the three vertexes
        for (size_t i = 0; i < 3; ++i) {
            iter = smoothVertexNormals.find(vi[i]);
            if (iter != smoothVertexNormals.end()) {
                // add
                iter->second[0] += normal[0];
                iter->second[1] += normal[1];
                iter->second[2] += normal[2];
            } else {
                smoothVertexNormals[vi[i]][0] = normal[0];
                smoothVertexNormals[vi[i]][1] = normal[1];
                smoothVertexNormals[vi[i]][2] = normal[2];
            }
        }

    }  // f

    // Normalize the normals, that is, make them unit vectors
    for (iter = smoothVertexNormals.begin(); iter != smoothVertexNormals.end();
         iter++) {
        iter->second = glm::normalize(iter->second);
    }

}  // computeSmoothingNormals

class TangentGenerator {
  private:
    static int getNumFaces(const SMikkTSpaceContext *pContext) {
        const auto that = (TangentGenerator *)pContext->m_pUserData;

        return that->numFaces;
    }

    static int getNumVerticesOfFace(const SMikkTSpaceContext *pContext,
                                    const int iFace) {
        return 3;
    }

    static void getPosition(const SMikkTSpaceContext *pContext,
                            float fvPosOut[], const int iFace,
                            const int iVert) {
        const auto that = (TangentGenerator *)pContext->m_pUserData;

        const auto &face = that->faces[iFace];
        const auto &position = that->positions[face[iVert]];

        memcpy(fvPosOut, &position, sizeof(glm::vec3));
    }

    static void getNormal(const SMikkTSpaceContext *pContext, float fvNormOut[],
                          const int iFace, const int iVert) {
        const auto that = (TangentGenerator *)pContext->m_pUserData;

        const auto &face = that->faces[iFace];
        const auto &normal = that->normals[face[iVert]];

        memcpy(fvNormOut, &normal, sizeof(glm::vec3));
    }

    static void getTexCoord(const SMikkTSpaceContext *pContext,
                            float fvTexcOut[], const int iFace,
                            const int iVert) {
        auto that = (TangentGenerator *)pContext->m_pUserData;

        const auto &face = that->faces[iFace];
        const auto &uv = that->uvs[face[iVert]];

        memcpy(fvTexcOut, &uv, sizeof(glm::vec2));
    }

    static void setTSpaceBasic(const SMikkTSpaceContext *pContext,
                               const float fvTangent[], const float fSign,
                               const int iFace, const int iVert) {
        const auto that = (TangentGenerator *)pContext->m_pUserData;
        const auto &face = that->faces[iFace];
        auto &tangent = that->tangents[face[iVert]];

        tangent = glm::vec4(fvTangent[0], fvTangent[1], fvTangent[2], fSign);
    }

    static void setTSpace(const SMikkTSpaceContext *pContext,
                          const float fvTangent[], const float fvBiTangent[],
                          const float fMagS, const float fMagT,
                          const tbool bIsOrientationPreserving, const int iFace,
                          const int iVert) {}

    SMikkTSpaceContext context = {0};
    SMikkTSpaceInterface param = {0};

    int32_t numFaces;
    const glm::u32vec3 *faces;
    const glm::vec3 *positions;
    const glm::vec3 *normals;
    const glm::vec2 *uvs;

    std::vector<glm::vec4> tangents;

    TangentGenerator() {
        param.m_getNumFaces = getNumFaces;
        param.m_getNumVerticesOfFace = getNumVerticesOfFace;
        param.m_getPosition = getPosition;
        param.m_getNormal = getNormal;
        param.m_getTexCoord = getTexCoord;
        param.m_setTSpaceBasic = setTSpaceBasic;
        param.m_setTSpace = setTSpace;
        context.m_pInterface = &param;
        context.m_pUserData = this;
    }

    std::vector<glm::vec4> _generate(int32_t numFaces, int32_t numVertices,
                                     const glm::u32vec3 *faces,
                                     const glm::vec3 *positions,
                                     const glm::vec3 *normals,
                                     const glm::vec2 *uvs) {
        tangents = std::vector<glm::vec4>(numVertices);

        this->numFaces = numFaces;
        this->faces = faces;
        this->positions = positions;
        this->normals = normals;
        this->uvs = uvs;

        genTangSpaceDefault(&context);

        return std::move(tangents);
    }

  public:
    static std::vector<glm::vec4> generate(int32_t numFaces,
                                           int32_t numVertices,
                                           const glm::u32vec3 *faces,
                                           const glm::vec3 *positions,
                                           const glm::vec3 *normals,
                                           const glm::vec2 *uvs) {
        return TangentGenerator()._generate(numFaces, numVertices, faces,
                                            positions, normals, uvs);
    }
};
}  // namespace

std::list<std::shared_ptr<const Geometry>> Geometry::generateGeometries(
    RTCDevice device, RTCScene scene, ConstantPNode node) {
    return Geometry::generateGeometries(device, scene, node, glm::mat4(1.0f));
}

std::list<std::shared_ptr<const Geometry>> Geometry::generateGeometries(
    RTCDevice device, RTCScene scene, ConstantPNode node,
    const glm::mat4 &parent) {
    ConstantPNodeList nodes;
    nodes.push_back(node);
    return Geometry::generateGeometries(device, scene, nodes, parent);
}

std::list<std::shared_ptr<const Geometry>> Geometry::generateGeometries(
    RTCDevice device, RTCScene scene, ConstantPNodeList nodes,
    const glm::mat4 &parent) {
    std::list<std::shared_ptr<const Geometry>> geometries;

    auto node = nodes.back();

    const auto transform = parent * node->getMatrix();
    const auto inverseTranspose = glm::inverseTranspose(transform);

    auto mesh = node->getMesh();
    if (mesh.get() != nullptr) {
        for (auto primitive : mesh->getPrimitives()) {
            const auto &srcTriangles = primitive->getTriangles();
            const auto &srcPositions = primitive->getPositions();
            const auto &srcNormals = primitive->getNormals();
            const auto &srcTexcoords0 = primitive->getTexCoords0();
            const auto &srcTangents = primitive->getTangents();
            const auto material = primitive->getMaterial();
            const auto numVertices = srcPositions.size();
            const auto numFaces = srcTriangles.size();

            auto geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

            rtcSetGeometryVertexAttributeCount(
                geom,
                (uint32_t)VertexAttributeSlot::NUM_VERTEX_ATTRIBUTE_SLOTS);

            auto *positionsBuffer = (glm::vec3 *)rtcSetNewGeometryBuffer(
                geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3,
                sizeof(glm::vec3), numVertices);
            auto *trianglesBuffer = (glm::u32vec3 *)rtcSetNewGeometryBuffer(
                geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
                sizeof(glm::u32vec3), numFaces);
            auto *normalsBuffer = (glm::vec3 *)rtcSetNewGeometryBuffer(
                geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                (uint32_t)VertexAttributeSlot::SLOT_NORMAL, RTC_FORMAT_FLOAT3,
                sizeof(glm::vec3), numVertices);
            auto *texCoords0Buffer = (glm::vec2 *)rtcSetNewGeometryBuffer(
                geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                (uint32_t)VertexAttributeSlot::SLOT_TEXCOORD_0,
                RTC_FORMAT_FLOAT3, sizeof(glm::vec2), numVertices);
            auto *tangentsBuffer = (glm::vec3 *)rtcSetNewGeometryBuffer(
                geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                (uint32_t)VertexAttributeSlot::SLOT_TANGENT, RTC_FORMAT_FLOAT3,
                sizeof(glm::vec3), numVertices);
            auto *bitangentsBuffer = (glm::vec3 *)rtcSetNewGeometryBuffer(
                geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                (uint32_t)VertexAttributeSlot::SLOT_BITANGENT,
                RTC_FORMAT_FLOAT3, sizeof(glm::vec3), numVertices);

            for (auto i = 0; i < numFaces; i++) {
                trianglesBuffer[i] = srcTriangles[i];
            }

            for (auto i = 0; i < numVertices; i++) {
                positionsBuffer[i] =
                    glm::vec3(transform * glm::vec4(srcPositions[i], 1.0f));
                normalsBuffer[i] = glm::normalize(glm::vec3(
                    inverseTranspose * glm::vec4(srcNormals[i], 0.0f)));
                texCoords0Buffer[i] = srcTexcoords0[i];
                tangentsBuffer[i] = glm::normalize(glm::vec3(
                    transform * glm::vec4(glm::vec3(srcTangents[i]), 0.0f)));
                bitangentsBuffer[i] = glm::normalize(glm::vec3(
                    inverseTranspose *
                    glm::vec4(
                        (glm::cross(srcNormals[i], glm::vec3(srcTangents[i])) *
                         srcTangents[i].w),
                        0.0f)));
            }

            rtcSetGeometryUserData(geom, (void *)material.get());
            rtcSetGeometryIntersectFilterFunction(geom, intersectionFilter);
            rtcCommitGeometry(geom);
            auto geomId = rtcAttachGeometry(scene, geom);
            auto geometry = new Geometry();
            geometry->geom = geom;
            geometry->geomID = geomId;
            geometry->nodes = nodes;
            geometry->primitive = primitive;
            geometries.push_back(ConstantPGeometry(geometry));
        }
    }

    for (auto child : node->getChildren()) {
        auto childNodes = nodes;
        childNodes.push_back(child);
        geometries.splice(
            geometries.cend(),
            Geometry::generateGeometries(device, scene, childNodes, transform));
    }

    return geometries;
}

std::list<std::shared_ptr<const Geometry>> Geometry::updateGeometries(
    RTCDevice device, RTCScene scene,
    std::list<std::shared_ptr<const Geometry>> geometries,
    ConstantPAnimation animation, float timeStep, const glm::mat4 &parent) {
    timeStep = glm::mod(timeStep, animation->getTimelineMax());

    for (auto geometry : geometries) {
        auto primitive = geometry->primitive;
        auto nodes = geometry->nodes;

        const auto transform = std::accumulate(
            nodes.begin(), nodes.end(), parent,
            [&](glm::mat4 matrix, ConstantPNode node) {
                if (animation.get() == nullptr) {
                    return matrix * node->getMatrix();
                }

                auto rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                auto translation = glm::vec3(0.0f);
                auto scale = glm::vec3(1.0f);

                for (auto channel : animation->getChannels()) {
                    if (channel->getTargetNode()->getIndex() !=
                        node->getIndex()) {
                        continue;
                    }

                    auto sampler = channel->getSampler();

                    const auto &values = sampler->getValues();
                    const auto &timeline = sampler->getTimeline();
                    auto len = timeline.size();

                    for (auto i = 1; i < len; i++) {
                        const auto t0 = timeline[i - 1];
                        const auto t1 = timeline[i];
                        if (t0 > timeStep || t1 < timeStep) {
                            continue;
                        }

                        if (channel->getTargetPath().compare("rotation") == 0) {
                            const auto &v0 = 4 * (i - 1);
                            const auto &v1 = 4 * i;

                            auto q0 = glm::quat(values[v0 + 3], values[v0 + 0],
                                                values[v0 + 1], values[v0 + 2]);
                            auto q1 = glm::quat(values[v1 + 3], values[v1 + 0],
                                                values[v1 + 1], values[v1 + 2]);
                            rotation *=
                                glm::slerp(q0, q1, (timeStep - t0) / (t1 - t0));
                        }

                        else if (channel->getTargetPath().compare(
                                     "translation") == 0) {
                            const auto &v0 = 3 * (i - 1);
                            const auto &v1 = 3 * i;

                            const auto p0 = glm::vec3(
                                values[v0 + 0], values[v0 + 1], values[v0 + 2]);
                            const auto p1 = glm::vec3(
                                values[v1 + 0], values[v1 + 1], values[v1 + 2]);
                            translation +=
                                glm::lerp(p0, p1, (timeStep - t0) / (t1 - t0));
                        }

                        else if (channel->getTargetPath().compare("scale") ==
                                 0) {
                            const auto &v0 = 3 * (i - 1);
                            const auto &v1 = 3 * i;

                            const auto p0 = glm::vec3(
                                values[v0 + 0], values[v0 + 1], values[v0 + 2]);
                            const auto p1 = glm::vec3(
                                values[v1 + 0], values[v1 + 1], values[v1 + 2]);
                            scale +=
                                glm::lerp(p0, p1, (timeStep - t0) / (t1 - t0));
                        }

                        break;
                    }
                }

                return matrix * glm::translate(translation) *
                       glm::toMat4(rotation) * glm::scale(scale) *
                       node->getMatrix();
            });

        const auto inverseTranspose = glm::inverseTranspose(transform);

        const auto &srcTriangles = primitive->getTriangles();
        const auto &srcPositions = primitive->getPositions();
        const auto &srcNormals = primitive->getNormals();
        const auto &srcTexcoords0 = primitive->getTexCoords0();
        const auto &srcTangents = primitive->getTangents();
        const auto material = primitive->getMaterial();

        const auto numVertices = srcPositions.size();
        const auto numFaces = srcTriangles.size();

        auto geom = geometry->geom;
        auto positionsBuffer = (glm::vec3 *)rtcGetGeometryBufferData(
            geom, RTC_BUFFER_TYPE_VERTEX, 0);
        auto *normalsBuffer = (glm::vec3 *)rtcGetGeometryBufferData(
            geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
            (uint32_t)VertexAttributeSlot::SLOT_NORMAL);
        auto *texCoords0Buffer = (glm::vec2 *)rtcGetGeometryBufferData(
            geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
            (uint32_t)VertexAttributeSlot::SLOT_TEXCOORD_0);
        auto *tangentsBuffer = (glm::vec3 *)rtcGetGeometryBufferData(
            geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
            (uint32_t)VertexAttributeSlot::SLOT_TANGENT);
        auto *bitangentsBuffer = (glm::vec3 *)rtcGetGeometryBufferData(
            geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
            (uint32_t)VertexAttributeSlot::SLOT_BITANGENT);

        for (auto i = 0; i < numVertices; i++) {
            positionsBuffer[i] =
                glm::vec3(transform * glm::vec4(srcPositions[i], 1.0f));
            normalsBuffer[i] = glm::normalize(
                glm::vec3(inverseTranspose * glm::vec4(srcNormals[i], 0.0f)));
            texCoords0Buffer[i] = srcTexcoords0[i];
            tangentsBuffer[i] = glm::normalize(glm::vec3(
                transform * glm::vec4(glm::vec3(srcTangents[i]), 0.0f)));
            bitangentsBuffer[i] = glm::normalize(glm::vec3(
                inverseTranspose *
                glm::vec4(
                    (glm::cross(srcNormals[i], glm::vec3(srcTangents[i])) *
                     srcTangents[i].w),
                    0.0f)));
        }

        rtcUpdateGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0);
        rtcUpdateGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                                SLOT_NORMAL);
        rtcUpdateGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                                SLOT_TEXCOORD_0);
        rtcUpdateGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                                SLOT_TANGENT);
        rtcUpdateGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                                SLOT_BITANGENT);

        rtcCommitGeometry(geom);
    }
    return geometries;
};

void Geometry::setUserData(void *data) const {
    rtcSetGeometryUserData(geom, data);
}

void *Geometry::getUserData() const { return rtcGetGeometryUserData(geom); }

void Geometry::setMaterial(std::shared_ptr<Material> material) {
    auto p = this->primitive->clone();
    p->setMaterial(material);
    this->primitive = p;
    this->setUserData(material.get());
}

void Geometry::release(RTCScene scene) const {
    rtcDetachGeometry(scene, geomID);
    rtcReleaseGeometry(geom);
}

uint32_t Geometry::getGeomId() const { return this->geomID; }

RTCGeometry Geometry::getGeom() const { return this->geom; }

ConstantPMaterial Geometry::getMaterial() const {
    return this->primitive->getMaterial();
}

std::shared_ptr<Geometry> Geometry::clone() const {
    auto p = std::shared_ptr<Geometry>(new Geometry);
    p->geom = this->geom;
    p->geomID = this->geomID;
    p->nodes = this->nodes;
    p->primitive = this->primitive;
    return p;
}

ConstantPModel loadSphere(ConstantPMaterial material, uint32_t widthSegments,
                          uint32_t heightSegments, const glm::mat4 transform) {
    const auto radius = 1.0f;
    auto index = 0;
    auto grid = std::vector<std::vector<uint32_t>>();

    auto position = glm::vec3();
    auto normal = glm::vec3();

    auto srcTriangles = std::vector<glm::u32vec3>();
    auto srcPositions = std::vector<glm::vec3>();
    auto srcNormals = std::vector<glm::vec3>();
    auto srcTexcoords0 = std::vector<glm::vec2>();

    const auto inverseTranspose = glm::inverseTranspose(transform);

    for (auto iy = 0; iy <= (int32_t)heightSegments; iy++) {
        auto verticesRow = std::vector<uint32_t>();

        auto v = (float)iy / (float)heightSegments;

        auto uOffset = 0.0f;

        if (iy == 0) {
            uOffset = 0.5f / (float)widthSegments;

        } else if (iy == heightSegments) {
            uOffset = -0.5f / (float)widthSegments;
        }

        for (auto ix = 0; ix <= (int32_t)widthSegments; ix++) {
            auto u = (float)ix / widthSegments;
            position.x = -radius * std::cos(u * 2 * M_PI) * std::sin(v * M_PI);
            position.y = radius * std::cos(v * M_PI);
            position.z = radius * std::sin(u * 2 * M_PI) * std::sin(v * M_PI);
            srcPositions.push_back(position);
            normal = glm::normalize(position);
            srcNormals.push_back(normal);
            srcTexcoords0.push_back(glm::vec2(u + uOffset, 1.0f - v));
            verticesRow.push_back(index++);
        }
        grid.push_back(verticesRow);
    }
    for (auto iy = 0; iy < (int32_t)heightSegments; iy++) {
        for (auto ix = 0; ix < (int32_t)widthSegments; ix++) {
            auto a = grid[iy][ix + 1];
            auto b = grid[iy][ix];
            auto c = grid[iy + 1][ix];
            auto d = grid[iy + 1][ix + 1];

            if (iy != 0) srcTriangles.push_back(glm::u32vec3(a, b, d));
            if (iy != heightSegments - 1)
                srcTriangles.push_back(glm::u32vec3(b, c, d));
        }
    }

    const auto numVertices = srcPositions.size();
    const auto numFaces = srcTriangles.size();

    auto srcTangents = TangentGenerator::generate(
        static_cast<int32_t>(numFaces), static_cast<int32_t>(numVertices),
        srcTriangles.data(), srcPositions.data(), srcNormals.data(),
        srcTexcoords0.data());

    auto primitive = PPrimitive(new Primitive());

    primitive->setMaterial(material);
    primitive->setPositions(std::move(srcPositions));
    primitive->setNormals(std::move(srcNormals));
    primitive->setTexCoords0(std::move(srcTexcoords0));
    primitive->setTangents(std::move(srcTangents));
    primitive->setTriangles(std::move(srcTriangles));

    auto mesh = PMesh(new Mesh());
    mesh->addPrimitive(primitive);

    auto node = PNode(new Node(0));
    node->setMatrix(transform);
    node->setMesh(mesh);

    auto model = std::make_shared<Model>();

    model->addMaterial(material);
    model->addMesh(mesh);
    model->addNode(node);

    auto scene = PScene(new Scene());
    scene->addNode(node);
    model->addScene(scene);
    model->setScene(scene);

    return model;
}

/* adds a cube to the scene */
ConstantPModel loadCube(ConstantPMaterial material, glm::mat4 transform) {
    /* create a triangulated cube with 12 triangles and 8 vertices */
    const auto numFaces = 12;
    const auto numVertices = 24;

    /* set vertices and vertex colors */
    std::vector<glm::vec3> srcPositions(numVertices);
    srcPositions[0] = glm::vec3(1, 1, -1);
    srcPositions[1] = glm::vec3(1, 1, 1);
    srcPositions[2] = glm::vec3(1, -1, 1);
    srcPositions[3] = glm::vec3(1, -1, -1);
    srcPositions[4] = glm::vec3(-1, 1, 1);
    srcPositions[5] = glm::vec3(-1, 1, -1);
    srcPositions[6] = glm::vec3(-1, -1, -1);
    srcPositions[7] = glm::vec3(-1, -1, 1);
    srcPositions[8] = glm::vec3(-1, 1, 1);
    srcPositions[9] = glm::vec3(1, 1, 1);
    srcPositions[10] = glm::vec3(1, 1, -1);
    srcPositions[11] = glm::vec3(-1, 1, -1);
    srcPositions[12] = glm::vec3(-1, -1, -1);
    srcPositions[13] = glm::vec3(1, -1, -1);
    srcPositions[14] = glm::vec3(1, -1, 1);
    srcPositions[15] = glm::vec3(-1, -1, 1);
    srcPositions[16] = glm::vec3(1, 1, 1);
    srcPositions[17] = glm::vec3(-1, 1, 1);
    srcPositions[18] = glm::vec3(-1, -1, 1);
    srcPositions[19] = glm::vec3(1, -1, 1);
    srcPositions[20] = glm::vec3(-1, 1, -1);
    srcPositions[21] = glm::vec3(1, 1, -1);
    srcPositions[22] = glm::vec3(1, -1, -1);
    srcPositions[23] = glm::vec3(-1, -1, -1);

    /* set triangles and face colors */
    std::vector<glm::u32vec3> srcTriangles(numFaces);
    srcTriangles[0] = glm::u32vec3(0, 1, 2);
    srcTriangles[1] = glm::u32vec3(0, 2, 3);
    srcTriangles[2] = glm::u32vec3(4, 5, 6);
    srcTriangles[3] = glm::u32vec3(4, 6, 7);
    srcTriangles[4] = glm::u32vec3(8, 9, 10);
    srcTriangles[5] = glm::u32vec3(8, 10, 11);
    srcTriangles[6] = glm::u32vec3(12, 13, 14);
    srcTriangles[7] = glm::u32vec3(12, 14, 15);
    srcTriangles[8] = glm::u32vec3(16, 17, 18);
    srcTriangles[9] = glm::u32vec3(16, 18, 19);
    srcTriangles[10] = glm::u32vec3(20, 21, 22);
    srcTriangles[11] = glm::u32vec3(20, 22, 23);

    std::vector<glm::vec3> srcNormals(numVertices);
    srcNormals[0] = glm::vec3(1, 0, 0);
    srcNormals[1] = glm::vec3(1, 0, 0);
    srcNormals[2] = glm::vec3(1, 0, 0);
    srcNormals[3] = glm::vec3(1, 0, 0);
    srcNormals[4] = glm::vec3(-1, 0, 0);
    srcNormals[5] = glm::vec3(-1, 0, 0);
    srcNormals[6] = glm::vec3(-1, 0, 0);
    srcNormals[7] = glm::vec3(-1, 0, 0);
    srcNormals[8] = glm::vec3(0, 1, 0);
    srcNormals[9] = glm::vec3(0, 1, 0);
    srcNormals[10] = glm::vec3(0, 1, 0);
    srcNormals[11] = glm::vec3(0, 1, 0);
    srcNormals[12] = glm::vec3(0, -1, 0);
    srcNormals[13] = glm::vec3(0, -1, 0);
    srcNormals[14] = glm::vec3(0, -1, 0);
    srcNormals[15] = glm::vec3(0, -1, 0);
    srcNormals[16] = glm::vec3(0, 0, 1);
    srcNormals[17] = glm::vec3(0, 0, 1);
    srcNormals[18] = glm::vec3(0, 0, 1);
    srcNormals[19] = glm::vec3(0, 0, 1);
    srcNormals[20] = glm::vec3(0, 0, -1);
    srcNormals[21] = glm::vec3(0, 0, -1);
    srcNormals[22] = glm::vec3(0, 0, -1);
    srcNormals[23] = glm::vec3(0, 0, -1);

    std::vector<glm::vec2> srcTexcoords0(numVertices);
    srcTexcoords0[0] = glm::vec2(1, 0);
    srcTexcoords0[1] = glm::vec2(0, 0);
    srcTexcoords0[2] = glm::vec2(0, 1);
    srcTexcoords0[3] = glm::vec2(1, 1);
    srcTexcoords0[4] = glm::vec2(1, 0);
    srcTexcoords0[5] = glm::vec2(0, 0);
    srcTexcoords0[6] = glm::vec2(0, 1);
    srcTexcoords0[7] = glm::vec2(1, 1);
    srcTexcoords0[8] = glm::vec2(1, 0);
    srcTexcoords0[9] = glm::vec2(0, 0);
    srcTexcoords0[10] = glm::vec2(0, 1);
    srcTexcoords0[11] = glm::vec2(1, 1);
    srcTexcoords0[12] = glm::vec2(1, 0);
    srcTexcoords0[13] = glm::vec2(0, 0);
    srcTexcoords0[14] = glm::vec2(0, 1);
    srcTexcoords0[15] = glm::vec2(1, 1);
    srcTexcoords0[16] = glm::vec2(1, 0);
    srcTexcoords0[17] = glm::vec2(0, 0);
    srcTexcoords0[18] = glm::vec2(0, 1);
    srcTexcoords0[19] = glm::vec2(1, 1);
    srcTexcoords0[20] = glm::vec2(1, 0);
    srcTexcoords0[21] = glm::vec2(0, 0);
    srcTexcoords0[22] = glm::vec2(0, 1);
    srcTexcoords0[23] = glm::vec2(1, 1);

    auto srcTangents = TangentGenerator::generate(
        numFaces, numVertices, srcTriangles.data(), srcPositions.data(),
        srcNormals.data(), srcTexcoords0.data());

    auto primitive = PPrimitive(new Primitive());

    primitive->setMaterial(material);
    primitive->setPositions(std::move(srcPositions));
    primitive->setNormals(std::move(srcNormals));
    primitive->setTexCoords0(std::move(srcTexcoords0));
    primitive->setTangents(std::move(srcTangents));
    primitive->setTriangles(std::move(srcTriangles));

    auto mesh = PMesh(new Mesh());
    mesh->addPrimitive(primitive);

    auto node = PNode(new Node(0));
    node->setMatrix(transform);
    node->setMesh(mesh);

    auto model = std::make_shared<Model>();
    model->addMaterial(material);
    model->addMesh(mesh);
    model->addNode(node);

    auto scene = PScene(new Scene());
    scene->addNode(node);
    model->addScene(scene);
    model->setScene(scene);

    return model;
}

/* adds a ground plane to the scene */
ConstantPModel loadPlane(ConstantPMaterial material,
                         const glm::mat4 transform) {
    const auto numFaces = 2;
    const auto numVertices = 4;

    /* set vertices */
    std::vector<glm::vec3> srcPositions(numVertices);
    srcPositions[0] = glm::vec3(-1, 0, -1);
    srcPositions[1] = glm::vec3(-1, 0, +1);
    srcPositions[2] = glm::vec3(+1, 0, -1);
    srcPositions[3] = glm::vec3(+1, 0, +1);

    /* set triangles */
    std::vector<glm::u32vec3> srcTriangles(numFaces);
    srcTriangles[0] = glm::u32vec3(0, 1, 2);
    srcTriangles[1] = glm::u32vec3(1, 3, 2);

    const auto inverseTranspose = glm::transpose(glm::inverse(transform));
    std::vector<glm::vec3> srcNormals(numVertices);
    srcNormals[0] = glm::vec3(0.0f, 1.0f, 0.0f);
    srcNormals[1] = glm::vec3(0.0f, 1.0f, 0.0f);
    srcNormals[2] = glm::vec3(0.0f, 1.0f, 0.0f);
    srcNormals[3] = glm::vec3(0.0f, 1.0f, 0.0f);

    std::vector<glm::vec2> srcTexcoords0(numVertices);
    srcTexcoords0[0] = glm::vec2(0.0f, 0.0f);
    srcTexcoords0[1] = glm::vec2(0.0f, 1.0f);
    srcTexcoords0[2] = glm::vec2(1.0f, 0.0f);
    srcTexcoords0[3] = glm::vec2(1.0f, 1.0f);

    auto srcTangents = TangentGenerator::generate(
        numFaces, numVertices, srcTriangles.data(), srcPositions.data(),
        srcNormals.data(), srcTexcoords0.data());

    auto primitive = PPrimitive(new Primitive());
    primitive->setMaterial(material);
    primitive->setPositions(std::move(srcPositions));
    primitive->setNormals(std::move(srcNormals));
    primitive->setTexCoords0(std::move(srcTexcoords0));
    primitive->setTangents(std::move(srcTangents));
    primitive->setTriangles(std::move(srcTriangles));

    auto mesh = PMesh(new Mesh());
    mesh->addPrimitive(primitive);

    auto node = PNode(new Node(0));
    node->setMatrix(transform);
    node->setMesh(mesh);

    auto model = std::make_shared<Model>();
    model->addMaterial(material);
    model->addMesh(mesh);
    model->addNode(node);

    auto scene = PScene(new Scene());
    scene->addNode(node);
    model->addScene(scene);
    model->setScene(scene);

    return model;
}

ConstantPModel loadObjModel(const std::string &filename) {
    auto model = PModel(new Model());

    auto baseDir = std::filesystem::path(filename).parent_path().string();
    if (baseDir.empty()) {
        baseDir = ".";
    }
#ifdef _WIN32
    baseDir += "\\";
#else
    baseDir += "/";
#endif

    tinyobj::attrib_t srcAttrib;
    std::vector<tinyobj::shape_t> srcShapes;
    std::vector<tinyobj::material_t> srcMaterials;
    std::string warn;
    std::string err;
    auto ret = tinyobj::LoadObj(&srcAttrib, &srcShapes, &srcMaterials, &warn,
                                &err, filename.c_str(), baseDir.c_str());
    if (!ret) {
        return model;
    }

    std::map<std::string, std::shared_ptr<const Texture>> textures;

    // Load diffuse textures
    for (size_t m = 0; m < srcMaterials.size(); m++) {
        tinyobj::material_t *mp = &srcMaterials[m];

        if (mp->diffuse_texname.length() > 0) {
            // Only load the texture if it is not already loaded
            if (textures.find(mp->diffuse_texname) == textures.end()) {
                int width, height;
                int components;

                const std::string &texFileName = mp->diffuse_texname;

                stbi_ldr_to_hdr_gamma(1.0f);
                stbi_ldr_to_hdr_scale(1.0f);
                auto ret = stbi_loadf(texFileName.c_str(), &width, &height,
                                      &components, 4);
                if (ret) {
                    const size_t n = width * height;
                    auto image = std::shared_ptr<glm::vec4[]>(new glm::vec4[n]);
                    memcpy(image.get(), ret, n * sizeof(glm::vec4));
                    stbi_image_free(ret);

                    int32_t wrapS = 0;
                    int32_t wrapT = 0;

                    auto texture = std::shared_ptr<const Texture>(
                        new Texture(image, width, height, wrapS, wrapT));
                    textures.insert(
                        std::make_pair(mp->diffuse_texname, texture));
                    model->addTexture(texture);
                }
            }
        }

        auto material = ConstantPMaterial(new Material(
            REFLECTION,
            glm::vec4(mp->diffuse[0], mp->diffuse[1], mp->diffuse[2], 1.0f),
            textures.find(mp->diffuse_texname) == textures.end()
                ? textures[mp->diffuse_texname]
                : nullptr,
            textures.find(mp->normal_texname) == textures.end()
                ? textures[mp->normal_texname]
                : nullptr,
            mp->roughness, mp->metallic, nullptr,
            glm::vec3(mp->emission[0], mp->emission[1], mp->emission[2]),
            textures.find(mp->emissive_texname) == textures.end()
                ? textures[mp->emissive_texname]
                : nullptr));
        model->addMaterial(material);
    }

    // Append `default` material
    model->addMaterial(ConstantPMaterial(new Material()));

    for (size_t s = 0; s < srcShapes.size(); s++) {
        std::vector<glm::u32vec3> srcTriangles;
        std::vector<glm::vec3> srcPositions;
        std::vector<glm::vec3> srcNormals;
        std::vector<glm::vec2> srcTexCoords0;

        // Check for smoothing group and compute smoothing normals
        std::map<int, glm::vec3> smoothVertexNormals;
        if (hasSmoothingGroup(srcShapes[s])) {
            computeSmoothingNormals(srcAttrib, srcShapes[s],
                                    smoothVertexNormals);
        }

        for (size_t f = 0; f < srcShapes[s].mesh.indices.size() / 3; f++) {
            tinyobj::index_t idx0 = srcShapes[s].mesh.indices[3 * f + 0];
            tinyobj::index_t idx1 = srcShapes[s].mesh.indices[3 * f + 1];
            tinyobj::index_t idx2 = srcShapes[s].mesh.indices[3 * f + 2];

            float tc[3][2];
            if (srcAttrib.texcoords.size() > 0) {
                if ((idx0.texcoord_index < 0) || (idx1.texcoord_index < 0) ||
                    (idx2.texcoord_index < 0)) {
                    // face does not contain valid uv index.
                    tc[0][0] = 0.0f;
                    tc[0][1] = 0.0f;
                    tc[1][0] = 0.0f;
                    tc[1][1] = 0.0f;
                    tc[2][0] = 0.0f;
                    tc[2][1] = 0.0f;
                } else {
                    assert(srcAttrib.texcoords.size() >
                           size_t(2 * idx0.texcoord_index + 1));
                    assert(srcAttrib.texcoords.size() >
                           size_t(2 * idx1.texcoord_index + 1));
                    assert(srcAttrib.texcoords.size() >
                           size_t(2 * idx2.texcoord_index + 1));

                    // Flip Y coord.
                    tc[0][0] = srcAttrib.texcoords[2 * idx0.texcoord_index];
                    tc[0][1] =
                        1.0f - srcAttrib.texcoords[2 * idx0.texcoord_index + 1];
                    tc[1][0] = srcAttrib.texcoords[2 * idx1.texcoord_index];
                    tc[1][1] =
                        1.0f - srcAttrib.texcoords[2 * idx1.texcoord_index + 1];
                    tc[2][0] = srcAttrib.texcoords[2 * idx2.texcoord_index];
                    tc[2][1] =
                        1.0f - srcAttrib.texcoords[2 * idx2.texcoord_index + 1];
                }
            } else {
                tc[0][0] = 0.0f;
                tc[0][1] = 0.0f;
                tc[1][0] = 0.0f;
                tc[1][1] = 0.0f;
                tc[2][0] = 0.0f;
                tc[2][1] = 0.0f;
            }

            float v[3][3];
            for (int k = 0; k < 3; k++) {
                int f0 = idx0.vertex_index;
                int f1 = idx1.vertex_index;
                int f2 = idx2.vertex_index;
                assert(f0 >= 0);
                assert(f1 >= 0);
                assert(f2 >= 0);

                v[0][k] = srcAttrib.vertices[3 * f0 + k];
                v[1][k] = srcAttrib.vertices[3 * f1 + k];
                v[2][k] = srcAttrib.vertices[3 * f2 + k];
            }

            float n[3][3];
            {
                bool invalid_normal_index = false;
                if (srcAttrib.normals.size() > 0) {
                    int nf0 = idx0.normal_index;
                    int nf1 = idx1.normal_index;
                    int nf2 = idx2.normal_index;

                    if ((nf0 < 0) || (nf1 < 0) || (nf2 < 0)) {
                        // normal index is missing from this face.
                        invalid_normal_index = true;
                    } else {
                        for (int k = 0; k < 3; k++) {
                            assert(size_t(3 * nf0 + k) <
                                   srcAttrib.normals.size());
                            assert(size_t(3 * nf1 + k) <
                                   srcAttrib.normals.size());
                            assert(size_t(3 * nf2 + k) <
                                   srcAttrib.normals.size());
                            n[0][k] = srcAttrib.normals[3 * nf0 + k];
                            n[1][k] = srcAttrib.normals[3 * nf1 + k];
                            n[2][k] = srcAttrib.normals[3 * nf2 + k];
                        }
                    }
                } else {
                    invalid_normal_index = true;
                }

                if (invalid_normal_index && !smoothVertexNormals.empty()) {
                    // Use smoothing normals
                    int f0 = idx0.vertex_index;
                    int f1 = idx1.vertex_index;
                    int f2 = idx2.vertex_index;

                    if (f0 >= 0 && f1 >= 0 && f2 >= 0) {
                        n[0][0] = smoothVertexNormals[f0][0];
                        n[0][1] = smoothVertexNormals[f0][1];
                        n[0][2] = smoothVertexNormals[f0][2];

                        n[1][0] = smoothVertexNormals[f1][0];
                        n[1][1] = smoothVertexNormals[f1][1];
                        n[1][2] = smoothVertexNormals[f1][2];

                        n[2][0] = smoothVertexNormals[f2][0];
                        n[2][1] = smoothVertexNormals[f2][1];
                        n[2][2] = smoothVertexNormals[f2][2];

                        invalid_normal_index = false;
                    }
                }

                if (invalid_normal_index) {
                    // compute geometric normal
                    calcNormal(n[0], v[0], v[1], v[2]);
                    n[1][0] = n[0][0];
                    n[1][1] = n[0][1];
                    n[1][2] = n[0][2];
                    n[2][0] = n[0][0];
                    n[2][1] = n[0][1];
                    n[2][2] = n[0][2];
                }
            }

            for (int k = 0; k < 3; k++) {
                srcPositions.push_back(glm::vec3(v[k][0], v[k][1], v[k][2]));
                srcNormals.push_back(glm::vec3(n[k][0], n[k][1], n[k][2]));
                srcTexCoords0.push_back(glm::vec2(tc[k][0], tc[k][1]));
            }
        }

        auto materialId = srcShapes[s].mesh.material_ids[0];

        if (materialId < 0 || materialId >= model->getMaterials().size()) {
            materialId = static_cast<int32_t>(model->getMaterials().size()) - 1;
        }

        auto numFaces = srcPositions.size() / 3;

        for (auto i = 0; i < numFaces; i++) {
            srcTriangles.push_back(
                glm::u32vec3(i * 3 + 0, i * 3 + 1, i * 3 + 2));
        }

        const auto material = model->getMaterials()[materialId];

        const auto numVertices = srcPositions.size();

        auto srcTangents = TangentGenerator::generate(
            static_cast<int32_t>(numFaces), static_cast<int32_t>(numVertices),
            srcTriangles.data(), srcPositions.data(), srcNormals.data(),
            srcTexCoords0.data());

        auto primitive = PPrimitive(new Primitive());
        primitive->setMaterial(material);

        auto mesh = PMesh(new Mesh());
        primitive->setPositions(std::move(srcPositions));
        primitive->setNormals(std::move(srcNormals));
        primitive->setTexCoords0(std::move(srcTexCoords0));
        primitive->setTangents(std::move(srcTangents));
        primitive->setTriangles(std::move(srcTriangles));
        mesh->addPrimitive(std::move(primitive));

        auto node = PNode(new Node(model->getNodes().size()));
        node->setMatrix(glm::mat4(1.0f));
        node->setMesh(mesh);

        model->addMesh(mesh);
        model->addNode(node);
    }

    auto scene = PScene(new Scene);
    for (auto node : model->getNodes()) {
        scene->addNode(node);
    }

    model->addScene(scene);
    model->setScene(scene);

    return model;
}

ConstantPModel loadGltfModel(const tinygltf::Model &gltfModel) {
    auto model = std::make_shared<Model>();

    for (auto it = gltfModel.textures.begin(); it != gltfModel.textures.end();
         it++) {
        const auto &gltfImage = gltfModel.images[it->source];
        const auto width = gltfImage.width;
        const auto height = gltfImage.height;
        const auto channels = gltfImage.component;
        const auto bits = gltfImage.bits;
        const size_t n = width * height;
        auto image = std::shared_ptr<glm::vec4[]>(new glm::vec4[n]);

        assert(bits == 8);
        assert(channels == 3 || channels == 4);

        const auto len = gltfImage.image.size() / channels;
        for (auto i = 0; i < len; i++) {
            const auto offset = i * channels;
            if (channels == 4) {
                image[i] = glm::vec4(gltfImage.image[offset + 0] / 255.0f,
                                     gltfImage.image[offset + 1] / 255.0f,
                                     gltfImage.image[offset + 2] / 255.0f,
                                     gltfImage.image[offset + 3] / 255.0f);
            }

            if (channels == 3) {
                image[i] =
                    glm::vec4(gltfImage.image[offset + 0] / 255.0f,
                              gltfImage.image[offset + 1] / 255.0f,
                              gltfImage.image[offset + 2] / 255.0f, 1.0f);
            }
        }

        int32_t wrapS = 0;
        int32_t wrapT = 0;
        if (it->sampler >= 0) {
            const auto &sampler = gltfModel.samplers[it->sampler];
            wrapS = sampler.wrapS;
            wrapT = sampler.wrapT;
        }

        model->addTexture(
            std::make_shared<Texture>(image, width, height, wrapS, wrapT));
    }

    for (auto it = gltfModel.materials.cbegin();
         it != gltfModel.materials.cend(); it++) {
        const auto &gltfMaterial = *it;
        const auto &images = model->getTextures();

        auto baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        auto baseColorTextureIndex = -1;
        auto roughnessFactor = 0.5f;
        auto metalnessFactor = 0.5f;
        auto emissiveFactor = glm::vec3(0.0f, 0.0f, 0.0f);
        auto emissiveTextureIndex = -1;
        auto normalTextureIndex = -1;
        auto metallicRoughnessTextureIndex = -1;

        const auto &pbr = gltfMaterial.pbrMetallicRoughness;
        switch (pbr.baseColorFactor.size()) {
            case 4:
                baseColorFactor.a = (float)pbr.baseColorFactor[3];
            case 3:
                baseColorFactor.b = (float)pbr.baseColorFactor[2];
                baseColorFactor.g = (float)pbr.baseColorFactor[1];
                baseColorFactor.r = (float)pbr.baseColorFactor[0];
                break;
        }
        baseColorTextureIndex = pbr.baseColorTexture.index;
        roughnessFactor = (float)pbr.roughnessFactor;
        metalnessFactor = (float)pbr.metallicFactor;

        if (gltfMaterial.emissiveFactor.size() == 3) {
            emissiveFactor = glm::vec3(gltfMaterial.emissiveFactor[0],
                                       gltfMaterial.emissiveFactor[1],
                                       gltfMaterial.emissiveFactor[2]);
        }
        emissiveTextureIndex = gltfMaterial.emissiveTexture.index;
        normalTextureIndex = gltfMaterial.normalTexture.index;
        metallicRoughnessTextureIndex = pbr.metallicRoughnessTexture.index;

        auto material = ConstantPMaterial(new Material(
            REFLECTION, baseColorFactor,
            baseColorTextureIndex >= 0 ? images[baseColorTextureIndex]
                                       : nullptr,
            normalTextureIndex >= 0 ? images[normalTextureIndex] : nullptr,
            roughnessFactor, metalnessFactor,
            metallicRoughnessTextureIndex >= 0
                ? images[metallicRoughnessTextureIndex]
                : nullptr,
            emissiveFactor,
            emissiveTextureIndex >= 0 ? images[emissiveTextureIndex]
                                      : nullptr));
        model->addMaterial(material);
    }

    for (auto it1 = gltfModel.meshes.cbegin(); it1 != gltfModel.meshes.cend();
         it1++) {
        auto mesh = std::make_shared<Mesh>();
        const auto &materials = model->getMaterials();
        const auto &gltfMesh = *it1;
        const auto &gltfPrimitives = gltfMesh.primitives;

        for (auto it2 = gltfPrimitives.cbegin(); it2 != gltfPrimitives.cend();
             it2++) {
            const auto &gltfPrimitive = *it2;
            assert(gltfPrimitive.indices >= 0);

            int mode = gltfPrimitive.mode;
            assert(mode == TINYGLTF_MODE_TRIANGLES);

            auto allSemantics = 0;

            const auto &indexAccessor =
                gltfModel.accessors[gltfPrimitive.indices];
            const auto &indexBufferView =
                gltfModel.bufferViews[indexAccessor.bufferView];
            const auto &indexBuffer = gltfModel.buffers[indexBufferView.buffer];

            assert(indexAccessor.type == TINYGLTF_TYPE_SCALAR);
            assert(indexAccessor.componentType ==
                       TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ||
                   indexAccessor.componentType ==
                       TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT);

            static const int32_t SEM_POSITION = 1 << 0;
            static const int32_t SEM_NORMAL = 1 << 1;
            static const int32_t SEM_TEXCOORD_0 = 1 << 2;
            static const int32_t SEM_TANGENT = 1 << 3;

            int32_t numFaces = static_cast<int32_t>(indexAccessor.count / 3);
            int32_t numVertices = 0;
            std::vector<glm::u32vec3> srcTriangles;
            std::vector<glm::vec3> srcNormals;
            std::vector<glm::vec3> srcPositions;
            std::vector<glm::vec2> srcTexCoords0;
            std::vector<glm::vec4> srcTangents;

            srcTriangles = std::vector<glm::u32vec3>(numFaces);

            for (auto i = 0; i < indexAccessor.count; i++) {
                const auto byteStride =
                    indexAccessor.ByteStride(indexBufferView);
                const auto byteOffset =
                    indexAccessor.byteOffset + indexBufferView.byteOffset;
                const auto componentType = indexAccessor.componentType;
                const auto normalized = indexAccessor.normalized;

                switch (indexAccessor.componentType) {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                        ((uint32_t *)srcTriangles.data())[i] =
                            *(uint16_t *)(gltfModel
                                              .buffers[indexBufferView.buffer]
                                              .data.data() +
                                          byteOffset + byteStride * i);
                    } break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                        ((uint32_t *)srcTriangles.data())[i] =
                            *(uint32_t *)(gltfModel
                                              .buffers[indexBufferView.buffer]
                                              .data.data() +
                                          byteOffset + byteStride * i);
                    } break;
                }
            }

            auto it(gltfPrimitive.attributes.begin());
            const auto itEnd(gltfPrimitive.attributes.end());
            for (; it != itEnd; it++) {
                const auto &accessor = gltfModel.accessors[it->second];
                const auto bufferView =
                    gltfModel.bufferViews[accessor.bufferView];

                int size = 1;
                switch (accessor.type) {
                    case TINYGLTF_TYPE_SCALAR:
                        size = 1;
                        break;
                    case TINYGLTF_TYPE_VEC2:
                        size = 2;
                        break;
                    case TINYGLTF_TYPE_VEC3:
                        size = 3;
                        break;
                    case TINYGLTF_TYPE_VEC4:
                        size = 4;
                        break;
                }

                int semantics = 0;
                if (it->first.compare("POSITION") == 0) {
                    semantics = SEM_POSITION;
                } else if (it->first.compare("NORMAL") == 0) {
                    semantics = SEM_NORMAL;
                } else if (it->first.compare("TEXCOORD_0") == 0) {
                    semantics = SEM_TEXCOORD_0;
                } else if (it->first.compare("TANGENT") == 0) {
                    semantics = SEM_TANGENT;
                }

                allSemantics |= semantics;

                // it->first would be "POSITION", "NORMAL", "TEXCOORD_0", ...
                if (semantics > 0) {
                    // Compute byteStride from Accessor + BufferView
                    // combination.
                    const auto byteStride = accessor.ByteStride(bufferView);
                    const auto byteOffset =
                        accessor.byteOffset + bufferView.byteOffset;
                    const auto componentType = accessor.componentType;
                    const auto normalized = accessor.normalized;

                    switch (accessor.componentType) {
                        case TINYGLTF_COMPONENT_TYPE_FLOAT: {
                            float *dstBuffer = nullptr;

                            switch (semantics) {
                                case SEM_POSITION: {
                                    assert(size == 3);

                                    srcPositions =
                                        std::vector<glm::vec3>(accessor.count);
                                    numVertices =
                                        static_cast<int32_t>(accessor.count);
                                    dstBuffer = (float *)srcPositions.data();
                                } break;
                                case SEM_NORMAL: {
                                    assert(size == 3);
                                    srcNormals =
                                        std::vector<glm::vec3>(accessor.count);
                                    dstBuffer = (float *)srcNormals.data();
                                } break;
                                case SEM_TEXCOORD_0: {
                                    assert(size == 2);
                                    srcTexCoords0 =
                                        std::vector<glm::vec2>(accessor.count);
                                    dstBuffer = (float *)srcTexCoords0.data();
                                } break;
                                case SEM_TANGENT: {
                                    assert(size == 4);
                                    srcTangents =
                                        std::vector<glm::vec4>(accessor.count);
                                    dstBuffer = (float *)srcTangents.data();
                                } break;
                            }

                            for (auto i = 0; i < accessor.count; i++) {
                                const auto srcBuffer =
                                    (const float
                                         *)(gltfModel.buffers[bufferView.buffer]
                                                .data.data() +
                                            byteOffset + byteStride * i);

                                switch (size) {
                                    case 2: {
                                        dstBuffer[size * i + 0] = srcBuffer[0];
                                        dstBuffer[size * i + 1] = srcBuffer[1];
                                    } break;
                                    case 3: {
                                        dstBuffer[size * i + 0] = srcBuffer[0];
                                        dstBuffer[size * i + 1] = srcBuffer[1];
                                        dstBuffer[size * i + 2] = srcBuffer[2];
                                    } break;
                                    case 4: {
                                        dstBuffer[size * i + 0] = srcBuffer[0];
                                        dstBuffer[size * i + 1] = srcBuffer[1];
                                        dstBuffer[size * i + 2] = srcBuffer[2];
                                        dstBuffer[size * i + 3] = srcBuffer[3];
                                    } break;
                                }
                            }
                        } break;
                    }
                }
            }

            assert(numVertices > 0);

            if (!(allSemantics & SEM_NORMAL)) {
                srcNormals = std::vector<glm::vec3>(numVertices);

                // TODO: COMPUTE NORMALs
                for (auto i = 0; i < numVertices; i++) {
                    srcNormals[i] = glm::vec3(0.0f, 0.0f, 1.0f);
                }
            }

            if (!(allSemantics & SEM_TEXCOORD_0)) {
                srcTexCoords0 = std::vector<glm::vec2>(numVertices);

                // TODO: COMPUTE UVs
                for (auto i = 0; i < numVertices; i++) {
                    srcTexCoords0[i] = glm::vec2(0.0f, 0.0f);
                }
            }

            if (!(allSemantics & SEM_TANGENT)) {
                srcTangents = TangentGenerator::generate(
                    numFaces, numVertices,
                    (const glm::u32vec3 *)srcTriangles.data(),
                    srcPositions.data(), srcNormals.data(),
                    srcTexCoords0.data());
            }

            {
                auto material = gltfPrimitive.material < 0
                                    ? PMaterial(new Material())
                                    : materials[gltfPrimitive.material];
                auto primitive = PPrimitive(new Primitive());
                primitive->setMaterial(std::move(material));
                primitive->setPositions(std::move(srcPositions));
                primitive->setNormals(std::move(srcNormals));
                primitive->setTexCoords0(std::move(srcTexCoords0));
                primitive->setTangents(std::move(srcTangents));
                primitive->setTriangles(std::move(srcTriangles));
                mesh->addPrimitive(primitive);
            }
        }

        model->addMesh(mesh);
    }

    std::vector<std::shared_ptr<Node>> nodes;
    for (auto it = gltfModel.nodes.cbegin(); it != gltfModel.nodes.cend();
         it++) {
        const auto &gltfNode = *it;
        auto node = PNode(new Node(nodes.size()));

        glm::mat4 matrix(1.0f);
        if (gltfNode.matrix.size() == 16) {
            // Use `matrix' attribute
            matrix = glm::make_mat4(gltfNode.matrix.data());
        } else {
            // Assume Trans x Rotate x Scale order
            if (gltfNode.scale.size() == 3) {
                const auto &s = gltfNode.scale;
                matrix = glm::scale(
                             glm::vec3((float)s[0], (float)s[1], (float)s[2])) *
                         matrix;
            }

            if (gltfNode.rotation.size() == 4) {
                const auto &r = gltfNode.rotation;
                matrix =
                    static_cast<glm::mat4>(glm::quat(
                        (float)r[3], (float)r[0], (float)r[1], (float)r[2])) *
                    matrix;
            }

            if (gltfNode.translation.size() == 3) {
                const auto &t = gltfNode.translation;
                matrix = glm::translate(
                             glm::vec3((float)t[0], (float)t[1], (float)t[2])) *
                         matrix;
            }
        }

        if (gltfNode.mesh >= 0) {
            node->setMesh(model->getMeshs()[gltfNode.mesh]);
        }
        node->setMatrix(matrix);
        nodes.push_back(node);
    }

    for (auto parentIndex = 0; parentIndex < nodes.size(); parentIndex++) {
        const auto &children = gltfModel.nodes[parentIndex].children;
        for (auto it = children.cbegin(); it != children.cend(); it++) {
            const auto childIndex = *it;
            nodes[parentIndex]->addChild(nodes[childIndex]);
        }
    }

    for (auto node : nodes) {
        model->addNode(node);
    }

    for (const auto &gltfAnim : gltfModel.animations) {
        auto anim = PAnimation(new Animation(model->getAnimations().size()));

        anim->setName(gltfAnim.name);

        for (const auto &gltfSampler : gltfAnim.samplers) {
            auto sampler = PAnimationSampler(new AnimationSampler());

            const auto &outputAccessor =
                gltfModel.accessors[gltfSampler.output];
            assert(outputAccessor.componentType ==
                   TINYGLTF_COMPONENT_TYPE_FLOAT);
            const auto &outputBufferView =
                gltfModel.bufferViews[outputAccessor.bufferView];
            auto values = std::vector<float>();
            for (auto i = 0; i < outputAccessor.count; i++) {
                const auto p =
                    gltfModel.buffers[outputBufferView.buffer].data.data() +
                    outputAccessor.byteOffset + outputBufferView.byteOffset +
                    i * outputAccessor.ByteStride(outputBufferView);

                const auto *f = reinterpret_cast<const float *>(p);

                assert(outputAccessor.type == TINYGLTF_TYPE_SCALAR ||
                       outputAccessor.type == TINYGLTF_TYPE_VEC3 ||
                       outputAccessor.type == TINYGLTF_TYPE_VEC4);

                switch (outputAccessor.type) {
                    case TINYGLTF_TYPE_VEC4:
                        values.push_back(f[0]);
                        values.push_back(f[1]);
                        values.push_back(f[2]);
                        values.push_back(f[3]);
                        break;
                    case TINYGLTF_TYPE_VEC3:
                        values.push_back(f[0]);
                        values.push_back(f[1]);
                        values.push_back(f[2]);
                        break;
                    case TINYGLTF_TYPE_SCALAR:
                        values.push_back(f[0]);
                        break;
                }
            }
            sampler->setValues(std::move(values));

            const auto &inputAccessor = gltfModel.accessors[gltfSampler.input];
            assert(inputAccessor.type == TINYGLTF_TYPE_SCALAR);
            assert(inputAccessor.componentType ==
                   TINYGLTF_COMPONENT_TYPE_FLOAT);
            const auto &inputBufferView =
                gltfModel.bufferViews[inputAccessor.bufferView];
            auto timeline = std::vector<float>();
            for (auto i = 0; i < inputAccessor.count; i++) {
                const auto p =
                    gltfModel.buffers[inputBufferView.buffer].data.data() +
                    inputAccessor.byteOffset + inputBufferView.byteOffset +
                    inputAccessor.ByteStride(inputBufferView) * i;
                auto *f = reinterpret_cast<const float *>(p);
                timeline.push_back(f[0]);
            }
            sampler->setTimeline(std::move(timeline));

            anim->addSampler(sampler);
        }

        for (const auto &gltfChannel : gltfAnim.channels) {
            auto channel = PAnimationChannel(new AnimationChannel());
            auto sampler = anim->getSamplers()[gltfChannel.sampler];

            channel->setSampler(sampler);
            channel->setTargetPath(gltfChannel.target_path);
            channel->setTargetNode(model->getNodes()[gltfChannel.target_node]);
            anim->addChannel(channel);
        }

        model->addAnimation(anim);
    }

    for (auto gltfScene : gltfModel.scenes) {
        auto scene = PScene(new Scene());
        for (auto gltfNode : gltfScene.nodes) {
            auto node = model->getNodes()[gltfNode];
            scene->addNode(node);
        }
        model->addScene(scene);
    }

    if (model->getScenes().empty()) {
        auto scene = PScene(new Scene());
        for (auto node : model->getNodes()) {
            scene->addNode(node);
        }
        model->addScene(scene);
    }

    if (gltfModel.defaultScene >= 0) {
        model->setScene(model->getScenes()[gltfModel.defaultScene]);
    } else {
        model->setScene(*model->getScenes().begin());
    }

    return model;
}
