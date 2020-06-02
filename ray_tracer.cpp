#include <tbb/parallel_for.h>
#include <algorithm>
#include <glm/ext.hpp>
#include <random>
#include <stb.h>

#include "geometry.hpp"
#include "ray_tracer.hpp"

glm::vec3 RayTracer::radiance(RTCScene scene, const RayTracerCamera &camera,
                              xorshift128plus_state &randomState,
                              RTCRayHit &ray, int32_t depth) {
    const auto cDepthLimit = 64;
    const auto cDepth = 5;
    const auto cEPS = 0.001f;

    const auto tnear = camera.getNear();
    const auto tfar = camera.getFar();

    auto result = glm::vec3(0.0f);

    RTCIntersectContext context;
    rtcInitIntersectContext(&context);

    /* intersect ray with scene */
    rtcIntersect1(scene, &context, &ray);

    if (ray.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return glm::vec3(0.0f);
    }

    auto geom = rtcGetGeometry(scene, ray.hit.geomID);
    auto material = *(const Material *)rtcGetGeometryUserData(geom);

    auto russianRouletteProbability = std::max(
        material.baseColorFactor.x,
        std::max(material.baseColorFactor.y, material.baseColorFactor.z));

    if (depth > cDepthLimit) {
        russianRouletteProbability *= pow(0.5f, depth - cDepthLimit);
    }

    if(depth > cDepth) {
        if(xorshift128plus01(randomState) >= russianRouletteProbability) {
            return material.emissiveFactor;
        }
    } else {
        russianRouletteProbability = 1.0f;
    }

    const auto Ng = glm::vec3(ray.hit.Ng_x, ray.hit.Ng_y, ray.hit.Ng_z);
    const auto orig =
        glm::vec3(ray.ray.org_x, ray.ray.org_y, ray.ray.org_z) +
        ray.ray.tfar * glm::vec3(ray.ray.dir_x, ray.ray.dir_y, ray.ray.dir_z);
    const auto normal = glm::normalize(Ng);

    auto incomingRadiance = glm::vec3(0.0f);
    auto weight = glm::vec3(1.0f);

    glm::vec3 w, u, v;
    w = normal;

    if (fabs(w.x) > cEPS) 
    {
        u = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), w));
    } else {
        u = glm::normalize(glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), w));
    }

    v = glm::cross(w, u);

    const float r1 = 2.0f * M_PI * static_cast<float>(xorshift128plus01(randomState));
    const float r2 = static_cast<float>(xorshift128plus01(randomState)), r2s = sqrt(r2);
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
    incomingRadiance = radiance(scene, camera, randomState, nextRay, depth + 1);
    // レンダリング方程式に対するモンテカルロ積分を考えると、outgoing_radiance =
    // weight * incoming_radiance。 ここで、weight = (ρ/π) * cosθ / pdf(ω) / R
    // になる。
    // ρ/πは完全拡散面のBRDFでρは反射率、cosθはレンダリング方程式におけるコサイン項、pdf(ω)はサンプリング方向についての確率密度関数。
    // Rはロシアンルーレットの確率。
    // 今、コサイン項に比例した確率密度関数によるサンプリングを行っているため、pdf(ω)
    // = cosθ/π よって、weight = ρ/ R。
    weight = material.baseColorFactor / russianRouletteProbability;

    return result;
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
    const auto normal = glm::normalize(Ng);

    /* shade pixels */
    auto color = glm::vec3(0.0f);

    if (ray.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
        auto diffuse = glm::vec3(0.0f);
        auto geom = rtcGetGeometry(scene, ray.hit.geomID);
        auto material = (const Material *)rtcGetGeometryUserData(geom);

        auto normal = glm::vec3(0.0f);

        rtcInterpolate0(geom, ray.hit.primID, ray.hit.u, ray.hit.v,
                        RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0,
                        glm::value_ptr(normal), 3);

        if (material != nullptr) {
            diffuse = material->baseColorFactor;
        } else {
            diffuse = glm::vec3(1.0f);
        }

        color = diffuse * 0.5f;

        glm::vec3 lightDir = glm::normalize(glm::vec3(-1, -1, -1));
        glm::vec3 hitOrig =
            glm::vec3(ray.ray.org_x + ray.ray.tfar * ray.ray.dir_x,
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
            color = color + diffuse * std::clamp(-glm::dot(lightDir, normal),
                                                 0.0f, 1.0f);
        }

        if (aoSample > 0) {
            glm::vec3 p;
            auto r = 1.0f;
            auto r2 = r * r;
            auto hit = 0;
            for (auto i = 0; i < aoSample; i++) {
                do {
                    const auto range = UINT64_MAX;
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
                    hit++;
                }
            }

            color = glm::vec3(color * (1.0f - (float)hit / (float)aoSample));
        }
    }

    return color;
}

glm::vec3 RayTracer::renderPixel(RTCScene scene, const RayTracerCamera &camera,
                                 xorshift128plus_state &randomState, float x,
                                 float y) {
    return renderPixelClassic(scene, camera, randomState, x, y);
}

/* renders a single screen tile */
void RayTracer::renderTile(RTCScene scene, const RayTracerCamera &camera,
                           xorshift128plus_state &randomState,
                           ImageBuffer &image, int tileIndex, int numTilesX,
                           int numTilesY) {
    const auto width = image.getWidth();
    const auto height = image.getHeight();
    const auto aspect = image.getAspect();
    const auto pixels = image.getBuffer();
    const auto tileY = tileIndex / numTilesX;
    const auto tileX = tileIndex - tileY * numTilesX;
    const auto x0 = tileX * TILE_SIZE_X;
    const auto x1 = std::min(x0 + TILE_SIZE_X, width);
    const auto y0 = tileY * TILE_SIZE_Y;
    const auto y1 = std::min(y0 + TILE_SIZE_Y, height);

    for (auto y = y0; y < y1; y++)
        for (auto x = x0; x < x1; x++) {
            /* calculate pixel color */
            glm::vec3 color =
                renderPixel(scene, camera, randomState,
                            ((float)x / (float)width - 0.5f) * aspect,
                            (float)y / (float)height - 0.5f);

            /* write color to framebuffer */
            const auto r = (uint8_t)(255.0f * std::clamp(color.x, 0.0f, 1.0f));
            const auto g = (uint8_t)(255.0f * std::clamp(color.y, 0.0f, 1.0f));
            const auto b = (uint8_t)(255.0f * std::clamp(color.z, 0.0f, 1.0f));
            pixels[y * width + x] = glm::u8vec3(r, g, b);
        }
}

void RayTracer::render(RTCScene scene, const RayTracerCamera &camera,
                       ImageBuffer &image) {
    const auto width = image.getWidth();
    const auto height = image.getHeight();
    const auto numTilesX = (width + TILE_SIZE_X - 1) / TILE_SIZE_X;
    const auto numTilesY = (height + TILE_SIZE_Y - 1) / TILE_SIZE_Y;

    tbb::parallel_for(
        size_t(0), size_t(numTilesX * numTilesY), [&](size_t tileIndex) {
            std::mt19937 mt(static_cast<unsigned int>(tileIndex));
            auto randomInt64 = std::uniform_int<int64_t>(1, INT64_MAX);

            xorshift128plus_state randomState;
            randomState.a = randomInt64(mt);
            randomState.b = 0;

            renderTile(scene, camera, randomState, image,
                       static_cast<int>(tileIndex), numTilesX, numTilesY);
        });
}
