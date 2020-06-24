#include <stb.h>
#include <tbb/parallel_for.h>
#include <algorithm>
#include <glm/ext.hpp>
#include <random>

#include "geometry.hpp"
#include "ray_tracer.hpp"

RayTracer::RayTracer() {}

void RayTracer::initIntersectContext(IntersectContext *context) {
    rtcInitIntersectContext(context);
    context->raytracer = this;
    context->depth = 0;
}

void RayTracer::intersectionFilter(
    const struct RTCFilterFunctionNArguments *args) {
    assert(args->N == 1);

    auto context = (IntersectContext *)args->context;
    auto mesh = (Mesh *)args->geometryUserPtr;
    auto material = mesh->getMaterial().get();
    auto depth = context->depth;

    if (material->isLight && depth == 0) {
        args->valid[0] = 0;
    }
}

void RayTracer::setRenderingMode(RenderingMode mode) {
    this->reset();
    this->mode = mode;
}

void RayTracer::setEnableSuperSampling(bool enableSuperSampling) {
    this->enableSuperSamples = enableSuperSampling;
}

RenderingMode RayTracer::getRenderingMode() const { return mode; }

const ImageBuffer &RayTracer::getImage() const { return this->image; }

int32_t RayTracer::getSamples() const {
    return std::min(this->samples, getMaxSamples());
}

void RayTracer::setMaxSamples(int32_t samples) { this->maxSamples = samples; }

int32_t RayTracer::getMaxSamples() const {
    switch (mode) {
        case NORMAL:
        case ALBEDO:
        case EMISSIVE:
            return 1;
        default:
            return this->maxSamples;
    }
}

RayTracer::RayTracer(const glm::i32vec2 &size) { this->resize(size); }

void RayTracer::reset() {
    this->image.reset();
    this->samples = 0;
}

void RayTracer::resize(const glm::i32vec2 &size) {
    this->image.resize(size);
    this->samples = 0;
}

// GGX NDF via importance sampling
glm::vec3 RayTracer::importanceSampleGGX(const glm::vec2 &Xi,
                                         const glm::vec3 &N, float roughness) {
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;

    float phi = 2.0f * M_PI * Xi.x;
    float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (alpha2 - 1.0f) * Xi.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    // from spherical coordinates to cartesian coordinates
    glm::vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    glm::vec3 up = std::abs(N.z) < 0.999f ? glm::vec3(0.0, 0.0, 1.0)
                                          : glm::vec3(1.0, 0.0, 0.0);
    glm::vec3 tangent = glm::normalize(cross(up, N));
    glm::vec3 bitangent = glm::cross(N, tangent);

    glm::vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return glm::normalize(sampleVec);
}

float G1_Smith(float NdotV, float k)
{
    return NdotV / (NdotV * (1.0 - k) + k);
}

float G_Smith(float roughness, float NdotV, float NdotL)
{
    float k = roughness * roughness / 2.0f;
    return G1_Smith(NdotV, k) * G1_Smith(NdotL, k);
}

glm::vec3 RayTracer::radiance(RTCScene scene, const RayTracerCamera &camera,
                              xorshift128plus_state &randomState,
                              IntersectContext context, RTCRayHit &ray,
                              int32_t depth) {
    const auto kDepthLimit = 64;
    const auto kDepth = 5;
    const auto kEPS = 0.001f;

    const auto tnear = camera.getNear();
    const auto tfar = camera.getFar();

    /* intersect ray with scene */
    context.depth = depth;
    rtcIntersect1(scene, &context, &ray);

    const auto rayDir = glm::vec3(ray.ray.dir_x, ray.ray.dir_y, ray.ray.dir_z);
    const auto orig = glm::vec3(ray.ray.org_x, ray.ray.org_y, ray.ray.org_z) +
                      ray.ray.tfar * rayDir;

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

    // const auto Ng = glm::vec3(ray.hit.Ng_x, ray.hit.Ng_y, ray.hit.Ng_z);
    // const auto normal = glm::normalize(glm::dot(rayDir, Ng) < 0 ? Ng : -Ng);

    glm::vec2 uv(0.0f);
    rtcInterpolate0(geom, ray.hit.primID, ray.hit.u, ray.hit.v,
                    RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, glm::value_ptr(uv), 2);

    glm::vec3 normal(0.0f);
    // auto normalTexture = material->normalTexture.get();
    // if (normalTexture != nullptr) {
    //     auto buffer = normalTexture->getBuffer();
    //     auto width = normalTexture->getWidth();
    //     auto height = normalTexture->getHeight();
    //     auto u = static_cast<int32_t>(std::round(uv.x * width));
    //     auto v = static_cast<int32_t>(std::round(uv.y * height));
    //     // TODO: TEXTURE_WRAP
    //     auto index =
    //         std::clamp((v * width + u) % (width * height), 0, width *
    //         height);
    //     const auto &u8color = buffer[index];
    //     normal +=
    //         glm::vec3(u8color.r / 255.0f - 0.5f, u8color.g / 255.0f - 0.5f,
    //                   u8color.b / 255.0f - 0.5f);
    // } else {
    rtcInterpolate0(geom, ray.hit.primID, ray.hit.u, ray.hit.v,
                    RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, glm::value_ptr(normal),
                    3);
    // }
    normal = glm::normalize(normal);

    auto incomingRadiance = glm::vec3(0.0f);
    auto weight = glm::vec3(1.0f);

    // glm::vec3 u, v;
    // if (fabs(normal.x) > kEPS) {
    //     u = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), normal));
    // } else {
    //     u = glm::normalize(glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), normal));
    // }
    // v = glm::cross(normal, u);
    // const float r1 = 2.0f * M_PI *
    // static_cast<float>(xorshift128plus01(randomState)); const float r2 =
    // static_cast<float>(xorshift128plus01(randomState)), r2s = std::sqrt(r2);
    // auto H = glm::normalize(glm::vec3(u * cos(r1) * r2s + v * sin(r1) * r2s +
    // normal * sqrt(1.0 - r2)));

    auto roughness = 0.5f;
    const auto &N = normal;
    const auto V = -rayDir;
    const auto Xi = glm::vec2(xorshift128plus01(randomState),
                              xorshift128plus01(randomState));
    const auto H = importanceSampleGGX(Xi, N, roughness);
    const auto L = 2.0f * glm::dot(V, H) * H - V;

    const auto NoV = std::clamp(glm::dot(N, V), 0.0f, 1.0f);
    const auto NoL = std::clamp(glm::dot(N, L), 0.0f ,1.0f);
    const auto NoH = std::clamp(glm::dot(N, H), 0.0f, 1.0f);
    const auto VoH = std::clamp(glm::dot(V, H), 0.0f, 1.0f);

    /* initialize ray */
    auto nextRay = RTCRayHit();
    nextRay.ray.dir_x = H.x;
    nextRay.ray.dir_y = H.y;
    nextRay.ray.dir_z = H.z;
    nextRay.ray.org_x = orig.x;
    nextRay.ray.org_y = orig.y;
    nextRay.ray.org_z = orig.z;
    nextRay.ray.tnear = tnear;
    nextRay.ray.tfar = tfar;
    nextRay.ray.time = 0.0f;
    nextRay.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    incomingRadiance =
        radiance(scene, camera, randomState, context, nextRay, depth + 1);


    glm::vec4 baseColor = material->baseColorFactor;
    auto baseColorTexture = material->baseColorTexture.get();
    if (baseColorTexture != nullptr) {
        auto buffer = baseColorTexture->getBuffer();
        auto width = baseColorTexture->getWidth();
        auto height = baseColorTexture->getHeight();
        auto u = static_cast<int32_t>(std::round(uv.x * width));
        auto v = static_cast<int32_t>(std::round(uv.y * height));
        // TODO: TEXTURE_WRAP
        if (u < 0.0f) {
            u = width + u;
        }
        u = u % width;
        if (v < 0.0f) {
            v = height + v;
        }
        v = v % height;
        auto index = v * width + u;

        const auto &u8color = buffer[index];
        const auto color = glm::vec4(u8color.r / 255.0f, u8color.g / 255.0f,
                                     u8color.b / 255.0f, u8color.a / 255.0f);
        baseColor = baseColor * color;
    }

    glm::vec3 emissive = material->emissiveFactor;
    auto emissiveTexture = material->emissiveTexture.get();
    if (emissiveTexture != nullptr) {
        auto buffer = emissiveTexture->getBuffer();
        auto width = emissiveTexture->getWidth();
        auto height = emissiveTexture->getHeight();
        auto u = static_cast<int32_t>(std::round(uv.x * width));
        auto v = static_cast<int32_t>(std::round(uv.y * height));
        // TODO: TEXTURE_WRAP
        if (u < 0.0f) {
            u = width + u;
        }
        u = u % width;
        if (v < 0.0f) {
            v = height + v;
        }
        v = v % height;
        auto index = v * width + u;
        const auto &u8color = buffer[index];
        const auto color = glm::vec3(u8color.r / 255.0f, u8color.g / 255.0f,
                                     u8color.b / 255.0f);
        emissive = emissive * color;
    }

    if (NoL > 0) {
        auto G = G_Smith(roughness, NoV, NoL);
        auto Fc = pow(1 - VoH, 5);
        auto F = (1 - Fc) * glm::vec3(baseColor) + Fc;

        // Incident light = SampleColor * NoL
        // Microfacet specular = D*G*F / (4*NoL*NoV)
        // pdf = D * NoH / (4 * VoH)
        weight = (F * G * VoH / (NoH * NoV + 1e-6)) / russianRouletteProbability;
    }

    // weight = baseColor / russianRouletteProbability;

    return emissive + weight * incomingRadiance;
}

glm::vec3 RayTracer::renderAlbedo(RTCScene scene, const RayTracerCamera &camera,
                                  float x, float y) {
    auto totalColor = glm::vec3(0.0f);
    const auto tnear = camera.getNear();
    const auto tfar = camera.getFar();
    const auto cameraFrom = camera.getCameraOrigin();
    const auto width = image.getWidth();
    const auto height = image.getHeight();

    const auto samples = enableSuperSamples ? 2 : 1;
    for (int sy = 0; sy < samples; sy++) {
        for (int sx = 0; sx < samples; sx++) {
            const auto rate = 1.0f / samples;
            const auto r1 = (sx * rate + rate / 2.0f);
            const auto r2 = (sy * rate + rate / 2.0f);
            const auto rayDir =
                camera.getRayDir(x + r1 / height, y + r2 / height);

            IntersectContext context;
            initIntersectContext(&context);

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

            if (ray.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
                continue;
            }

            auto geom = rtcGetGeometry(scene, ray.hit.geomID);
            auto mesh = (const Mesh *)rtcGetGeometryUserData(geom);
            auto material = mesh->getMaterial().get();

            glm::vec2 uv(0.0f);
            rtcInterpolate0(geom, ray.hit.primID, ray.hit.u, ray.hit.v,
                            RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1,
                            glm::value_ptr(uv), 2);

            glm::vec4 baseColor = material->baseColorFactor;
            auto baseColorTexture = material->baseColorTexture.get();
            if (baseColorTexture != nullptr) {
                auto buffer = baseColorTexture->getBuffer();
                auto width = baseColorTexture->getWidth();
                auto height = baseColorTexture->getHeight();
                auto u = static_cast<int32_t>(std::round(uv.x * width));
                auto v = static_cast<int32_t>(std::round(uv.y * height));
                // TODO: TEXTURE_WRAP
                if (u < 0.0f) {
                    u = width + u;
                }
                u = u % width;
                if (v < 0.0f) {
                    v = height + v;
                }
                v = v % height;
                auto index = v * width + u;
                const auto &u8color = buffer[index];
                const auto color =
                    glm::vec4(u8color.r / 255.0f, u8color.g / 255.0f,
                              u8color.b / 255.0f, u8color.a / 255.0f);
                baseColor = baseColor * color;
            }
            totalColor += glm::vec3(baseColor);
        }
    }

    return totalColor / (samples * samples);
}

glm::vec3 RayTracer::renderNormal(RTCScene scene, const RayTracerCamera &camera,
                                  float x, float y) {
    const auto tnear = camera.getNear();
    const auto tfar = camera.getFar();
    const auto cameraFrom = camera.getCameraOrigin();
    const auto rayDir = camera.getRayDir(x, y);

    IntersectContext context;
    initIntersectContext(&context);

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

    if (ray.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return glm::vec3(0.0f);
    }

    // const auto Ng = glm::vec3(ray.hit.Ng_x, ray.hit.Ng_y, ray.hit.Ng_z);
    // return glm::normalize(Ng);

    auto geom = rtcGetGeometry(scene, ray.hit.geomID);
    auto mesh = (const Mesh *)rtcGetGeometryUserData(geom);
    auto material = mesh->getMaterial().get();

    glm::vec2 uv(0.0f);
    rtcInterpolate0(geom, ray.hit.primID, ray.hit.u, ray.hit.v,
                    RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, glm::value_ptr(uv), 2);

    glm::vec3 normal(0.0f);
    // auto normalTexture = material->normalTexture.get();
    // if (normalTexture != nullptr) {
    //     auto buffer = normalTexture->getBuffer();
    //     auto width = normalTexture->getWidth();
    //     auto height = normalTexture->getHeight();
    //     auto u = static_cast<int32_t>(std::round(uv.x * width));
    //     auto v = static_cast<int32_t>(std::round(uv.y * height));
    //     // TODO: TEXTURE_WRAP
    //     auto index =
    //         std::clamp((v * width + u) % (width * height), 0, width *
    //         height);
    //     const auto &u8color = buffer[index];
    //     normal +=
    //         glm::vec3(u8color.r / 255.0f - 0.5f, u8color.g / 255.0f - 0.5f,
    //                   u8color.b / 255.0f - 0.5f);
    // } else {
    rtcInterpolate0(geom, ray.hit.primID, ray.hit.u, ray.hit.v,
                    RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, glm::value_ptr(normal),
                    3);
    // }
    normal = glm::normalize(normal);

    return normal;
}

glm::vec3 RayTracer::renderEmissive(RTCScene scene,
                                    const RayTracerCamera &camera, float x,
                                    float y) {
    const auto tnear = camera.getNear();
    const auto tfar = camera.getFar();
    const auto cameraFrom = camera.getCameraOrigin();
    const auto rayDir = camera.getRayDir(x, y);

    IntersectContext context;
    initIntersectContext(&context);

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

    if (ray.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        return glm::vec3(0.0f);
    }

    // const auto Ng = glm::vec3(ray.hit.Ng_x, ray.hit.Ng_y, ray.hit.Ng_z);
    // return glm::normalize(Ng);

    auto geom = rtcGetGeometry(scene, ray.hit.geomID);
    auto mesh = (const Mesh *)rtcGetGeometryUserData(geom);
    auto material = mesh->getMaterial().get();

    glm::vec2 uv(0.0f);
    rtcInterpolate0(geom, ray.hit.primID, ray.hit.u, ray.hit.v,
                    RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, glm::value_ptr(uv), 2);

    glm::vec3 emissive = material->emissiveFactor;
    auto emissiveTexture = material->emissiveTexture.get();
    if (emissiveTexture != nullptr) {
        auto buffer = emissiveTexture->getBuffer();
        auto width = emissiveTexture->getWidth();
        auto height = emissiveTexture->getHeight();
        auto u = static_cast<int32_t>(std::round(uv.x * width));
        auto v = static_cast<int32_t>(std::round(uv.y * height));
        // TODO: TEXTURE_WRAP
        if (u < 0.0f) {
            u = width + u;
        }
        u = u % width;
        if (v < 0.0f) {
            v = height + v;
        }
        v = v % height;
        auto index = v * width + u;
        const auto &u8color = buffer[index];
        const auto color = glm::vec3(u8color.r / 255.0f, u8color.g / 255.0f,
                                     u8color.b / 255.0f);
        emissive = emissive * color;
    }

    return emissive;
}

glm::vec3 RayTracer::renderPathTrace(RTCScene scene,
                                     const RayTracerCamera &camera,
                                     xorshift128plus_state &randomState,
                                     float x, float y) {
    auto totalRadiance = glm::vec3(0.0f);
    const auto tnear = camera.getNear();
    const auto tfar = camera.getFar();
    const auto cameraFrom = camera.getCameraOrigin();
    const auto width = image.getWidth();
    const auto height = image.getHeight();

    const auto samples = enableSuperSamples ? 2 : 1;
    for (int sy = 0; sy < samples; sy++) {
        for (int sx = 0; sx < samples; sx++) {
            const auto rate = 1.0f / samples;
            const auto r1 = (sx * rate + rate / 2.0f);
            const auto r2 = (sy * rate + rate / 2.0f);
            const auto rayDir =
                camera.getRayDir(x + r1 / height, y + r2 / height);

            IntersectContext context;
            initIntersectContext(&context);

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
            totalRadiance +=
                radiance(scene, camera, randomState, context, ray, 0);
        }
    }

    return totalRadiance / (samples * samples);
}

glm::vec3 RayTracer::renderPixel(RTCScene scene, const RayTracerCamera &camera,
                                 xorshift128plus_state &randomState, float x,
                                 float y) {
    switch (mode) {
        case PATHTRACING: {
            return renderPathTrace(scene, camera, randomState, x, y);
        } break;
        case NORMAL: {
            return renderNormal(scene, camera, x, y);
        } break;
        case ALBEDO: {
            return renderAlbedo(scene, camera, x, y);
        } break;
        case EMISSIVE: {
            return renderEmissive(scene, camera, x, y);
        } break;
        default: {
            return renderAlbedo(scene, camera, x, y);
        } break;
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

bool RayTracer::render(RTCScene scene, const RayTracerCamera &camera,
                       oidn::DeviceRef denoiser) {
    if (getSamples() >= getMaxSamples()) {
        return false;
    }

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

    if (this->samples == 0) {
        auto bufferSize = static_cast<int64_t>(
            image.getWidth() * image.getHeight() * sizeof(glm::vec3));
        auto origMode = mode;

        mode = NORMAL;
        tbb::parallel_for(size_t(0), size_t(tileSize), [&](size_t tileIndex) {
            renderTile(scene, camera, randomStates[tileIndex],
                       static_cast<int>(tileIndex), numTilesX, numTilesY);
        });
        memcpy(image.getNormal(), image.getBuffer(), bufferSize);

        mode = ALBEDO;
        tbb::parallel_for(size_t(0), size_t(tileSize), [&](size_t tileIndex) {
            renderTile(scene, camera, randomStates[tileIndex],
                       static_cast<int>(tileIndex), numTilesX, numTilesY);
        });
        memcpy(image.getAlbedo(), image.getBuffer(), bufferSize);

        memset(image.getBuffer(), 0, bufferSize);

        mode = origMode;
    }

    tbb::parallel_for(size_t(0), size_t(tileSize), [&](size_t tileIndex) {
        renderTile(scene, camera, randomStates[tileIndex],
                   static_cast<int>(tileIndex), numTilesX, numTilesY);
    });

    this->samples += 1;

    bool done = false;

    if (this->getSamples() >= this->getMaxSamples() && mode != NORMAL &&
        mode != ALBEDO && mode != EMISSIVE) {
        auto bufferSize =
            static_cast<uint64_t>(image.getWidth() * image.getHeight()) *
            sizeof(glm::vec3);
        auto filter = denoiser.newFilter("RT");

        auto temp = std::make_unique<glm::vec3[]>(bufferSize);
        memcpy(temp.get(), image.getBuffer(), bufferSize);

        filter.setImage("color", temp.get(), oidn::Format::Float3, width,
                        height);
        filter.setImage("albedo", image.getAlbedo(), oidn::Format::Float3,
                        width, height);
        filter.setImage("normal", image.getNormal(), oidn::Format::Float3,
                        width, height);
        filter.setImage("output", image.getBuffer(), oidn::Format::Float3,
                        width, height);
        filter.set("hdr", true);  // image is HDR
        filter.commit();

        // Filter the image
        filter.execute();

        done = true;
    }

    this->image.updateTextureBuffer();
    return done;
}
