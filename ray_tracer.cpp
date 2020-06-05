#include <stb.h>
#include <tbb/parallel_for.h>
#include <algorithm>
#include <glm/ext.hpp>
#include <random>

#include "geometry.hpp"
#include "ray_tracer.hpp"

RayTracer::RayTracer() {}

const ImageBuffer &RayTracer::getImage() { return this->image; }

RayTracer::RayTracer(const glm::i32vec2 &size) { this->resize(size); }

void RayTracer::resize(const glm::i32vec2 &size) {
    this->image.resize(size);
    this->samples = 0;
}

glm::vec3 RayTracer::radiance(RTCScene scene, const RayTracerCamera &camera,
                              xorshift128plus_state &randomState,
                              RTCIntersectContext context, RTCRayHit &ray,
                              int32_t depth) {
    const auto kDepthLimit = 64;
    const auto kDepth = 5;
    const auto kEPS = 0.001f;

    const auto tnear = camera.getNear();
    const auto tfar = camera.getFar();

    auto result = glm::vec3(0.0f);

    /* intersect ray with scene */
    rtcIntersect1(scene, &context, &ray);

    if (ray.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return glm::vec3(0.0f);
    }

    auto geom = rtcGetGeometry(scene, ray.hit.geomID);
    auto mesh = (const Mesh *)rtcGetGeometryUserData(geom);
    auto material = mesh->getMaterial().get();

    auto russianRouletteProbability = std::max(
        material->baseColorFactor.x,
        std::max(material->baseColorFactor.y, material->baseColorFactor.z));

    if (depth > kDepthLimit) {
        russianRouletteProbability *= pow(0.5f, depth - kDepthLimit);
    }

    if (depth > kDepth) {
        if (xorshift128plus01(randomState) >= russianRouletteProbability) {
            return material->emissiveFactor;
        }
    } else {
        russianRouletteProbability = 1.0f;
    }

    const auto Ng = glm::vec3(ray.hit.Ng_x, ray.hit.Ng_y, ray.hit.Ng_z);
    const auto rayDir = glm::vec3(ray.ray.dir_x, ray.ray.dir_y, ray.ray.dir_z);
    const auto orig = glm::vec3(ray.ray.org_x, ray.ray.org_y, ray.ray.org_z) +
                      ray.ray.tfar * rayDir;
    const auto normal = glm::normalize(glm::dot(rayDir, Ng) < 0 ? Ng : -Ng);

    auto incomingRadiance = glm::vec3(0.0f);
    auto weight = glm::vec3(1.0f);

    glm::vec3 w, u, v;
    w = normal;

    if (fabs(w.x) > kEPS) {
        u = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), w));
    } else {
        u = glm::normalize(glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), w));
    }

    v = glm::cross(w, u);

    const float r1 =
        2.0f * M_PI * static_cast<float>(xorshift128plus01(randomState));
    const float r2 = static_cast<float>(xorshift128plus01(randomState)),
                r2s = std::sqrt(r2);
    auto dir = glm::normalize(
        glm::vec3(u * cos(r1) * r2s + v * sin(r1) * r2s + w * sqrt(1.0 - r2)));

    /* initialize ray */
    auto nextRay = RTCRayHit();
    nextRay.ray.dir_x = dir.x;
    nextRay.ray.dir_y = dir.y;
    nextRay.ray.dir_z = dir.z;
    nextRay.ray.org_x = orig.x;
    nextRay.ray.org_y = orig.y;
    nextRay.ray.org_z = orig.z;
    nextRay.ray.tnear = tnear;
    nextRay.ray.tfar = tfar;
    nextRay.ray.time = 0.0f;
    nextRay.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    incomingRadiance =
        radiance(scene, camera, randomState, context, nextRay, depth + 1);

    weight = material->baseColorFactor / russianRouletteProbability;

    return material->emissiveFactor + weight * incomingRadiance;
}

glm::vec3 RayTracer::renderPixelClassic(RTCScene scene,
                                        const RayTracerCamera &camera,
                                        xorshift128plus_state &randomState,
                                        float x, float y) {
    const auto tnear = camera.getNear();
    const auto tfar = camera.getFar();
    const auto cameraFrom = camera.getCameraOrigin();
    const auto rayDir = camera.getRayDir(x, y);

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

    const auto Ng = glm::vec3(ray.hit.Ng_x, ray.hit.Ng_y, ray.hit.Ng_z);
    const auto orig = cameraFrom + ray.ray.tfar * rayDir;

    /* shade pixels */
    auto color = glm::vec3(0.0f);

    if (ray.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return color;
    }

    auto diffuse = glm::vec3(0.0f);
    auto geom = rtcGetGeometry(scene, ray.hit.geomID);
    auto mesh = (const Mesh *)rtcGetGeometryUserData(geom);
    auto material = mesh->getMaterial().get();

    auto normal = glm::vec3(0.0f);

    normal = glm::normalize(glm::dot(rayDir, Ng) < 0 ? Ng : -Ng);

    // rtcInterpolate0(geom, ray.hit.primID, ray.hit.u, ray.hit.v,
    //                 RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0,
    //                 glm::value_ptr(normal), 3);

    if (material != nullptr) {
        diffuse = material->baseColorFactor;
    } else {
        diffuse = glm::vec3(1.0f);
    }

    color = diffuse * 0.5f;

    glm::vec3 lightDir = glm::normalize(glm::vec3(-1, -1, -1));
    glm::vec3 hitOrig = glm::vec3(ray.ray.org_x + ray.ray.tfar * ray.ray.dir_x,
                                  ray.ray.org_y + ray.ray.tfar * ray.ray.dir_y,
                                  ray.ray.org_z + ray.ray.tfar * ray.ray.dir_z);

    /* initialize shadow ray */
    RTCRay shadow;
    shadow.org_x = hitOrig.x;
    shadow.org_y = hitOrig.y;
    shadow.org_z = hitOrig.z;
    shadow.dir_x = -lightDir.x;
    shadow.dir_y = -lightDir.y;
    shadow.dir_z = -lightDir.z;
    shadow.tnear = tnear;
    shadow.tfar = tfar;
    shadow.time = 0.0f;

    /* trace shadow ray */
    rtcOccluded1(scene, &context, &shadow);

    /* add light contribution */
    if (shadow.tfar == tfar) {
        color = color +
                diffuse * std::clamp(-glm::dot(lightDir, normal), 0.0f, 1.0f);
    }

    {
        glm::vec3 p;
        auto r = 1.0f;
        auto r2 = r * r;
        do {
            p = 2.0f * r *
                    glm::vec3(xorshift128plus01(randomState),
                              xorshift128plus01(randomState),
                              xorshift128plus01(randomState)) -
                glm::vec3(r, r, r);
        } while (glm::length2(p) >= r2);

        auto org = hitOrig;
        auto target = p + r * normal + org;
        auto dir = glm::normalize(target - org);

        RTCRay occ;
        auto tnear = 0.0001f;
        auto tfar = glm::length(target - org);
        occ.org_x = org.x;
        occ.org_y = org.y;
        occ.org_z = org.z;
        occ.dir_x = dir.x;
        occ.dir_y = dir.y;
        occ.dir_z = dir.z;
        occ.tnear = tnear;
        occ.tfar = tfar;
        occ.time = 0.0f;

        /* trace shadow ray */
        rtcOccluded1(scene, &context, &occ);

        if (occ.tfar != tfar) {
            color *= 0.1f;
        }
    }

    return color;
}

glm::vec3 RayTracer::renderPixel(RTCScene scene, const RayTracerCamera &camera,
                                 xorshift128plus_state &randomState, float x,
                                 float y) {
    const auto isClassic = false;
    if (isClassic) {
        return renderPixelClassic(scene, camera, randomState, x, y);
    } else {
        const auto tnear = camera.getNear();
        const auto tfar = camera.getFar();
        const auto cameraFrom = camera.getCameraOrigin();
        const auto rayDir = camera.getRayDir(x, y);

        RTCIntersectContext context;
        rtcInitIntersectContext(&context);

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

        return radiance(scene, camera, randomState, context, ray, 0);
    }
}

/* renders a single screen tile */
void RayTracer::renderTile(RTCScene scene, const RayTracerCamera &camera,
                           xorshift128plus_state &randomState, int tileIndex,
                           int numTilesX, int numTilesY) {
    const auto width = this->image.getWidth();
    const auto height = this->image.getHeight();
    const auto aspect = this->image.getAspect();
    const auto pixels = this->image.getBuffer();
    const auto tileY = tileIndex / numTilesX;
    const auto tileX = tileIndex - tileY * numTilesX;
    const auto x0 = tileX * TILE_SIZE_X;
    const auto x1 = std::min(x0 + TILE_SIZE_X, width);
    const auto y0 = tileY * TILE_SIZE_Y;
    const auto y1 = std::min(y0 + TILE_SIZE_Y, height);

    for (auto y = y0; y < y1; y++) {
        for (auto x = x0; x < x1; x++) {
            auto result = renderPixel(scene, camera, randomState,
                                      ((float)x / (float)width - 0.5f) * aspect,
                                      (float)y / (float)height - 0.5f);
            auto index = y * width + x;
            pixels[index] =
                (pixels[index] * this->samples + result) / (this->samples + 1);
        }
    }
}

void RayTracer::render(RTCScene scene, const RayTracerCamera &camera) {
    const auto width = this->image.getWidth();
    const auto height = this->image.getHeight();
    const auto numTilesX = (width + TILE_SIZE_X - 1) / TILE_SIZE_X;
    const auto numTilesY = (height + TILE_SIZE_Y - 1) / TILE_SIZE_Y;
    const auto tileSize = numTilesX * numTilesY;

    std::random_device seed;
    xorshift128plus_state randomState;
    randomState.a = seed();
    randomState.b = 0;
    std::vector<xorshift128plus_state> randomStates(tileSize);

    for (auto it = randomStates.begin(); it != randomStates.end(); it++) {
        it->a = xorshift128plus(randomState);
        it->b = xorshift128plus(randomState);
    }

    tbb::parallel_for(size_t(0), size_t(tileSize), [&](size_t tileIndex) {
        renderTile(scene, camera, randomStates[tileIndex],
                   static_cast<int>(tileIndex), numTilesX, numTilesY);
    });

    this->image.updateTextureBuffer();

    this->samples += 1;
}
