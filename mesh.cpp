#include "mesh.hpp"
#include "ray_tracer.hpp"

#include <mikktspace.h>
#include <filesystem>

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
    const glm::uvec3 *faces;
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
                                     const glm::uvec3 *faces,
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

        return tangents;
    }

  public:
    static std::vector<glm::vec4> generate(int32_t numFaces,
                                           int32_t numVertices,
                                           const glm::uvec3 *faces,
                                           const glm::vec3 *positions,
                                           const glm::vec3 *normals,
                                           const glm::vec2 *uvs) {
        return TangentGenerator()._generate(numFaces, numVertices, faces,
                                            positions, normals, uvs);
    }
};
}  // namespace

ConstantPMesh addSphere(const RTCDevice device, const RTCScene scene,
                        ConstantPMaterial material,
                        uint32_t widthSegments, uint32_t heightSegments,
                        const glm::mat4 transform) {
    const auto radius = 1.0f;
    auto index = 0;
    auto grid = std::vector<std::vector<uint32_t>>();

    auto position = glm::vec3();
    auto normal = glm::vec3();

    auto srcIndices = std::vector<glm::uvec3>();
    auto srcPositions = std::vector<glm::vec3>();
    auto srcNormals = std::vector<glm::vec3>();
    auto srcTexcoords0 = std::vector<glm::vec2>();

    const auto inverseTranspose = glm::transpose(glm::inverse(transform));

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

            if (iy != 0) srcIndices.push_back(glm::uvec3(a, b, d));
            if (iy != heightSegments - 1)
                srcIndices.push_back(glm::uvec3(b, c, d));
        }
    }

    const auto numVertices = srcPositions.size();
    const auto numFaces = srcIndices.size();

    auto geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

    rtcSetGeometryVertexAttributeCount(
        geom, (uint32_t)VertexAttributeSlot::NUM_VERTEX_ATTRIBUTE_SLOTS);

    auto *positions = (glm::vec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3),
        srcPositions.size());
    for (auto i = 0; i < srcPositions.size(); i++) {
        positions[i] = srcPositions[i];
    }

    auto *indices = (glm::uvec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(glm::uvec3),
        srcIndices.size());
    for (auto i = 0; i < srcIndices.size(); i++) {
        indices[i] = srcIndices[i];
    }

    auto *normals = (glm::vec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        (uint32_t)VertexAttributeSlot::SLOT_NORMAL, RTC_FORMAT_FLOAT3,
        sizeof(glm::vec3), srcNormals.size());
    for (auto i = 0; i < srcNormals.size(); i++) {
        normals[i] = srcNormals[i];
    }

    auto *texCoords0 = (glm::vec2 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        (uint32_t)VertexAttributeSlot::SLOT_TEXCOORD_0, RTC_FORMAT_FLOAT2,
        sizeof(glm::vec2), srcTexcoords0.size());
    for (auto i = 0; i < srcTexcoords0.size(); i++) {
        texCoords0[i] = srcTexcoords0[i];
    }

    auto srcTangents = TangentGenerator::generate(
        static_cast<int32_t>(numFaces), static_cast<int32_t>(numVertices),
        indices, positions, normals, texCoords0);

    auto *tangents = (glm::vec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        (uint32_t)VertexAttributeSlot::SLOT_TANGENT, RTC_FORMAT_FLOAT3,
        sizeof(glm::vec3), srcTangents.size());

    auto *bitangents = (glm::vec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        (uint32_t)VertexAttributeSlot::SLOT_BITANGENT, RTC_FORMAT_FLOAT3,
        sizeof(glm::vec3), srcTangents.size());

    for (auto i = 0; i < numVertices; i++) {
        positions[i] = glm::vec3(transform * glm::vec4(positions[i], 1.0f));
        normals[i] = glm::normalize(
            glm::vec3(inverseTranspose * glm::vec4(normals[i], 0.0f)));
        tangents[i] = glm::normalize(
            glm::vec3(transform * glm::vec4(glm::vec3(srcTangents[i]), 0.0f)));
        bitangents[i] = glm::normalize(glm::cross(normals[i], tangents[i]) *
                                       srcTangents[i].w);
    }

    auto mesh = PMesh(new Mesh());
    rtcSetGeometryUserData(geom, (void *)mesh.get());
    rtcSetGeometryIntersectFilterFunction(geom, intersectionFilter);
    rtcCommitGeometry(geom);
    auto geomId = rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);

    mesh->setGeometryId(geomId);
    mesh->setMaterial(material);
    mesh->setWorldMatrix(transform);
    mesh->setWorldInverseTransposeMatrix(inverseTranspose);
    return mesh;
}

/* adds a cube to the scene */
ConstantPMesh addCube(RTCDevice device, RTCScene scene,
                      ConstantPMaterial material, glm::mat4 transform) {
    /* create a triangulated cube with 12 triangles and 8 vertices */
    auto geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

    rtcSetGeometryVertexAttributeCount(
        geom, (uint32_t)VertexAttributeSlot::NUM_VERTEX_ATTRIBUTE_SLOTS);

    const auto numFaces = 12;
    const auto numVertices = 24;

    /* set vertices and vertex colors */
    auto *positions = (glm::vec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3),
        numVertices);
    positions[0] = glm::vec3(1, 1, -1);
    positions[1] = glm::vec3(1, 1, 1);
    positions[2] = glm::vec3(1, -1, 1);
    positions[3] = glm::vec3(1, -1, -1);
    positions[4] = glm::vec3(-1, 1, 1);
    positions[5] = glm::vec3(-1, 1, -1);
    positions[6] = glm::vec3(-1, -1, -1);
    positions[7] = glm::vec3(-1, -1, 1);
    positions[8] = glm::vec3(-1, 1, 1);
    positions[9] = glm::vec3(1, 1, 1);
    positions[10] = glm::vec3(1, 1, -1);
    positions[11] = glm::vec3(-1, 1, -1);
    positions[12] = glm::vec3(-1, -1, -1);
    positions[13] = glm::vec3(1, -1, -1);
    positions[14] = glm::vec3(1, -1, 1);
    positions[15] = glm::vec3(-1, -1, 1);
    positions[16] = glm::vec3(1, 1, 1);
    positions[17] = glm::vec3(-1, 1, 1);
    positions[18] = glm::vec3(-1, -1, 1);
    positions[19] = glm::vec3(1, -1, 1);
    positions[20] = glm::vec3(-1, 1, -1);
    positions[21] = glm::vec3(1, 1, -1);
    positions[22] = glm::vec3(1, -1, -1);
    positions[23] = glm::vec3(-1, -1, -1);

    /* set triangles and face colors */
    glm::uvec3 *indices = (glm::uvec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(glm::uvec3),
        numFaces);
    indices[0] = glm::uvec3(0, 1, 2);
    indices[1] = glm::uvec3(0, 2, 3);
    indices[2] = glm::uvec3(4, 5, 6);
    indices[3] = glm::uvec3(4, 6, 7);
    indices[4] = glm::uvec3(8, 9, 10);
    indices[5] = glm::uvec3(8, 10, 11);
    indices[6] = glm::uvec3(12, 13, 14);
    indices[7] = glm::uvec3(12, 14, 15);
    indices[8] = glm::uvec3(16, 17, 18);
    indices[9] = glm::uvec3(16, 18, 19);
    indices[10] = glm::uvec3(20, 21, 22);
    indices[11] = glm::uvec3(20, 22, 23);

    glm::vec3 *normals = (glm::vec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        (uint32_t)VertexAttributeSlot::SLOT_NORMAL, RTC_FORMAT_FLOAT3,
        sizeof(glm::vec3), numVertices);
    const auto inverseTranspose = glm::transpose(glm::inverse(transform));
    normals[0] = glm::vec3(1, 0, 0);
    normals[1] = glm::vec3(1, 0, 0);
    normals[2] = glm::vec3(1, 0, 0);
    normals[3] = glm::vec3(1, 0, 0);
    normals[4] = glm::vec3(-1, 0, 0);
    normals[5] = glm::vec3(-1, 0, 0);
    normals[6] = glm::vec3(-1, 0, 0);
    normals[7] = glm::vec3(-1, 0, 0);
    normals[8] = glm::vec3(0, 1, 0);
    normals[9] = glm::vec3(0, 1, 0);
    normals[10] = glm::vec3(0, 1, 0);
    normals[11] = glm::vec3(0, 1, 0);
    normals[12] = glm::vec3(0, -1, 0);
    normals[13] = glm::vec3(0, -1, 0);
    normals[14] = glm::vec3(0, -1, 0);
    normals[15] = glm::vec3(0, -1, 0);
    normals[16] = glm::vec3(0, 0, 1);
    normals[17] = glm::vec3(0, 0, 1);
    normals[18] = glm::vec3(0, 0, 1);
    normals[19] = glm::vec3(0, 0, 1);
    normals[20] = glm::vec3(0, 0, -1);
    normals[21] = glm::vec3(0, 0, -1);
    normals[22] = glm::vec3(0, 0, -1);
    normals[23] = glm::vec3(0, 0, -1);

    glm::vec2 *texCoords0 = (glm::vec2 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        (uint32_t)VertexAttributeSlot::SLOT_TEXCOORD_0, RTC_FORMAT_FLOAT3,
        sizeof(glm::vec2), numVertices);
    texCoords0[0] = glm::vec2(1, 0);
    texCoords0[1] = glm::vec2(0, 0);
    texCoords0[2] = glm::vec2(0, 1);
    texCoords0[3] = glm::vec2(1, 1);
    texCoords0[4] = glm::vec2(1, 0);
    texCoords0[5] = glm::vec2(0, 0);
    texCoords0[6] = glm::vec2(0, 1);
    texCoords0[7] = glm::vec2(1, 1);
    texCoords0[8] = glm::vec2(1, 0);
    texCoords0[9] = glm::vec2(0, 0);
    texCoords0[10] = glm::vec2(0, 1);
    texCoords0[11] = glm::vec2(1, 1);
    texCoords0[12] = glm::vec2(1, 0);
    texCoords0[13] = glm::vec2(0, 0);
    texCoords0[14] = glm::vec2(0, 1);
    texCoords0[15] = glm::vec2(1, 1);
    texCoords0[16] = glm::vec2(1, 0);
    texCoords0[17] = glm::vec2(0, 0);
    texCoords0[18] = glm::vec2(0, 1);
    texCoords0[19] = glm::vec2(1, 1);
    texCoords0[20] = glm::vec2(1, 0);
    texCoords0[21] = glm::vec2(0, 0);
    texCoords0[22] = glm::vec2(0, 1);
    texCoords0[23] = glm::vec2(1, 1);

    auto srcTangents = TangentGenerator::generate(
        numFaces, numVertices, indices, positions, normals, texCoords0);

    auto *tangents = (glm::vec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        (uint32_t)VertexAttributeSlot::SLOT_TANGENT, RTC_FORMAT_FLOAT3,
        sizeof(glm::vec3), srcTangents.size());

    auto *bitangents = (glm::vec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        (uint32_t)VertexAttributeSlot::SLOT_BITANGENT, RTC_FORMAT_FLOAT3,
        sizeof(glm::vec3), srcTangents.size());

    for (auto i = 0; i < numVertices; i++) {
        positions[i] = glm::vec3(transform * glm::vec4(positions[i], 1.0f));
        normals[i] = glm::normalize(
            glm::vec3(inverseTranspose * glm::vec4(normals[i], 0.0f)));
        tangents[i] = glm::normalize(
            glm::vec3(transform * glm::vec4(glm::vec3(srcTangents[i]), 0.0f)));
        bitangents[i] = glm::normalize(glm::cross(normals[i], tangents[i]) *
                                       srcTangents[i].w);
    }

    auto mesh = PMesh(new Mesh());
    rtcSetGeometryUserData(geom, (void *)mesh.get());
    rtcSetGeometryIntersectFilterFunction(geom, intersectionFilter);
    rtcCommitGeometry(geom);
    auto geomId = rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);

    mesh->setGeometryId(geomId);
    mesh->setMaterial(material);
    mesh->setWorldMatrix(transform);
    mesh->setWorldInverseTransposeMatrix(inverseTranspose);
    return mesh;
}

/* adds a ground plane to the scene */
ConstantPMesh addGroundPlane(RTCDevice device, RTCScene scene,
                             ConstantPMaterial material,
                             const glm::mat4 transform) {
    /* create a triangulated plane with 2 triangles and 4 vertices */
    auto geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

    const auto numFaces = 2;
    const auto numVertices = 4;

    rtcSetGeometryVertexAttributeCount(
        geom, (uint32_t)VertexAttributeSlot::NUM_VERTEX_ATTRIBUTE_SLOTS);

    /* set vertices */
    auto *positions = (glm::vec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3),
        numVertices);
    positions[0] = glm::vec3(-1, 0, -1);
    positions[1] = glm::vec3(-1, 0, +1);
    positions[2] = glm::vec3(+1, 0, -1);
    positions[3] = glm::vec3(+1, 0, +1);

    /* set triangles */
    auto *indices = (glm::uvec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(glm::uvec3),
        numFaces);
    indices[0] = glm::uvec3(0, 1, 2);
    indices[1] = glm::uvec3(1, 3, 2);

    const auto inverseTranspose = glm::transpose(glm::inverse(transform));
    auto *normals = (glm::vec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        (uint32_t)VertexAttributeSlot::SLOT_NORMAL, RTC_FORMAT_FLOAT3,
        sizeof(glm::vec3), numVertices);
    normals[0] = glm::vec3(0.0f, 1.0f, 0.0f);
    normals[1] = glm::vec3(0.0f, 1.0f, 0.0f);
    normals[2] = glm::vec3(0.0f, 1.0f, 0.0f);
    normals[3] = glm::vec3(0.0f, 1.0f, 0.0f);

    auto *texCoords0 = (glm::vec2 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        (uint32_t)VertexAttributeSlot::SLOT_TEXCOORD_0, RTC_FORMAT_FLOAT2,
        sizeof(glm::vec2), 4);
    texCoords0[0] = glm::vec2(0.0f, 0.0f);
    texCoords0[1] = glm::vec2(0.0f, 1.0f);
    texCoords0[2] = glm::vec2(1.0f, 0.0f);
    texCoords0[3] = glm::vec2(1.0f, 1.0f);

    auto srcTangents = TangentGenerator::generate(
        numFaces, numVertices, indices, positions, normals, texCoords0);

    auto *tangents = (glm::vec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        (uint32_t)VertexAttributeSlot::SLOT_TANGENT, RTC_FORMAT_FLOAT3,
        sizeof(glm::vec3), srcTangents.size());

    auto *bitangents = (glm::vec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
        (uint32_t)VertexAttributeSlot::SLOT_BITANGENT, RTC_FORMAT_FLOAT3,
        sizeof(glm::vec3), srcTangents.size());

    for (auto i = 0; i < numVertices; i++) {
        positions[i] = glm::vec3(transform * glm::vec4(positions[i], 1.0f));
        normals[i] = glm::normalize(
            glm::vec3(inverseTranspose * glm::vec4(normals[i], 0.0f)));
        tangents[i] = glm::normalize(
            glm::vec3(transform * glm::vec4(glm::vec3(srcTangents[i]), 0.0f)));
        bitangents[i] = glm::normalize(glm::cross(normals[i], tangents[i]) *
                                       srcTangents[i].w);
    }

    auto mesh = PMesh(new Mesh());
    rtcSetGeometryUserData(geom, (void *)mesh.get());
    rtcSetGeometryIntersectFilterFunction(geom, intersectionFilter);
    rtcCommitGeometry(geom);
    auto geomId = rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);

    mesh->setGeometryId(geomId);
    mesh->setMaterial(material);
    mesh->setWorldMatrix(transform);
    mesh->setWorldInverseTransposeMatrix(inverseTranspose);

    return mesh;
}

void addMesh(const RTCDevice device, const RTCScene scene,
             const tinygltf::Model &model,
             const std::vector<std::shared_ptr<const Texture>> &images,
             const tinygltf::Mesh &gltfMesh, const glm::mat4 &transform,
             ConstantPMeshList &meshs) {
    auto inverseTranspose = glm::transpose(glm::inverse(transform));
    for (size_t i = 0; i < gltfMesh.primitives.size(); i++) {
        const auto &primitive = gltfMesh.primitives[i];

        if (primitive.indices < 0) return;

        auto gltfMaterial = model.materials[primitive.material];

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

        int mode = primitive.mode;
        assert(mode == TINYGLTF_MODE_TRIANGLES);

        auto geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
        rtcSetGeometryVertexAttributeCount(
            geom, (uint32_t)VertexAttributeSlot::NUM_VERTEX_ATTRIBUTE_SLOTS);
        auto allSemantics = 0;

        const auto &indexAccessor = model.accessors[primitive.indices];
        const auto &indexBufferView =
            model.bufferViews[indexAccessor.bufferView];
        const auto &indexBuffer = model.buffers[indexBufferView.buffer];

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
        uint32_t *triangles = nullptr;
        glm::vec3 *normals = nullptr;
        glm::vec3 *positions = nullptr;
        glm::vec2 *texCoords0 = nullptr;
        std::vector<glm::vec4> srcTangents;

        triangles = (uint32_t *)rtcSetNewGeometryBuffer(
            geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
            sizeof(uint32_t) * 3, numFaces);

        for (auto i = 0; i < indexAccessor.count; i++) {
            const auto byteStride = indexAccessor.ByteStride(indexBufferView);
            const auto byteOffset =
                indexAccessor.byteOffset + indexBufferView.byteOffset;
            const auto componentType = indexAccessor.componentType;
            const auto normalized = indexAccessor.normalized;

            switch (indexAccessor.componentType) {
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                    triangles[i] =
                        *(uint16_t *)(model.buffers[indexBufferView.buffer]
                                          .data.data() +
                                      byteOffset + byteStride * i);
                } break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                    triangles[i] =
                        *(uint32_t *)(model.buffers[indexBufferView.buffer]
                                          .data.data() +
                                      byteOffset + byteStride * i);
                } break;
            }
        }

        auto it(primitive.attributes.begin());
        const auto itEnd(primitive.attributes.end());
        for (; it != itEnd; it++) {
            const auto &accessor = model.accessors[it->second];
            const auto bufferView = model.bufferViews[accessor.bufferView];

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

                                dstBuffer = (float *)rtcSetNewGeometryBuffer(
                                    geom, RTC_BUFFER_TYPE_VERTEX, 0,
                                    (RTCFormat)((int)RTC_FORMAT_FLOAT + size -
                                                1),
                                    sizeof(float) * size, accessor.count);
                                positions = (glm::vec3 *)dstBuffer;
                                numVertices =
                                    static_cast<int32_t>(accessor.count);
                            } break;
                            case SEM_NORMAL: {
                                assert(size == 3);
                                dstBuffer = (float *)rtcSetNewGeometryBuffer(
                                    geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                                    (uint32_t)VertexAttributeSlot::SLOT_NORMAL,
                                    (RTCFormat)((int)RTC_FORMAT_FLOAT + size -
                                                1),
                                    sizeof(float) * size, accessor.count);
                                normals = (glm::vec3 *)dstBuffer;
                            } break;
                            case SEM_TEXCOORD_0: {
                                assert(size == 2);
                                dstBuffer = (float *)rtcSetNewGeometryBuffer(
                                    geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                                    (uint32_t)
                                        VertexAttributeSlot::SLOT_TEXCOORD_0,
                                    (RTCFormat)((int)RTC_FORMAT_FLOAT + size -
                                                1),
                                    sizeof(float) * size, accessor.count);
                                texCoords0 = (glm::vec2 *)dstBuffer;
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
                                (const float *)(model.buffers[bufferView.buffer]
                                                    .data.data() +
                                                byteOffset + byteStride * i);

                            switch (size) {
                                case 2: {
                                    const auto v =
                                        glm::vec2(srcBuffer[0], srcBuffer[1]);
                                    dstBuffer[size * i + 0] = v[0];
                                    dstBuffer[size * i + 1] = v[1];
                                } break;
                                case 3: {
                                    const auto v =
                                        glm::vec4(srcBuffer[0], srcBuffer[1],
                                                  srcBuffer[2], 1.0f);
                                    dstBuffer[size * i + 0] = v[0];
                                    dstBuffer[size * i + 1] = v[1];
                                    dstBuffer[size * i + 2] = v[2];
                                } break;
                                case 4: {
                                    const auto v =
                                        glm::vec4(srcBuffer[0], srcBuffer[1],
                                                  srcBuffer[2], srcBuffer[3]);
                                    dstBuffer[size * i + 0] = v[0];
                                    dstBuffer[size * i + 1] = v[1];
                                    dstBuffer[size * i + 2] = v[2];
                                    dstBuffer[size * i + 3] = v[3];
                                } break;
                            }
                        }
                    } break;
                }
            }
        }

        assert(numVertices > 0);

        if (!(allSemantics & SEM_NORMAL)) {
            auto geometryBuffer = (float *)rtcSetNewGeometryBuffer(
                geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                (uint32_t)VertexAttributeSlot::SLOT_NORMAL,
                (RTCFormat)(RTC_FORMAT_FLOAT3), sizeof(glm::vec3), numVertices);
            normals = (glm::vec3 *)geometryBuffer;

            // TODO: COMPUTE NORMALs
            for (auto i = 0; i < numVertices; i++) {
                normals[i] = glm::vec3(0.0f, 0.0f, 1.0f);
            }
        }

        if (!(allSemantics & SEM_TEXCOORD_0)) {
            auto geometryBuffer = (float *)rtcSetNewGeometryBuffer(
                geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                (uint32_t)VertexAttributeSlot::SLOT_TEXCOORD_0,
                (RTCFormat)(RTC_FORMAT_FLOAT2), sizeof(glm::vec3), numVertices);
            texCoords0 = (glm::vec2 *)geometryBuffer;

            // TODO: COMPUTE UVs
            for (auto i = 0; i < numVertices; i++) {
                texCoords0[i] = glm::vec2(0.0f, 0.0f);
            }
        }

        if (!(allSemantics & SEM_TANGENT)) {
            srcTangents = TangentGenerator::generate(
                numFaces, numVertices, (const glm::uvec3 *)triangles, positions,
                normals, texCoords0);
        }

        auto tangents = (glm::vec3 *)rtcSetNewGeometryBuffer(
            geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, SLOT_TANGENT,
            RTC_FORMAT_FLOAT3, sizeof(glm::vec3), numVertices);

        auto bitangents = (glm::vec3 *)rtcSetNewGeometryBuffer(
            geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, SLOT_BITANGENT,
            RTC_FORMAT_FLOAT3, sizeof(glm::vec3), numVertices);

        for (auto i = 0; i < numVertices; i++) {
            positions[i] = glm::vec3(transform * glm::vec4(positions[i], 1.0f));
            normals[i] = glm::normalize(
                glm::vec3(inverseTranspose * glm::vec4(normals[i], 0.0f)));
            tangents[i] = glm::normalize(glm::vec3(
                inverseTranspose * glm::vec4(glm::vec3(srcTangents[i]), 0.0f)));
            bitangents[i] = glm::normalize(glm::cross(normals[i], tangents[i]) *
                                           srcTangents[i].w);
        }

        {
            auto material = ConstantPMaterial(new Material(
                REFLECTION,
                baseColorFactor,
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

            auto mesh = PMesh(new Mesh());
            rtcSetGeometryUserData(geom, (void *)mesh.get());
            rtcSetGeometryIntersectFilterFunction(geom, intersectionFilter);
            rtcCommitGeometry(geom);
            auto geomId = rtcAttachGeometry(scene, geom);
            rtcReleaseGeometry(geom);

            mesh->setGeometryId(geomId);
            mesh->setWorldMatrix(transform);
            mesh->setWorldInverseTransposeMatrix(inverseTranspose);
            mesh->setMaterial(material);

            meshs.push_back(mesh);
        }
    }
}

void addNode(const RTCDevice device, const RTCScene scene,
             const tinygltf::Model &model,
             const std::vector<std::shared_ptr<const Texture>> &images,
             const tinygltf::Node &node, const glm::mat4 world,
             ConstantPMeshList &meshs) {
    glm::mat4 matrix(1.0f);
    if (node.matrix.size() == 16) {
        // Use `matrix' attribute
        matrix = glm::make_mat4(node.matrix.data());
    } else {
        // Assume Trans x Rotate x Scale order
        if (node.scale.size() == 3) {
            const auto &s = node.scale;
            matrix =
                glm::scale(glm::vec3((float)s[0], (float)s[1], (float)s[2])) *
                matrix;
        }

        if (node.rotation.size() == 4) {
            const auto &r = node.rotation;
            matrix = static_cast<glm::mat4>(glm::quat(
                         (float)r[3], (float)r[0], (float)r[1], (float)r[2])) *
                     matrix;
        }

        if (node.translation.size() == 3) {
            const auto &t = node.translation;
            matrix = glm::translate(
                         glm::vec3((float)t[0], (float)t[1], (float)t[2])) *
                     matrix;
        }
    }

    if (node.mesh > -1) {
        addMesh(device, scene, model, images, model.meshes[node.mesh],
                world * matrix, meshs);
    }

    // Draw child nodes.
    for (auto i = 0; i < node.children.size(); i++) {
        addNode(device, scene, model, images, model.nodes[node.children[i]],
                world * matrix, meshs);
    }
}

ConstantPMeshList addObjModel(const RTCDevice device, const RTCScene scene,
                              const std::string &filename) {
    ConstantPMeshList meshs;

    auto transform = glm::mat4(1.0f);
    auto inverseTranspose = glm::inverseTranspose(transform);

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
        return meshs;
    }

    std::vector<ConstantPMaterial> materials;
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

                auto ret = stbi_loadf(texFileName.c_str(), &width, &height,
                                      &components, 4);
                if (ret) {
                    const size_t n = width * height;
                    auto image = std::shared_ptr<glm::vec4[]>(new glm::vec4[n]);
                    memcpy(image.get(), ret, n * sizeof(glm::vec4));
                    stbi_image_free(ret);

                    int32_t wrapS = 0;
                    int32_t wrapT = 0;

                    textures.insert(std::make_pair(
                        mp->diffuse_texname,
                        std::shared_ptr<const Texture>(
                            new Texture(image, width, height, wrapS, wrapT))));
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
        materials.push_back(material);
    }

    // Append `default` material
    materials.push_back(ConstantPMaterial(new Material()));

    for (size_t s = 0; s < srcShapes.size(); s++) {
        std::vector<glm::vec3> srcPositions;
        std::vector<glm::vec3> srcNormals;
        std::vector<glm::vec2> srcTexcoords0;

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
                srcTexcoords0.push_back(glm::vec2(tc[k][0], tc[k][1]));
            }
        }

        auto materialId = srcShapes[s].mesh.material_ids[0];

        if (materialId < 0 || materialId >= materials.size()) {
            materialId = static_cast<int32_t>(materials.size()) - 1;
        }

        auto numTriangles = srcPositions.size() / 3;

        std::vector<glm::uvec3> srcIndices;
        for (auto i = 0; i < numTriangles; i++) {
            srcIndices.push_back(glm::uvec3(i * 3 + 0, i * 3 + 1, i * 3 + 2));
        }

        const auto material = materials[materialId];

        const auto numVertices = srcPositions.size();
        const auto numFaces = srcIndices.size();

        auto geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

        rtcSetGeometryVertexAttributeCount(
            geom, (uint32_t)VertexAttributeSlot::NUM_VERTEX_ATTRIBUTE_SLOTS);

        auto *positions = (glm::vec3 *)rtcSetNewGeometryBuffer(
            geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3,
            sizeof(glm::vec3), srcPositions.size());
        for (auto i = 0; i < srcPositions.size(); i++) {
            positions[i] = srcPositions[i];
        }

        auto *indices = (glm::uvec3 *)rtcSetNewGeometryBuffer(
            geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
            sizeof(glm::uvec3), srcIndices.size());
        for (auto i = 0; i < srcIndices.size(); i++) {
            indices[i] = srcIndices[i];
        }

        auto *normals = (glm::vec3 *)rtcSetNewGeometryBuffer(
            geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
            (uint32_t)VertexAttributeSlot::SLOT_NORMAL, RTC_FORMAT_FLOAT3,
            sizeof(glm::vec3), srcNormals.size());
        for (auto i = 0; i < srcNormals.size(); i++) {
            normals[i] = srcNormals[i];
        }

        auto *texCoords0 = (glm::vec2 *)rtcSetNewGeometryBuffer(
            geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
            (uint32_t)VertexAttributeSlot::SLOT_TEXCOORD_0, RTC_FORMAT_FLOAT2,
            sizeof(glm::vec2), srcTexcoords0.size());
        for (auto i = 0; i < srcTexcoords0.size(); i++) {
            texCoords0[i] = srcTexcoords0[i];
        }

        auto srcTangents = TangentGenerator::generate(
            static_cast<int32_t>(numFaces), static_cast<int32_t>(numVertices),
            indices, positions, normals, texCoords0);

        auto *tangents = (glm::vec3 *)rtcSetNewGeometryBuffer(
            geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
            (uint32_t)VertexAttributeSlot::SLOT_TANGENT, RTC_FORMAT_FLOAT3,
            sizeof(glm::vec3), srcTangents.size());

        auto *bitangents = (glm::vec3 *)rtcSetNewGeometryBuffer(
            geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
            (uint32_t)VertexAttributeSlot::SLOT_BITANGENT, RTC_FORMAT_FLOAT3,
            sizeof(glm::vec3), srcTangents.size());

        for (auto i = 0; i < numVertices; i++) {
            positions[i] = glm::vec3(transform * glm::vec4(positions[i], 1.0f));
            normals[i] = glm::normalize(
                glm::vec3(inverseTranspose * glm::vec4(normals[i], 0.0f)));
            tangents[i] = glm::normalize(glm::vec3(
                transform * glm::vec4(glm::vec3(srcTangents[i]), 0.0f)));
            bitangents[i] = glm::normalize(glm::cross(normals[i], tangents[i]) *
                                           srcTangents[i].w);
        }

        auto mesh = PMesh(new Mesh());
        rtcSetGeometryUserData(geom, (void *)mesh.get());
        rtcSetGeometryIntersectFilterFunction(geom, intersectionFilter);
        rtcCommitGeometry(geom);
        auto geomId = rtcAttachGeometry(scene, geom);
        rtcReleaseGeometry(geom);

        mesh->setGeometryId(geomId);
        mesh->setMaterial(material);
        mesh->setWorldMatrix(transform);
        mesh->setWorldInverseTransposeMatrix(inverseTranspose);
        meshs.push_back(mesh);
    }

    return meshs;
}

ConstantPMeshList addGlbModel(const RTCDevice device, const RTCScene scene,
                              const tinygltf::Model &model) {
    ConstantPMeshList meshs;
    const auto sceneToDisplay =
        model.defaultScene > -1 ? model.defaultScene : 0;
    const tinygltf::Scene &gltfScene = model.scenes[sceneToDisplay];

    std::vector<std::shared_ptr<const Texture>> textures;
    for (auto it = model.textures.begin(); it != model.textures.end(); it++) {
        const auto &bufferView =
            model.bufferViews[model.images[it->source].bufferView];
        const auto &buffer = model.buffers[bufferView.buffer];
        assert(bufferView.byteStride == 0);
        const auto p = &buffer.data[0];
        const size_t components = 4;

        int32_t width, height, channels;
        auto ret =
            stbi_loadf_from_memory(p + bufferView.byteOffset,
                                   static_cast<int32_t>(bufferView.byteLength),
                                   &width, &height, &channels, components);
        const size_t n = width * height;
        auto image = std::shared_ptr<glm::vec4[]>(new glm::vec4[n]);
        memcpy(image.get(), ret, n * sizeof(glm::vec4));
        stbi_image_free(ret);

        int32_t wrapS = 0;
        int32_t wrapT = 0;
        if (it->sampler >= 0) {
            const auto &sampler = model.samplers[it->sampler];
            wrapS = sampler.wrapS;
            wrapT = sampler.wrapT;
        }

        textures.push_back(
            std::make_shared<Texture>(image, width, height, wrapS, wrapT));
    }

    for (size_t i = 0; i < gltfScene.nodes.size(); i++) {
        auto matrix = glm::mat4(1.0f);
        const auto &node = model.nodes[gltfScene.nodes[i]];
        addNode(device, scene, model, textures, node, matrix, meshs);
    }

    return meshs;
}
