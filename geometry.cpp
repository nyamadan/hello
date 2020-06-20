#include "geometry.hpp"

#include "ray_tracer.hpp"

#include <stb_image.h>
#include <glm/ext.hpp>

const auto PI = 3.14159265358979323846f;

namespace {
void intersectionFilter(const struct RTCFilterFunctionNArguments *args) {
    auto context = (IntersectContext *)args->context;
    auto raytracer = context->raytracer;
    raytracer->intersectionFilter(args);
}
}  // namespace

ConstantPMesh addSphere(const RTCDevice device, const RTCScene scene,
                        ConstantPMaterial material, float radius,
                        uint32_t widthSegments, uint32_t heightSegments,
                        const glm::mat4 transform) {
    auto index = 0;
    auto grid = std::vector<std::vector<uint32_t>>();

    auto vertex = glm::vec3();
    auto normal = glm::vec3();

    auto indices = std::vector<glm::uvec3>();
    auto vertices = std::vector<glm::vec3>();
    auto normals = std::vector<glm::vec3>();
    auto uvs = std::vector<glm::vec2>();

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
            vertex.x = -radius * std::cos(u * 2 * PI) * std::sin(v * PI);
            vertex.y = radius * std::cos(v * PI);
            vertex.z = radius * std::sin(u * 2 * PI) * std::sin(v * PI);
            vertices.push_back(
                (glm::vec3)(transform * glm::vec4(vertex, 1.0f)));
            normal = glm::normalize(
                (glm::vec3)(inverseTranspose * glm::vec4(vertex, 1.0f)));
            normals.push_back(normal);
            uvs.push_back(glm::vec2(u + uOffset, 1.0f - v));
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

            if (iy != 0) indices.push_back(glm::uvec3(a, b, d));
            if (iy != heightSegments - 1)
                indices.push_back(glm::uvec3(b, c, d));
        }
    }

    auto geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

    auto *_vertices = (glm::vec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3),
        vertices.size());
    for (auto i = 0; i < vertices.size(); i++) {
        _vertices[i] = vertices[i];
    }

    auto *_indices = (glm::uvec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(glm::uvec3),
        indices.size());
    for (auto i = 0; i < indices.size(); i++) {
        _indices[i] = indices[i];
    }

    rtcSetGeometryVertexAttributeCount(geom, 2);

    auto *_normals = (glm::vec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT3,
        sizeof(glm::vec3), normals.size());
    for (auto i = 0; i < normals.size(); i++) {
        _normals[i] = normals[i];
    }

    auto *_uvs = (glm::vec2 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, RTC_FORMAT_FLOAT2,
        sizeof(glm::vec2), uvs.size());
    for (auto i = 0; i < uvs.size(); i++) {
        _uvs[i] = uvs[i];
    }

    auto mesh = PMesh(new Mesh());
    rtcSetGeometryUserData(geom, (void *)mesh.get());
    rtcSetGeometryIntersectFilterFunction(geom, intersectionFilter);
    rtcCommitGeometry(geom);
    auto geomId = rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);

    mesh->setGeometryId(geomId);
    mesh->setMaterial(material);
    return mesh;
}

/* adds a cube to the scene */
ConstantPMesh addCube(RTCDevice device, RTCScene scene,
                      ConstantPMaterial material, glm::mat4 transform) {
    /* create a triangulated cube with 12 triangles and 8 vertices */
    auto geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

    /* set vertices and vertex colors */
    auto *positions = (glm::vec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3),
        24);
    positions[0] = transform * glm::vec4(1, 1, -1, 1.0f);
    positions[1] = transform * glm::vec4(1, 1, 1, 1.0f);
    positions[2] = transform * glm::vec4(1, -1, 1, 1.0f);
    positions[3] = transform * glm::vec4(1, -1, -1, 1.0f);
    positions[4] = transform * glm::vec4(-1, 1, 1, 1.0f);
    positions[5] = transform * glm::vec4(-1, 1, -1, 1.0f);
    positions[6] = transform * glm::vec4(-1, -1, -1, 1.0f);
    positions[7] = transform * glm::vec4(-1, -1, 1, 1.0f);
    positions[8] = transform * glm::vec4(-1, 1, 1, 1.0f);
    positions[9] = transform * glm::vec4(1, 1, 1, 1.0f);
    positions[10] = transform * glm::vec4(1, 1, -1, 1.0f);
    positions[11] = transform * glm::vec4(-1, 1, -1, 1.0f);
    positions[12] = transform * glm::vec4(-1, -1, -1, 1.0f);
    positions[13] = transform * glm::vec4(1, -1, -1, 1.0f);
    positions[14] = transform * glm::vec4(1, -1, 1, 1.0f);
    positions[15] = transform * glm::vec4(-1, -1, 1, 1.0f);
    positions[16] = transform * glm::vec4(1, 1, 1, 1.0f);
    positions[17] = transform * glm::vec4(-1, 1, 1, 1.0f);
    positions[18] = transform * glm::vec4(-1, -1, 1, 1.0f);
    positions[19] = transform * glm::vec4(1, -1, 1, 1.0f);
    positions[20] = transform * glm::vec4(-1, 1, -1, 1.0f);
    positions[21] = transform * glm::vec4(1, 1, -1, 1.0f);
    positions[22] = transform * glm::vec4(1, -1, -1, 1.0f);
    positions[23] = transform * glm::vec4(-1, -1, -1, 1.0f);

    /* set triangles and face colors */
    glm::uvec3 *indices = (glm::uvec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(glm::uvec3),
        12);
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

    rtcSetGeometryVertexAttributeCount(geom, 2);
    glm::vec3 *normals = (glm::vec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT3,
        sizeof(glm::vec3), 24);
    const auto inverseTranspose = glm::transpose(glm::inverse(transform));
    normals[0] = inverseTranspose * glm::vec4(1, 0, 0, 1);
    normals[1] = inverseTranspose * glm::vec4(1, 0, 0, 1);
    normals[2] = inverseTranspose * glm::vec4(1, 0, 0, 1);
    normals[3] = inverseTranspose * glm::vec4(1, 0, 0, 1);
    normals[4] = inverseTranspose * glm::vec4(-1, 0, 0, 1);
    normals[5] = inverseTranspose * glm::vec4(-1, 0, 0, 1);
    normals[6] = inverseTranspose * glm::vec4(-1, 0, 0, 1);
    normals[7] = inverseTranspose * glm::vec4(-1, 0, 0, 1);
    normals[8] = inverseTranspose * glm::vec4(0, 1, 0, 1);
    normals[9] = inverseTranspose * glm::vec4(0, 1, 0, 1);
    normals[10] = inverseTranspose * glm::vec4(0, 1, 0, 1);
    normals[11] = inverseTranspose * glm::vec4(0, 1, 0, 1);
    normals[12] = inverseTranspose * glm::vec4(0, -1, 0, 1);
    normals[13] = inverseTranspose * glm::vec4(0, -1, 0, 1);
    normals[14] = inverseTranspose * glm::vec4(0, -1, 0, 1);
    normals[15] = inverseTranspose * glm::vec4(0, -1, 0, 1);
    normals[16] = inverseTranspose * glm::vec4(0, 0, 1, 1);
    normals[17] = inverseTranspose * glm::vec4(0, 0, 1, 1);
    normals[18] = inverseTranspose * glm::vec4(0, 0, 1, 1);
    normals[19] = inverseTranspose * glm::vec4(0, 0, 1, 1);
    normals[20] = inverseTranspose * glm::vec4(0, 0, -1, 1);
    normals[21] = inverseTranspose * glm::vec4(0, 0, -1, 1);
    normals[22] = inverseTranspose * glm::vec4(0, 0, -1, 1);
    normals[23] = inverseTranspose * glm::vec4(0, 0, -1, 1);
    for (auto i = 0; i < 24; i++) {
        normals[i] = glm::normalize(normals[i]);
    }

    glm::vec2 *uvs = (glm::vec2 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, RTC_FORMAT_FLOAT3,
        sizeof(glm::vec2), 24);
    uvs[0] = glm::vec2(1, 0);
    uvs[1] = glm::vec2(0, 0);
    uvs[2] = glm::vec2(0, 1);
    uvs[3] = glm::vec2(1, 1);
    uvs[4] = glm::vec2(1, 0);
    uvs[5] = glm::vec2(0, 0);
    uvs[6] = glm::vec2(0, 1);
    uvs[7] = glm::vec2(1, 1);
    uvs[8] = glm::vec2(1, 0);
    uvs[9] = glm::vec2(0, 0);
    uvs[10] = glm::vec2(0, 1);
    uvs[11] = glm::vec2(1, 1);
    uvs[12] = glm::vec2(1, 0);
    uvs[13] = glm::vec2(0, 0);
    uvs[14] = glm::vec2(0, 1);
    uvs[15] = glm::vec2(1, 1);
    uvs[16] = glm::vec2(1, 0);
    uvs[17] = glm::vec2(0, 0);
    uvs[18] = glm::vec2(0, 1);
    uvs[19] = glm::vec2(1, 1);
    uvs[20] = glm::vec2(1, 0);
    uvs[21] = glm::vec2(0, 0);
    uvs[22] = glm::vec2(0, 1);
    uvs[23] = glm::vec2(1, 1);

    auto mesh = PMesh(new Mesh());
    rtcSetGeometryUserData(geom, (void *)mesh.get());
    rtcSetGeometryIntersectFilterFunction(geom, intersectionFilter);
    rtcCommitGeometry(geom);
    auto geomId = rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);

    mesh->setGeometryId(geomId);
    mesh->setMaterial(material);
    return mesh;
}

/* adds a ground plane to the scene */
ConstantPMesh addGroundPlane(RTCDevice device, RTCScene scene,
                             ConstantPMaterial material,
                             const glm::mat4 transform) {
    /* create a triangulated plane with 2 triangles and 4 vertices */
    auto geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

    /* set vertices */
    auto *positions = (glm::vec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3),
        4);
    positions[0] = transform * glm::vec4(-1, 0, -1, 1);
    positions[1] = transform * glm::vec4(-1, 0, +1, 1);
    positions[2] = transform * glm::vec4(+1, 0, -1, 1);
    positions[3] = transform * glm::vec4(+1, 0, +1, 1);

    /* set triangles */
    auto *indices = (glm::uvec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(glm::uvec3),
        2);
    indices[0] = glm::uvec3(0, 1, 2);
    indices[1] = glm::uvec3(1, 3, 2);

    rtcSetGeometryVertexAttributeCount(geom, 2);

    const auto inverseTranspose = glm::transpose(glm::inverse(transform));
    auto *normals = (glm::vec3 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT3,
        sizeof(glm::vec3), 4);
    normals[0] = inverseTranspose * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    normals[1] = inverseTranspose * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    normals[2] = inverseTranspose * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    normals[3] = inverseTranspose * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    for (auto i = 0; i < 4; i++) {
        normals[i] = glm::normalize(normals[i]);
    }

    auto *uvs = (glm::vec2 *)rtcSetNewGeometryBuffer(
        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, RTC_FORMAT_FLOAT2,
        sizeof(glm::vec2), 4);
    uvs[0] = glm::vec2(0.0f, 0.0f);
    uvs[1] = glm::vec2(0.0f, 1.0f);
    uvs[2] = glm::vec2(1.0f, 0.0f);
    uvs[3] = glm::vec2(1.0f, 1.0f);

    auto mesh = PMesh(new Mesh());
    rtcSetGeometryUserData(geom, (void *)mesh.get());
    rtcSetGeometryIntersectFilterFunction(geom, intersectionFilter);
    rtcCommitGeometry(geom);
    auto geomId = rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);

    mesh->setGeometryId(geomId);
    mesh->setMaterial(material);

    return mesh;
}

void addMesh(const RTCDevice device, const RTCScene scene,
             const tinygltf::Model &model,
             const std::vector<std::shared_ptr<const Texture>> &images,
             const tinygltf::Mesh &gltfMesh, const glm::mat4 &world,
             ConstantPMeshList &meshs) {
    for (size_t i = 0; i < gltfMesh.primitives.size(); i++) {
        const auto &primitive = gltfMesh.primitives[i];

        if (primitive.indices < 0) return;

        auto gltfMaterial = model.materials[primitive.material];

        auto baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        auto baseColorTextureIndex = -1;
        auto metallicFactor = 0.5f;
        auto emissiveFactor = glm::vec3(0.0f, 0.0f, 0.0f);
        auto emissiveTextureIndex = -1;

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
        metallicFactor = (float)pbr.metallicFactor;

        if (gltfMaterial.emissiveFactor.size() == 3) {
            emissiveFactor = glm::vec3(gltfMaterial.emissiveFactor[0],
                                       gltfMaterial.emissiveFactor[1],
                                       gltfMaterial.emissiveFactor[2]);
        }
        emissiveTextureIndex = gltfMaterial.emissiveTexture.index;

        int mode = primitive.mode;
        assert(mode == TINYGLTF_MODE_TRIANGLES);

        auto geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
        rtcSetGeometryVertexAttributeCount(geom, 2);
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

        auto *triangles = (uint32_t *)rtcSetNewGeometryBuffer(
            geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
            sizeof(uint32_t) * 3, indexAccessor.count / 3);

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
                semantics = 1 << 0;
            } else if (it->first.compare("NORMAL") == 0) {
                semantics = 1 << 1;
            } else if (it->first.compare("TEXCOORD_0") == 0) {
                semantics = 1 << 2;
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
                        float *geometryBuffer = nullptr;

                        switch (semantics) {
                            case 1: {
                                geometryBuffer =
                                    (float *)rtcSetNewGeometryBuffer(
                                        geom, RTC_BUFFER_TYPE_VERTEX, 0,
                                        (RTCFormat)((int)RTC_FORMAT_FLOAT +
                                                    size - 1),
                                        sizeof(float) * size, accessor.count);
                            } break;
                            case 2: {
                                geometryBuffer =
                                    (float *)rtcSetNewGeometryBuffer(
                                        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                                        0,
                                        (RTCFormat)((int)RTC_FORMAT_FLOAT +
                                                    size - 1),
                                        sizeof(float) * size, accessor.count);
                            } break;
                            case 4: {
                                geometryBuffer =
                                    (float *)rtcSetNewGeometryBuffer(
                                        geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                                        1,
                                        (RTCFormat)((int)RTC_FORMAT_FLOAT +
                                                    size - 1),
                                        sizeof(float) * size, accessor.count);
                            } break;
                        }

                        for (auto i = 0; i < accessor.count; i++) {
                            const auto buffer =
                                (const float *)(model.buffers[bufferView.buffer]
                                                    .data.data() +
                                                byteOffset + byteStride * i);

                            switch (size) {
                                case 2: {
                                    const auto v =
                                        glm::vec2(buffer[0], buffer[1]);
                                    geometryBuffer[size * i + 0] = v[0];
                                    geometryBuffer[size * i + 1] = v[1];
                                } break;
                                case 3: {
                                    switch (semantics) {
                                        case 1: {
                                            const auto v =
                                                world *
                                                glm::vec4(buffer[0], buffer[1],
                                                          buffer[2], 1.0f);
                                            geometryBuffer[size * i + 0] = v[0];
                                            geometryBuffer[size * i + 1] = v[1];
                                            geometryBuffer[size * i + 2] = v[2];
                                        } break;
                                        case 2: {
                                            const auto v =
                                                glm::transpose(
                                                    glm::inverse(world)) *
                                                glm::vec4(buffer[0], buffer[1],
                                                          buffer[2], 1.0f);
                                            geometryBuffer[size * i + 0] = v[0];
                                            geometryBuffer[size * i + 1] = v[1];
                                            geometryBuffer[size * i + 2] = v[2];
                                        } break;
                                        default: {
                                            const auto v =
                                                glm::vec4(buffer[0], buffer[1],
                                                          buffer[2], 1.0f);
                                            geometryBuffer[size * i + 0] = v[0];
                                            geometryBuffer[size * i + 1] = v[1];
                                            geometryBuffer[size * i + 2] = v[2];
                                        } break;
                                    }

                                } break;
                                case 4: {
                                    const auto v =
                                        glm::vec4(buffer[0], buffer[1],
                                                  buffer[2], buffer[3]);
                                    geometryBuffer[size * i + 0] = v[0];
                                    geometryBuffer[size * i + 1] = v[1];
                                    geometryBuffer[size * i + 2] = v[2];
                                    geometryBuffer[size * i + 3] = v[3];
                                } break;
                            }
                        }
                    } break;
                }
            }
        }

        if (!(allSemantics & 2)) {
            auto normals = (float *)rtcSetNewGeometryBuffer(
                geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0,
                (RTCFormat)(RTC_FORMAT_FLOAT3), sizeof(glm::vec3),
                indexAccessor.count / 3);
        }

        if (!(allSemantics & 4)) {
            auto uvs = (float *)rtcSetNewGeometryBuffer(
                geom, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1,
                (RTCFormat)(RTC_FORMAT_FLOAT2), sizeof(glm::vec3),
                indexAccessor.count / 2);
        }

        {
            auto material = ConstantPMaterial(new Material(
                baseColorFactor,
                baseColorTextureIndex >= 0 ? images[baseColorTextureIndex]
                                           : nullptr,
                metallicFactor, emissiveFactor,
                emissiveTextureIndex >= 0 ? images[emissiveTextureIndex]
                                          : nullptr,
                false));

            auto mesh = PMesh(new Mesh());
            rtcSetGeometryUserData(geom, (void *)mesh.get());
            rtcSetGeometryIntersectFilterFunction(geom, intersectionFilter);
            rtcCommitGeometry(geom);
            auto geomId = rtcAttachGeometry(scene, geom);
            rtcReleaseGeometry(geom);

            mesh->setGeometryId(geomId);
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
            (glm::mat4) glm::quat((float)r[0], (float)r[1], (float)r[2],
                                  (float)r[3]) *
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

ConstantPMeshList addModel(const RTCDevice device, const RTCScene scene,
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
            stbi_load_from_memory(p + bufferView.byteOffset,
                                  static_cast<int32_t>(bufferView.byteLength),
                                  &width, &height, &channels, components);
        const size_t n = width * height;
        auto image = std::shared_ptr<glm::u8vec4[]>(new glm::u8vec4[n]);
        memcpy(image.get(), ret, n * sizeof(glm::u8vec4));
        stbi_image_free(ret);

        textures.push_back(std::make_shared<Texture>(image, width, height));
    }

    for (size_t i = 0; i < gltfScene.nodes.size(); i++) {
        auto matrix = glm::mat4(1.0f);
        const auto &node = model.nodes[gltfScene.nodes[i]];
        addNode(device, scene, model, textures, node, matrix, meshs);
    }

    return meshs;
}
