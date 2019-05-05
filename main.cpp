#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <embree3/rtcore.h>
#include <embree3/rtcore_common.h>
#include <embree3/rtcore_device.h>
#include <embree3/rtcore_geometry.h>
#include <embree3/rtcore_ray.h>

#ifdef _MSC_VER
#define aligned_alloc _aligned_malloc
#define aligned_free _aligned_free
#else
#define aligned_free free
#endif

template <typename T = void>
struct AlignedDeleter {
  constexpr AlignedDeleter() noexcept = default;

  template <typename U,
            typename std::enable_if<std::is_convertible<U *, T *>::value,
                                    std::nullptr_t>::type = nullptr>
  AlignedDeleter(const AlignedDeleter<U> &) noexcept {}

  void operator()(T *ptr) const { aligned_free(ptr); }
};

template <typename T>
struct AlignedDeleter<T[]> {
  constexpr AlignedDeleter() noexcept = default;

  template <typename U, typename std::enable_if<
                            std::is_convertible<U (*)[], T (*)[]>::value,
                            std::nullptr_t>::type = nullptr>
  AlignedDeleter(const AlignedDeleter<U[]> &) noexcept {}

  void operator()(T *ptr) const { aligned_free(ptr); }
};

/* adds a cube to the scene */
unsigned int addCube(
    RTCDevice device, RTCScene scene,
    std::unique_ptr<glm::vec3[], AlignedDeleter<glm::vec3[]>> &vertex_colors,
    std::unique_ptr<glm::vec3[], AlignedDeleter<glm::vec3[]>> &face_colors) {
  /* create a triangulated cube with 12 triangles and 8 vertices */
  auto mesh = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

  /* create face and vertex color arrays */
  face_colors.reset((glm::vec3 *)aligned_alloc(12 * sizeof(glm::vec3), 16));
  vertex_colors.reset((glm::vec3 *)aligned_alloc(8 * sizeof(glm::vec3), 16));

  /* set vertices and vertex colors */
  auto *vertices = (glm::vec3 *)rtcSetNewGeometryBuffer(
      mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3), 8);
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
      mesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(glm::uvec3), 12);

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
                             RTC_FORMAT_FLOAT3, vertex_colors.get(), 0,
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
      mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3), 4);
  vertices[0].x = -10;
  vertices[0].y = -2;
  vertices[0].z = -10;
  vertices[1].x = -10;
  vertices[1].y = -2;
  vertices[1].z = +10;
  vertices[2].x = +10;
  vertices[2].y = -2;
  vertices[2].z = -10;
  vertices[3].x = +10;
  vertices[3].y = -2;
  vertices[3].z = +10;

  /* set triangles */
  auto *triangles = (glm::uvec3 *)rtcSetNewGeometryBuffer(
      mesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(glm::uvec3), 2);
  triangles[0] = glm::uvec3(0, 1, 2);
  triangles[1] = glm::uvec3(1, 3, 2);

  rtcCommitGeometry(mesh);
  auto geomID = rtcAttachGeometry(scene, mesh);
  rtcReleaseGeometry(mesh);
  return geomID;
}

int main(void) {
  auto face_colors =
      std::unique_ptr<glm::vec3[], AlignedDeleter<glm::vec3[]>>();
  auto vertex_colors =
      std::unique_ptr<glm::vec3[], AlignedDeleter<glm::vec3[]>>();

  auto device = rtcNewDevice(NULL);
  auto scene = rtcNewScene(device);

  auto cube = addCube(device, scene, vertex_colors, face_colors);
  auto plane = addGroundPlane(device, scene);

  rtcReleaseScene(scene);
  rtcReleaseDevice(device);
  return 0;
}