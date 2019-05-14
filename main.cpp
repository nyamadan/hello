#include <algorithm>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>

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

#include "aligned_deleter.hpp"
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
unsigned int addCube(RTCDevice device, RTCScene scene,
                     glm::vec3 vertex_colors[], glm::vec3 face_colors[]) {
    /* create a triangulated cube with 12 triangles and 8 vertices */
    auto mesh = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

    /* set vertices and vertex colors */
    auto *vertices = (glm::vec3 *)rtcSetNewGeometryBuffer(
        mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3),
        8);
    vertex_colors[0] = glm::vec3(0, 0, 0);
    vertices[0] = glm::ivec3(-1, -1, -1);
    vertex_colors[1] = glm::vec3(0, 0, 1);
    vertices[1] = glm::ivec3(-1, -1, +1);
    vertex_colors[2] = glm::vec3(0, 1, 0);
    vertices[2] = glm::ivec3(-1, +1, -1);
    vertex_colors[3] = glm::vec3(0, 1, 1);
    vertices[3] = glm::ivec3(-1, +1, +1);
    vertex_colors[4] = glm::vec3(1, 0, 0);
    vertices[4] = glm::ivec3(+1, -1, -1);
    vertex_colors[5] = glm::vec3(1, 0, 1);
    vertices[5] = glm::ivec3(+1, -1, +1);
    vertex_colors[6] = glm::vec3(1, 1, 0);
    vertices[6] = glm::ivec3(+1, +1, -1);
    vertex_colors[7] = glm::vec3(1, 1, 1);
    vertices[7] = glm::ivec3(+1, +1, +1);

    /* set triangles and face colors */
    auto tri = 0;
    glm::uvec3 *triangles = (glm::uvec3 *)rtcSetNewGeometryBuffer(
        mesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(glm::uvec3),
        12);

    // left side
    face_colors[tri] = glm::vec3(1, 0, 0);
    triangles[tri] = glm::uvec3(0, 1, 2);
    tri++;
    face_colors[tri] = glm::vec3(1, 0, 0);
    triangles[tri] = glm::uvec3(1, 3, 2);
    tri++;

    // right side
    face_colors[tri] = glm::vec3(0, 1, 0);
    triangles[tri] = glm::uvec3(4, 6, 5);
    tri++;
    face_colors[tri] = glm::vec3(0, 1, 0);
    triangles[tri] = glm::uvec3(5, 6, 7);
    tri++;

    // bottom side
    face_colors[tri] = glm::vec3(0.5f);
    triangles[tri] = glm::uvec3(0, 4, 1);
    tri++;
    face_colors[tri] = glm::vec3(0.5f);
    triangles[tri] = glm::uvec3(1, 4, 5);
    tri++;

    // top side
    face_colors[tri] = glm::vec3(1.0f);
    triangles[tri] = glm::uvec3(2, 3, 6);
    tri++;
    face_colors[tri] = glm::vec3(1.0f);
    triangles[tri] = glm::uvec3(3, 7, 6);
    tri++;

    // front side
    face_colors[tri] = glm::vec3(0, 0, 1);
    triangles[tri] = glm::uvec3(0, 2, 4);
    tri++;
    face_colors[tri] = glm::vec3(0, 0, 1);
    triangles[tri] = glm::uvec3(2, 6, 4);
    tri++;

    // back side
    face_colors[tri] = glm::vec3(1, 1, 0);
    triangles[tri] = glm::uvec3(1, 5, 3);
    tri++;
    face_colors[tri] = glm::vec3(1, 1, 0);
    triangles[tri] = glm::uvec3(3, 5, 7);
    tri++;

    rtcSetGeometryVertexAttributeCount(mesh, 1);
    rtcSetSharedGeometryBuffer(mesh, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0,
                               RTC_FORMAT_FLOAT3, vertex_colors, 0,
                               sizeof(glm::vec3), 8);

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
    auto *vertices = (glm::vec3 *)rtcSetNewGeometryBuffer(
        mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3),
        4);
    vertices[0] = glm::vec3(-10, -2, -10);
    vertices[1] = glm::vec3(-10, -2, +10);
    vertices[2] = glm::vec3(+10, -2, -10);
    vertices[3] = glm::vec3(+10, -2, +10);

    /* set triangles */
    auto *triangles = (glm::uvec3 *)rtcSetNewGeometryBuffer(
        mesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(glm::uvec3),
        2);
    triangles[0] = glm::uvec3(0, 1, 2);
    triangles[1] = glm::uvec3(1, 3, 2);

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

int main(void) {
    auto face_colors =
        std::unique_ptr<glm::vec3[], AlignedDeleter<glm::vec3[]>>(
            (glm::vec3 *)aligned_alloc(12 * sizeof(glm::vec3), 16));
    auto vertex_colors =
        std::unique_ptr<glm::vec3[], AlignedDeleter<glm::vec3[]>>(
            (glm::vec3 *)aligned_alloc(12 * sizeof(glm::vec3), 16));

    auto device = rtcNewDevice("verbose=1");
    auto scene = rtcNewScene(device);

    auto cube = addCube(device, scene, vertex_colors.get(), face_colors.get());
    auto plane = addGroundPlane(device, scene);

    auto raytracer = RayTracer();
    auto camera = RayTracerCamera(640, 480, 120.0f, 0.001f, 1000.0f);

    rtcCommitScene(scene);

    const auto width = camera.getWidth();
    const auto height = camera.getHeight();
    auto pixels = std::make_unique<glm::u8vec3[]>(width * height);

    raytracer.render(scene, camera, vertex_colors.get(), face_colors.get(),
                     pixels.get());

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

    const auto windowWidth = width;
    const auto windowHeight = height;
    const auto window =
        glfwCreateWindow(windowWidth, windowHeight, "Hello Embree", NULL, NULL);
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

    copyPixelsToTexture(pixels.get(), fbo, texture, width, height);

    glViewport(0, 0, windowWidth, windowHeight);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    while (!glfwWindowShouldClose(window)) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

		const auto eye = glm::vec3(1.5f, 1.5f, -1.5f);
        const auto target = glm::vec3(0.0f, 0.0f, 0.0f);
        const auto up = glm::vec3(0.0f, 1.0f, 0.0f);
        camera.lookAt(eye, target, up);
        raytracer.render(scene, camera, vertex_colors.get(), face_colors.get(),
                         pixels.get());
        copyPixelsToTexture(pixels.get(), fbo, texture, width, height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
        glBlitFramebuffer(0, 0, width, height, 0, 0, windowWidth, windowHeight,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &texture);

    glfwTerminate();

    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
    return 0;
}