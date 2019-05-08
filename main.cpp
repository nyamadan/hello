#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>

#include <tbb/parallel_for.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <embree3/rtcore.h>
#include <embree3/rtcore_common.h>
#include <embree3/rtcore_device.h>
#include <embree3/rtcore_geometry.h>
#include <embree3/rtcore_ray.h>

#ifdef _WIN32
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

const auto TILE_SIZE_X = 32;
const auto TILE_SIZE_Y = 32;

/* adds a cube to the scene */
unsigned int addCube(RTCDevice device, RTCScene scene,
                     glm::vec3 vertex_colors[], glm::vec3 face_colors[]) {
  /* create a triangulated cube with 12 triangles and 8 vertices */
  auto mesh = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

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
      mesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3), 4);
  vertices[0] = glm::vec3(-10, -2, -10);
  vertices[1] = glm::vec3(-10, -2, +10);
  vertices[2] = glm::vec3(+10, -2, -10);
  vertices[3] = glm::vec3(+10, -2, +10);

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

glm::vec3 renderPixelStandard(RTCScene scene, const glm::vec3 vertex_colors[],
                              const glm::vec3 face_colors[], float x, float y,
                              float width, float height, glm::vec3 cameraFrom,
                              glm::vec3 cameraDir) {
  const auto fov = 120.0f;
  const auto tnear = 0.001f;
  const auto tfar = 1000.0f;

  const auto side =
      glm::normalize(glm::cross(cameraDir, glm::vec3(0.0f, 1.0f, 0.0f)));
  const auto up = glm::normalize(glm::cross(side, cameraDir));

  const auto t = std::tanf(glm::radians(fov) * 0.5f);
  const auto rayDir =
      glm::normalize(t * width / height * (x / width - 0.5f) * side +
                     t * (y / height - 0.5f) * up + cameraDir);

  RTCIntersectContext context;
  rtcInitIntersectContext(&context);

  /* initialize ray */
  auto ray = RTCRayHit();
  ray.ray.dir_x = rayDir.x;
  ray.ray.dir_y = rayDir.y;
  ray.ray.dir_z = rayDir.z;
  ray.ray.org_x = cameraFrom.x;
  ray.ray.org_y = cameraFrom.y;
  ray.ray.org_z = cameraFrom.z;
  ray.ray.tnear = tnear;
  ray.ray.tfar = tfar;
  ray.ray.time = 0.0f;
  ray.hit.geomID = RTC_INVALID_GEOMETRY_ID;

  /* intersect ray with scene */
  rtcIntersect1(scene, &context, &ray);

  /* shade pixels */
  auto color = glm::vec3(0.0f);
  if (ray.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
    glm::vec3 diffuse = face_colors[ray.hit.primID];
    color = color + diffuse * 0.5f;
    glm::vec3 lightDir = glm::normalize(glm::vec3(-1, -1, -1));

    /* initialize shadow ray */
    glm::vec3 shadowOrg = cameraFrom + ray.ray.tfar * rayDir;
    RTCRay shadow;
    shadow.org_x = ray.ray.org_x + ray.ray.tfar * ray.ray.dir_x;
    shadow.org_y = ray.ray.org_y + ray.ray.tfar * ray.ray.dir_y;
    shadow.org_z = ray.ray.org_z + ray.ray.tfar * ray.ray.dir_z;
    shadow.dir_x = -lightDir.x;
    shadow.dir_y = -lightDir.y;
    shadow.dir_z = -lightDir.z;
    shadow.tnear = tnear;
    shadow.tfar = tfar;
    shadow.time = 0.0f;

    /* trace shadow ray */
    rtcOccluded1(scene, &context, &shadow);

    /* add light contribution */
    if (shadow.tfar >= 0.0f) {
      auto Ng = glm::vec3(ray.hit.Ng_x, ray.hit.Ng_y, ray.hit.Ng_z);
      color =
          color + diffuse * std::clamp(-glm::dot(lightDir, glm::normalize(Ng)),
                                       0.0f, 1.0f);
    }
  }
  return color;
}

/* renders a single screen tile */
void renderTileStandard(int tileIndex, RTCScene scene,
                        const glm::vec3 vertex_colors[],
                        const glm::vec3 face_colors[], glm::u8vec3 *pixels,
                        const unsigned int width, const unsigned int height,
                        const glm::vec3 cameraFrom, const glm::vec3 cameraDir,
                        const int numTilesX, const int numTilesY) {
  const unsigned int tileY = tileIndex / numTilesX;
  const unsigned int tileX = tileIndex - tileY * numTilesX;
  const unsigned int x0 = tileX * TILE_SIZE_X;
  const unsigned int x1 = std::min(x0 + TILE_SIZE_X, width);
  const unsigned int y0 = tileY * TILE_SIZE_Y;
  const unsigned int y1 = std::min(y0 + TILE_SIZE_Y, height);

  for (unsigned int y = y0; y < y1; y++)
    for (unsigned int x = x0; x < x1; x++) {
      /* calculate pixel color */
      glm::vec3 color =
          renderPixelStandard(scene, vertex_colors, face_colors, (float)x,
                              (float)y, width, height, cameraFrom, cameraDir);

      /* write color to framebuffer */
      unsigned int r = (unsigned int)(255.0f * std::clamp(color.x, 0.0f, 1.0f));
      unsigned int g = (unsigned int)(255.0f * std::clamp(color.y, 0.0f, 1.0f));
      unsigned int b = (unsigned int)(255.0f * std::clamp(color.z, 0.0f, 1.0f));
      pixels[y * width + x] = glm::u8vec3(r, g, b);
    }
}

/* called by the C++ code to render */
void device_render(RTCScene scene, const glm::vec3 vertex_colors[],
                   const glm::vec3 face_colors[], glm::u8vec3 *pixels,
                   const uint32_t width, const uint32_t height) {
  const auto numTilesX = (width + TILE_SIZE_X - 1) / TILE_SIZE_X;
  const auto numTilesY = (height + TILE_SIZE_Y - 1) / TILE_SIZE_Y;

  const auto cameraFrom = glm::vec3(1.5f, 1.5f, -1.5f);
  const auto cameraTo = glm::vec3(0.0f, 0.0f, 0.0f);
  const auto cameraDir = glm::normalize(cameraTo - cameraFrom);

  tbb::parallel_for(
      size_t(0), size_t(numTilesX * numTilesY), [&](size_t tileIndex) {
        renderTileStandard((int)tileIndex, scene, vertex_colors, face_colors,
                           pixels, width, height, cameraFrom, cameraDir,
                           numTilesX, numTilesY);
      });
}

int main(void) {
  auto face_colors = std::unique_ptr<glm::vec3[], AlignedDeleter<glm::vec3[]>>(
      (glm::vec3 *)aligned_alloc(12 * sizeof(glm::vec3), 16));
  auto vertex_colors =
      std::unique_ptr<glm::vec3[], AlignedDeleter<glm::vec3[]>>(
          (glm::vec3 *)aligned_alloc(12 * sizeof(glm::vec3), 16));

  auto device = rtcNewDevice("verbose=1");
  auto scene = rtcNewScene(device);

  auto cube = addCube(device, scene, vertex_colors.get(), face_colors.get());
  auto plane = addGroundPlane(device, scene);

  rtcCommitScene(scene);

  const auto width = 1024;
  const auto height = 768;
  auto pixels = std::make_unique<glm::u8vec3[]>(width * height);

  device_render(scene, vertex_colors.get(), face_colors.get(), pixels.get(),
                width, height);

  stbi_flip_vertically_on_write(true);
  stbi_write_png("result.png", width, height, 3, pixels.get(), 3 * width);

  rtcReleaseScene(scene);
  rtcReleaseDevice(device);
  return 0;
}