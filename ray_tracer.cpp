#include <algorithm>
#include <tbb/parallel_for.h>
#include "ray_tracer.hpp"

glm::vec3 RayTracer::renderPixelStandard(RTCScene scene,
                                         const glm::vec3 vertex_colors[],
                                         const glm::vec3 face_colors[], float x,
                                         float y, float width, float height,
                                         glm::vec3 cameraFrom,
                                         glm::vec3 cameraDir) {
    const auto fov = 120.0f;
    const auto tnear = 0.001f;
    const auto tfar = 1000.0f;

    const auto side =
        glm::normalize(glm::cross(cameraDir, glm::vec3(0.0f, 1.0f, 0.0f)));
    const auto up = glm::normalize(glm::cross(side, cameraDir));

    const auto t = tanf(glm::radians(fov) * 0.5f);
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
            color = color + diffuse * std::clamp(-glm::dot(lightDir,
                                                           glm::normalize(Ng)),
                                                 0.0f, 1.0f);
        }
    }
    return color;
}

/* renders a single screen tile */
void RayTracer::renderTileStandard(
    int tileIndex, RTCScene scene, const glm::vec3 vertex_colors[],
    const glm::vec3 face_colors[], glm::u8vec3 *pixels,
    const unsigned int width, const unsigned int height,
    const glm::vec3 cameraFrom, const glm::vec3 cameraDir, const int numTilesX,
    const int numTilesY) {
    const unsigned int tileY = tileIndex / numTilesX;
    const unsigned int tileX = tileIndex - tileY * numTilesX;
    const unsigned int x0 = tileX * TILE_SIZE_X;
    const unsigned int x1 = std::min(x0 + TILE_SIZE_X, width);
    const unsigned int y0 = tileY * TILE_SIZE_Y;
    const unsigned int y1 = std::min(y0 + TILE_SIZE_Y, height);

    for (unsigned int y = y0; y < y1; y++)
        for (unsigned int x = x0; x < x1; x++) {
            /* calculate pixel color */
            glm::vec3 color = renderPixelStandard(
                scene, vertex_colors, face_colors, (float)x, (float)y, width,
                height, cameraFrom, cameraDir);

            /* write color to framebuffer */
            unsigned int r =
                (unsigned int)(255.0f * std::clamp(color.x, 0.0f, 1.0f));
            unsigned int g =
                (unsigned int)(255.0f * std::clamp(color.y, 0.0f, 1.0f));
            unsigned int b =
                (unsigned int)(255.0f * std::clamp(color.z, 0.0f, 1.0f));
            pixels[y * width + x] = glm::u8vec3(r, g, b);
        }
}

void RayTracer::render(RTCScene scene, const glm::vec3 vertex_colors[],
                              const glm::vec3 face_colors[],
                              glm::u8vec3 *pixels, const uint32_t width,
                              const uint32_t height) {
    const auto numTilesX = (width + TILE_SIZE_X - 1) / TILE_SIZE_X;
    const auto numTilesY = (height + TILE_SIZE_Y - 1) / TILE_SIZE_Y;

    const auto cameraFrom = glm::vec3(1.5f, 1.5f, -1.5f);
    const auto cameraTo = glm::vec3(0.0f, 0.0f, 0.0f);
    const auto cameraDir = glm::normalize(cameraTo - cameraFrom);

    tbb::parallel_for(
        size_t(0), size_t(numTilesX * numTilesY), [&](size_t tileIndex) {
            renderTileStandard((int)tileIndex, scene, vertex_colors,
                               face_colors, pixels, width, height, cameraFrom,
                               cameraDir, numTilesX, numTilesY);
        });
}
