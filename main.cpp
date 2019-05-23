#include <algorithm>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include <stb_image_write.h>
#include <tbb/parallel_for.h>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <embree3/rtcore.h>
#include <embree3/rtcore_common.h>
#include <embree3/rtcore_device.h>
#include <embree3/rtcore_geometry.h>
#include <embree3/rtcore_ray.h>

#include <tiny_gltf.h>

#include "ray_tracer.hpp"

void glfwErrorCallback(int error, const char *description) {
    fprintf(stderr, "error %d: %s\n", error, description);
}

void glDebugOutput(GLenum source, GLenum type, GLuint eid, GLenum severity,
                   GLsizei length, const GLchar *message,
                   const void *user_param) {
    switch (severity) {
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            break;
        case GL_DEBUG_SEVERITY_LOW:
        case GL_DEBUG_SEVERITY_MEDIUM:
        case GL_DEBUG_SEVERITY_HIGH:
            fprintf(stderr, "ERROR(%X): %s\n", eid, message);
            break;
    }
}

void EnableOpenGLDebugExtention() {
    GLint flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0,
                              nullptr, GL_TRUE);
    }
}

/* adds a cube to the scene */
unsigned int addCube(RTCDevice device, RTCScene scene, glm::mat4 transform = glm::mat4(1.0f)) {
    /* create a triangulated cube with 12 triangles and 8 vertices */
    auto mesh = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

    /* set vertices and vertex colors */
    auto *positions = (glm::vec3 *)rtcSetNewGeometryBuffer(
        mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3),
        24);
    positions[0] = transform * glm::vec4(glm::vec3(1, 1, -1), 1.0f);
    positions[1] = transform * glm::vec4(glm::vec3(1, 1, 1), 1.0f);
    positions[2] = transform * glm::vec4(glm::vec3(1, -1, 1), 1.0f);
    positions[3] = transform * glm::vec4(glm::vec3(1, -1, -1), 1.0f);
    positions[4] = transform * glm::vec4(glm::vec3(-1, 1, 1), 1.0f);
    positions[5] = transform * glm::vec4(glm::vec3(-1, 1, -1), 1.0f);
    positions[6] = transform * glm::vec4(glm::vec3(-1, -1, -1), 1.0f);
    positions[7] = transform * glm::vec4(glm::vec3(-1, -1, 1), 1.0f);
    positions[8] = transform * glm::vec4(glm::vec3(-1, 1, 1), 1.0f);
    positions[9] = transform * glm::vec4(glm::vec3(1, 1, 1), 1.0f);
    positions[10] = transform * glm::vec4(glm::vec3(1, 1, -1), 1.0f);
    positions[11] = transform * glm::vec4(glm::vec3(-1, 1, -1), 1.0f);
    positions[12] = transform * glm::vec4(glm::vec3(-1, -1, -1), 1.0f);
    positions[13] = transform * glm::vec4(glm::vec3(1, -1, -1), 1.0f);
    positions[14] = transform * glm::vec4(glm::vec3(1, -1, 1), 1.0f);
    positions[15] = transform * glm::vec4(glm::vec3(-1, -1, 1), 1.0f);
    positions[16] = transform * glm::vec4(glm::vec3(1, 1, 1), 1.0f);
    positions[17] = transform * glm::vec4(glm::vec3(-1, 1, 1), 1.0f);
    positions[18] = transform * glm::vec4(glm::vec3(-1, -1, 1), 1.0f);
    positions[19] = transform * glm::vec4(glm::vec3(1, -1, 1), 1.0f);
    positions[20] = transform * glm::vec4(glm::vec3(-1, 1, -1), 1.0f);
    positions[21] = transform * glm::vec4(glm::vec3(1, 1, -1), 1.0f);
    positions[22] = transform * glm::vec4(glm::vec3(1, -1, -1), 1.0f);
    positions[23] = transform * glm::vec4(glm::vec3(-1, -1, -1), 1.0f);

    /* set triangles and face colors */
    glm::uvec3 *indices = (glm::uvec3 *)rtcSetNewGeometryBuffer(
        mesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(glm::uvec3),
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

    rtcSetGeometryVertexAttributeCount(mesh, 2);
    glm::vec3 *normals = (glm::vec3 *)rtcSetNewGeometryBuffer(
        mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT3,
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

    glm::vec2 *uvs = (glm::vec2 *)rtcSetNewGeometryBuffer(
        mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, RTC_FORMAT_FLOAT3,
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

    rtcCommitGeometry(mesh);
    auto geomID = rtcAttachGeometry(scene, mesh);
    rtcReleaseGeometry(mesh);
    return geomID;
}

/* adds a ground plane to the scene */
unsigned int addGroundPlane(RTCDevice device, RTCScene scene) {
    /* create a triangulated plane with 2 triangles and 4 vertices */
    auto mesh = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

    /* set vertices */
    auto *positions = (glm::vec3 *)rtcSetNewGeometryBuffer(
        mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3),
        4);
    positions[0] = glm::vec3(-10, -2, -10);
    positions[1] = glm::vec3(-10, -2, +10);
    positions[2] = glm::vec3(+10, -2, -10);
    positions[3] = glm::vec3(+10, -2, +10);

    /* set triangles */
    auto *indices = (glm::uvec3 *)rtcSetNewGeometryBuffer(
        mesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(glm::uvec3),
        2);
    indices[0] = glm::uvec3(0, 1, 2);
    indices[1] = glm::uvec3(1, 3, 2);

    rtcSetGeometryVertexAttributeCount(mesh, 2);

    auto *normals = (glm::vec3 *)rtcSetNewGeometryBuffer(
        mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT3,
        sizeof(glm::vec3), 4);
    normals[0] = glm::vec3(0.0f, 1.0f, 0.0f);
    normals[1] = glm::vec3(0.0f, 1.0f, 0.0f);
    normals[2] = glm::vec3(0.0f, 1.0f, 0.0f);
    normals[3] = glm::vec3(0.0f, 1.0f, 0.0f);

    auto *uvs = (glm::vec2 *)rtcSetNewGeometryBuffer(
        mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, RTC_FORMAT_FLOAT2,
        sizeof(glm::vec2), 4);
    uvs[0] = glm::vec2(0.0f, 0.0f);
    uvs[1] = glm::vec2(0.0f, 1.0f);
    uvs[2] = glm::vec2(1.0f, 0.0f);
    uvs[3] = glm::vec2(1.0f, 1.0f);

    rtcCommitGeometry(mesh);
    auto geomID = rtcAttachGeometry(scene, mesh);
    rtcReleaseGeometry(mesh);
    return geomID;
}

static void addMesh(const RTCDevice device, const RTCScene rtcScene,
                    const tinygltf::Model &model,
                    const tinygltf::Mesh &gltfMesh, const glm::mat4 &world,
                    std::vector<int> &geomIds) {
    for (size_t i = 0; i < gltfMesh.primitives.size(); i++) {
        const auto &primitive = gltfMesh.primitives[i];

        if (primitive.indices < 0) return;

        auto it(primitive.attributes.begin());
        auto itEnd(primitive.attributes.end());

        int mode = primitive.mode;
        assert(mode == TINYGLTF_MODE_TRIANGLES);

        auto rtcMesh = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
        rtcSetGeometryVertexAttributeCount(rtcMesh, 2);
        auto allSemantics = 0;

        const auto &indexAccessor = model.accessors[primitive.indices];
        const auto &indexBufferView =
            model.bufferViews[indexAccessor.bufferView];
        const auto &indexBuffer = model.buffers[indexBufferView.buffer];

        assert(indexAccessor.type == TINYGLTF_TYPE_SCALAR);
        assert(indexAccessor.componentType ==
               TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT);

        auto *triangles = (uint32_t *)rtcSetNewGeometryBuffer(
            rtcMesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
            sizeof(uint32_t) * 3, indexAccessor.count / 3);

        for (auto i = 0; i < indexAccessor.count; i++) {
            const auto byteStride = indexAccessor.ByteStride(indexBufferView);
            const auto byteOffset =
                indexAccessor.byteOffset + indexBufferView.byteOffset;
            const auto componentType = indexAccessor.componentType;
            const auto normalized = indexAccessor.normalized;
            const auto buffer =
                (uint16_t *)(model.buffers[indexBufferView.buffer].data.data() +
                             byteOffset + byteStride * i);

            triangles[i] = *buffer;
        }

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
                                        rtcMesh, RTC_BUFFER_TYPE_VERTEX, 0,
                                        (RTCFormat)((int)RTC_FORMAT_FLOAT +
                                                    size - 1),
                                        sizeof(float) * size, accessor.count);
                            } break;
                            case 2: {
                                geometryBuffer =
                                    (float *)rtcSetNewGeometryBuffer(
                                        rtcMesh,
                                        RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0,
                                        (RTCFormat)((int)RTC_FORMAT_FLOAT +
                                                    size - 1),
                                        sizeof(float) * size, accessor.count);
                            } break;
                            case 4: {
                                geometryBuffer =
                                    (float *)rtcSetNewGeometryBuffer(
                                        rtcMesh,
                                        RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1,
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
                rtcMesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0,
                (RTCFormat)(RTC_FORMAT_FLOAT3), sizeof(glm::vec3),
                indexAccessor.count / 3);
        }

        if (!(allSemantics & 4)) {
            auto uvs = (float *)rtcSetNewGeometryBuffer(
                rtcMesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1,
                (RTCFormat)(RTC_FORMAT_FLOAT2), sizeof(glm::vec3),
                indexAccessor.count / 2);
        }

        rtcCommitGeometry(rtcMesh);
        auto geomID = rtcAttachGeometry(rtcScene, rtcMesh);
        rtcReleaseGeometry(rtcMesh);

        geomIds.push_back(geomID);
    }
}

static void addNode(const RTCDevice device, const RTCScene scene,
                    const tinygltf::Model &model, const tinygltf::Node &node,
                    const glm::mat4 world, std::vector<int> &geomIds) {
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
        addMesh(device, scene, model, model.meshes[node.mesh], world * matrix,
                geomIds);
    }

    // Draw child nodes.
    for (auto i = 0; i < node.children.size(); i++) {
        addNode(device, scene, model, model.nodes[node.children[i]],
                world * matrix, geomIds);
    }
}

std::vector<int> addModel(const RTCDevice device, const RTCScene rtcScene,
                          const tinygltf::Model &model) {
    std::vector<int> geomIds;
    const auto sceneToDisplay =
        model.defaultScene > -1 ? model.defaultScene : 0;
    const tinygltf::Scene &gltfScene = model.scenes[sceneToDisplay];

    for (size_t i = 0; i < gltfScene.nodes.size(); i++) {
        auto matrix = glm::mat4(1.0f);
        const auto &node = model.nodes[gltfScene.nodes[i]];
        addNode(device, rtcScene, model, node, matrix, geomIds);
    }

    return geomIds;
}

int addSphere(const RTCDevice device, const RTCScene scene, float radius = 1.0f,
              uint32_t widthSegments = 8, uint32_t heightSegments = 6) {
    const auto pi = 3.14159265358979323846f;
    auto index = 0;
    auto grid = std::vector<std::vector<uint32_t>>();

    auto vertex = glm::vec3();
    auto normal = glm::vec3();

    auto indices = std::vector<glm::uvec3>();
    auto vertices = std::vector<glm::vec3>();
    auto normals = std::vector<glm::vec3>();
    auto uvs = std::vector<glm::vec2>();

    for (auto iy = 0; iy <= heightSegments; iy++) {
        auto verticesRow = std::vector<uint32_t>();

        auto v = (float)iy / heightSegments;

        auto uOffset = 0;

        if (iy == 0) {
            uOffset = 0.5 / widthSegments;

        } else if (iy == heightSegments) {
            uOffset = -0.5 / widthSegments;
        }

        for (auto ix = 0; ix <= widthSegments; ix++) {
            auto u = (float)ix / widthSegments;
            vertex.x = -radius * std::cos(u * 2 * pi) * std::sin(v * pi);
            vertex.y = radius * std::cos(v * pi);
            vertex.z = radius * std::sin(u * 2 * pi) * std::sin(v * pi);
            vertices.push_back(vertex);
            normal = glm::normalize(vertex);
            normals.push_back(normal);
            uvs.push_back(glm::vec2(u + uOffset, 1.0f - v));
            verticesRow.push_back(index++);
        }
        grid.push_back(verticesRow);
    }
    for (auto iy = 0; iy < heightSegments; iy++) {
        for (auto ix = 0; ix < widthSegments; ix++) {
            auto a = grid[iy][ix + 1];
            auto b = grid[iy][ix];
            auto c = grid[iy + 1][ix];
            auto d = grid[iy + 1][ix + 1];

            if (iy != 0) indices.push_back(glm::uvec3(a, b, d));
            if (iy != heightSegments - 1)
                indices.push_back(glm::uvec3(b, c, d));
        }
    }

    auto mesh = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

    auto *_vertices = (glm::vec3 *)rtcSetNewGeometryBuffer(
        mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3),
        vertices.size());
    for (auto i = 0; i < vertices.size(); i++) {
        _vertices[i] = vertices[i];
    }

    auto *_indices = (glm::uvec3 *)rtcSetNewGeometryBuffer(
        mesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(glm::uvec3),
        indices.size());
    for (auto i = 0; i < indices.size(); i++) {
        _indices[i] = indices[i];
    }

    rtcSetGeometryVertexAttributeCount(mesh, 2);

    auto *_normals = (glm::vec3 *)rtcSetNewGeometryBuffer(
        mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, RTC_FORMAT_FLOAT3,
        sizeof(glm::vec3), normals.size());
    for (auto i = 0; i < normals.size(); i++) {
        _normals[i] = normals[i];
    }

    auto *_uvs = (glm::vec2 *)rtcSetNewGeometryBuffer(
        mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, RTC_FORMAT_FLOAT2,
        sizeof(glm::vec2), uvs.size());
    for (auto i = 0; i < uvs.size(); i++) {
        _uvs[i] = uvs[i];
    }

    rtcCommitGeometry(mesh);
    auto geomID = rtcAttachGeometry(scene, mesh);
    rtcReleaseGeometry(mesh);
    return geomID;
}

void copyPixelsToTexture(const glm::u8vec3 pixels[], GLuint fbo, GLuint texture,
                         int32_t width, int32_t height) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::dvec2 getWindowMousePos(GLFWwindow *window, const glm::u32vec2 &size) {
    double x, y;
    double aspect = size.x / size.y;
    glfwGetCursorPos(window, &x, &y);
    return glm::dvec2(aspect * (x / size.x - 0.5f), 1.0f - y / size.y);
}

int main(void) {
    auto device = rtcNewDevice("verbose=1");
    auto scene = rtcNewScene(device);

    auto plane = addGroundPlane(device, scene);
    // auto cube = addCube(device, scene);
    // auto sphere = addSphere(device, scene, 1.8f, 80, 60);

    // add model
     tinygltf::Model model;
     tinygltf::TinyGLTF loader;
     std::string err;
     std::string warn;
     const auto ret = loader.LoadBinaryFromFile(&model, &err, &warn,
     "Buggy.glb"); if (ret) {
        const auto box = addModel(device, scene, model);
    }

    auto raytracer = RayTracer();
    const auto width = 640u;
    const auto height = 480u;

    auto camera = RayTracerCamera(width, height, 120.0f, 0.001f, 1000.0f);
    const auto eye = glm::vec3(80, 80, -90);
    const auto target = glm::vec3(0.0f, 0.0f, 0.0f);
    const auto up = glm::vec3(0.0f, 1.0f, 0.0f);
    camera.lookAt(eye, target, up);

    rtcCommitScene(scene);

    auto pixels = std::make_unique<glm::u8vec3[]>(width * height);

    raytracer.render(scene, camera, pixels.get());

    stbi_flip_vertically_on_write(true);
    stbi_write_png("hello_embree.png", width, height, 3, pixels.get(),
                   3 * width);

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifndef NDEBUG
    glfwSetErrorCallback(glfwErrorCallback);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
#endif

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    const auto windowSize = glm::u32vec2(width, height);
    const auto window = glfwCreateWindow(windowSize.x, windowSize.y,
                                         "Hello Embree", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Initialize OpenGL loader
    if (gl3wInit() != 0) {
        return 1;
    }

#ifndef NDEBUG
    EnableOpenGLDebugExtention();
#endif

    GLuint texture = 0;
    GLuint fbo = 0;
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &texture);
    glViewport(0, 0, windowSize.x, windowSize.y);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

    glm::dvec2 prevMousePos = getWindowMousePos(window, windowSize);

    auto t0 = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        auto t = glfwGetTime();
        auto dt = t - t0;
        glm::dvec2 mousePos = getWindowMousePos(window, windowSize);

        glm::vec2 mouseDelta = mousePos - prevMousePos;

        const auto side = camera.getCameraSide();
        const auto up = camera.getCameraUp(side);
        const auto lbtn = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
        const auto controlSpeed = 1.0f;
        if (lbtn == GLFW_PRESS) {
            camera.setCameraDir(glm::rotate(
                glm::rotate(camera.getCameraDir(), -mouseDelta.y, side),
                mouseDelta.x, up));
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            const auto dir = camera.getCameraDir();
            camera.setCameraOrigin(camera.getCameraOrigin() +
                                   dir * controlSpeed * dt);
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            const auto dir = camera.getCameraDir();
            camera.setCameraOrigin(camera.getCameraOrigin() -
                                   dir * controlSpeed * dt);
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            const auto dir = camera.getCameraDir();
            camera.setCameraOrigin(camera.getCameraOrigin() +
                                   side * controlSpeed * dt);
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            const auto dir = camera.getCameraDir();
            camera.setCameraOrigin(camera.getCameraOrigin() -
                                   side * controlSpeed * dt);
        }

        raytracer.render(scene, camera, pixels.get());

        copyPixelsToTexture(pixels.get(), fbo, texture, width, height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glBlitFramebuffer(0, 0, width, height, 0, 0, windowSize.x, windowSize.y,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glfwSwapBuffers(window);
        glfwPollEvents();

        prevMousePos = mousePos;
        t0 = t;
    }

    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texture);

    glfwTerminate();

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
    return 0;
}