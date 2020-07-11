#include <stb.h>
#include <stb_image.h>
#include <tbb/parallel_for.h>
#include <algorithm>
#include <glm/ext.hpp>
#include <random>

#include "color.hpp"
#include "mesh.hpp"
#include "ray_tracer.hpp"

template <typename T>
T nearest(const glm::vec2 &uv, const T *image, int32_t width,
                  int32_t height) {
    auto u = static_cast<int32_t>(glm::round((width - 1) * uv.x));
    auto v = static_cast<int32_t>(glm::round((height - 1) * uv.y));

    return image[v * width + u];
}

template <typename T>
T bilinear(const glm::vec2 &uv, const T *image, int32_t width,
                   int32_t height) {
    auto u = uv.x * (width - 1);
    auto v = uv.y * (height - 1);
    auto u0 = static_cast<int32_t>(glm::floor(u));
    auto u1 = static_cast<int32_t>(glm::ceil(u));
    auto v0 = static_cast<int32_t>(glm::floor(v));
    auto v1 = static_cast<int32_t>(glm::ceil(v));

    auto x = u - static_cast<float>(u0);
    auto y = v - static_cast<float>(v0);
    auto p0 = glm::lerp(image[v0 * width + u0], image[v0 * width + u1], x);
    auto p1 = glm::lerp(image[v1 * width + u0], image[v1 * width + u1], x);
    return glm::lerp(p0, p1, y);
}

glm::vec2 RayTracer::toRadialCoords(glm::vec3 coords) {
    auto normalizedCoords = glm::normalize(coords);
    auto latitude = std::acos(-normalizedCoords.y);
    auto longitude = std::atan2(normalizedCoords.z, -normalizedCoords.x);
    auto sphereCoords =
        glm::vec2(longitude, latitude) * glm::vec2(0.5f / M_PI, 1.0f / M_PI);
    return glm::vec2(0.5f, 1.0f) - sphereCoords;
}

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
}

void RayTracer::setRenderingMode(RenderingMode mode) {
    this->reset();
    this->mode = mode;
}

void RayTracer::loadSkybox(const std::string &path) {
    int channels;
    auto image =
        stbi_loadf(path.c_str(), &skyboxWidth, &skyboxHeight, &channels, 3);
    const auto length = skyboxWidth * skyboxHeight;
    this->skybox = std::shared_ptr<glm::vec3>(new glm::vec3[length]);
    memcpy(this->skybox.get(), image, sizeof(glm::vec3) * length);
    stbi_image_free(image);

    for (auto i = 0; i < length; i++) {
        auto &x = this->skybox.get()[i];
        x = toneMapping(x);
    }
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
    glm::vec3 up = glm::abs(N.z) < 0.999f ? glm::vec3(0.0, 0.0, 1.0)
                                          : glm::vec3(1.0, 0.0, 0.0);
    glm::vec3 tangent = glm::normalize(glm::cross(up, N));
    glm::vec3 bitangent = glm::cross(N, tangent);
    glm::vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return glm::normalize(sampleVec);
}

float G1_Smith(float NdotV, float k) {
    return NdotV / (NdotV * (1.0f - k) + k);
}

float G_Smith(float roughness, float NdotV, float NdotL) {
    float k = roughness * roughness / 2.0f;
    return G1_Smith(NdotV, k) * G1_Smith(NdotL, k);
}

glm::vec3 RayTracer::radiance(RTCScene scene, const RayTracerCamera &camera,
                              xorshift128plus_state &randomState,
                              IntersectContext context, RTCRayHit &ray,
                              int32_t depth) {
    const auto kDepthLimit = 64;
    const auto kEPS = 0.001f;

    const auto tnear = camera.getNear();
    const auto tfar = camera.getFar();

    /* intersect ray with scene */
    context.depth = depth;
    rtcIntersect1(scene, &context, &ray);

    const auto rayDir = glm::vec3(ray.ray.dir_x, ray.ray.dir_y, ray.ray.dir_z);
    const auto p = glm::vec3(ray.ray.org_x, ray.ray.org_y, ray.ray.org_z) +
                   ray.ray.tfar * rayDir;

    if (ray.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        const auto skybox = this->skybox.get();

        auto uv = toRadialCoords(rayDir);
        return bilinear(uv, skybox, skyboxWidth, skyboxHeight);
    }

    auto geom = rtcGetGeometry(scene, ray.hit.geomID);
    auto mesh = (const Mesh *)rtcGetGeometryUserData(geom);
    auto material = mesh->getMaterial().get();

    glm::vec2 texcoord0(0.0f);
    rtcInterpolate0(geom, ray.hit.primID, ray.hit.u, ray.hit.v,
                    RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, SLOT_TEXCOORD_0, glm::value_ptr(texcoord0), 2);

    auto Ng = glm::vec3(ray.hit.Ng_x, ray.hit.Ng_y, ray.hit.Ng_z);

    glm::vec3 emissive = material->emissiveFactor;
    auto emissiveTexture = material->emissiveTexture.get();
    if (emissiveTexture != nullptr) {
        auto buffer = emissiveTexture->getBuffer();
        auto width = emissiveTexture->getWidth();
        auto height = emissiveTexture->getHeight();
        // TODO: TEXTURE_WRAP
        const auto color = glm::vec3(bilinear(glm::repeat(texcoord0), buffer, width, height));
        emissive = emissive * color;
    }

    if(depth >= kDepthLimit) {
        return emissive;
    }

    glm::vec3 normal(0.0f);
    rtcInterpolate0(geom, ray.hit.primID, ray.hit.u, ray.hit.v,
                    RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, SLOT_NORMAL, glm::value_ptr(normal),
                    3);

    glm::vec3 tangent(0.0f);
    rtcInterpolate0(geom, ray.hit.primID, ray.hit.u, ray.hit.v,
                    RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, SLOT_TANGENT, glm::value_ptr(tangent),
                    3);
    tangent = glm::normalize(tangent);

    glm::vec4 bitangent(0.0f);
    rtcInterpolate0(geom, ray.hit.primID, ray.hit.u, ray.hit.v,
                    RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, SLOT_BITANGENT, glm::value_ptr(bitangent),
                    3);
    bitangent = glm::normalize(bitangent);

    auto normalTexture = material->normalTexture.get();
    if (normalTexture != nullptr) {
        const auto &N = normal;
        auto buffer = normalTexture->getBuffer();
        auto width = normalTexture->getWidth();
        auto height = normalTexture->getHeight();
        // TODO: TEXTURE_WRAP
        const auto mapN = 2.0f * glm::vec3(bilinear(glm::repeat(texcoord0), buffer, width, height)) - 1.0f;
        normal = glm::normalize(glm::mat3(tangent, bitangent, N) * mapN);
    }

    glm::vec4 baseColor = material->baseColorFactor;
    auto baseColorTexture = material->baseColorTexture.get();
    if (baseColorTexture != nullptr) {
        auto buffer = baseColorTexture->getBuffer();
        auto width = baseColorTexture->getWidth();
        auto height = baseColorTexture->getHeight();
        // TODO: TEXTURE_WRAP
        const auto color = bilinear(glm::repeat(texcoord0), buffer, width, height);
        baseColor = baseColor * color;
    }

    auto metalness = material->metalnessFactor;
    auto roughness = material->roughnessFactor;
    auto metalRoughnessTexture = material->metallicRoughnessTexture.get();
    if (metalRoughnessTexture != nullptr) {
        auto buffer = metalRoughnessTexture->getBuffer();
        auto width = metalRoughnessTexture->getWidth();
        auto height = metalRoughnessTexture->getHeight();
        // TODO: TEXTURE_WRAP
        const auto color = glm::vec3(bilinear(glm::repeat(texcoord0), buffer, width, height));
        roughness = roughness * color.y;
        metalness = metalness * color.z;
    }

    auto specular = glm::vec3(0.0f);
    auto diffuse = glm::vec3(0.0f);
    if(xorshift128plus01f(randomState) < metalness){
        const auto &N = normal;
        const auto V = -rayDir;
        const auto Xi = glm::vec2(xorshift128plus01f(randomState),
                                  xorshift128plus01f(randomState));
        const auto H = importanceSampleGGX(Xi, N, roughness);
        const auto L = 2.0f * glm::dot(V, H) * H - V;

        const auto NoV = std::clamp(glm::dot(N, V), 0.0f, 1.0f);
        const auto NoL = std::clamp(glm::dot(N, L), 0.0f, 1.0f);
        const auto NoH = std::clamp(glm::dot(N, H), 0.0f, 1.0f);
        const auto VoH = std::clamp(glm::dot(V, H), 0.0f, 1.0f);

        /* initialize ray */
        auto nextRay = RTCRayHit();
        nextRay.ray.dir_x = L.x;
        nextRay.ray.dir_y = L.y;
        nextRay.ray.dir_z = L.z;
        nextRay.ray.org_x = p.x;
        nextRay.ray.org_y = p.y;
        nextRay.ray.org_z = p.z;
        nextRay.ray.tnear = tnear;
        nextRay.ray.tfar = tfar;
        nextRay.ray.time = 0.0f;
        nextRay.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        auto incomingRadiance =
            radiance(scene, camera, randomState, context, nextRay, depth + 1);

        if (NoL > 0) {
            auto G = G_Smith(roughness, NoV, NoL);
            auto Fc = glm::pow(1 - VoH, 5);
            auto F = (1 - Fc) * glm::vec3(baseColor) + Fc;

            // Incident light = SampleColor * NoL
            // Microfacet specular = D*G*F / (4*NoL*NoV)
            // pdf = D * NoH / (4 * VoH)
            specular = incomingRadiance * (F * G * VoH / (NoH * NoV + 1e-6));
        }
    }
    else {
        const auto &N = normal;
        glm::vec3 u, v;
        if (glm::abs(N.x) > kEPS) {
            u = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), N));
        } else {
            u = glm::normalize(glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), N));
        }
        v = glm::cross(N, u);
        const float r1 = 2.0f * M_PI * xorshift128plus01f(randomState);
        const float r2 = xorshift128plus01f(randomState), r2s = glm::sqrt(r2);
        glm::vec3 L =
            glm::normalize((u * glm::cos(r1) * r2s + v * glm::sin(r1) * r2s + N * glm::sqrt(1.0f - r2)));

        /* initialize ray */
        auto nextRay = RTCRayHit();
        nextRay.ray.dir_x = L.x;
        nextRay.ray.dir_y = L.y;
        nextRay.ray.dir_z = L.z;
        nextRay.ray.org_x = p.x;
        nextRay.ray.org_y = p.y;
        nextRay.ray.org_z = p.z;
        nextRay.ray.tnear = tnear;
        nextRay.ray.tfar = tfar;
        nextRay.ray.time = 0.0f;
        nextRay.hit.geomID = RTC_INVALID_GEOMETRY_ID;
        auto incomingRadiance =
            radiance(scene, camera, randomState, context, nextRay, depth + 1);

        diffuse = glm::vec3(baseColor) * incomingRadiance;
    }

    return emissive + specular + diffuse;
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

            glm::vec2 texcoord0(0.0f);
            rtcInterpolate0(geom, ray.hit.primID, ray.hit.u, ray.hit.v,
                            RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, SLOT_TEXCOORD_0,
                            glm::value_ptr(texcoord0), 2);

            glm::vec4 baseColor = material->baseColorFactor;
            auto baseColorTexture = material->baseColorTexture.get();
            if (baseColorTexture != nullptr) {
                auto buffer = baseColorTexture->getBuffer();
                auto width = baseColorTexture->getWidth();
                auto height = baseColorTexture->getHeight();
                // TODO: TEXTURE_WRAP
                const auto color = bilinear(glm::repeat(texcoord0), buffer, width, height);
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

    glm::vec3 normal(0.0f);
    rtcInterpolate0(geom, ray.hit.primID, ray.hit.u, ray.hit.v,
                    RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, SLOT_NORMAL, glm::value_ptr(normal),
                    3);
    glm::normalize(normal);

    glm::vec2 texcoord0(0.0f);
    rtcInterpolate0(geom, ray.hit.primID, ray.hit.u, ray.hit.v,
                    RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, SLOT_TEXCOORD_0, glm::value_ptr(texcoord0), 2);

    glm::vec3 tangent(0.0f);
    rtcInterpolate0(geom, ray.hit.primID, ray.hit.u, ray.hit.v,
                    RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, SLOT_TANGENT, glm::value_ptr(tangent),
                    3);
    tangent = glm::normalize(tangent);

    glm::vec3 bitangent(0.0f);
    rtcInterpolate0(geom, ray.hit.primID, ray.hit.u, ray.hit.v,
                    RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, SLOT_BITANGENT, glm::value_ptr(bitangent),
                    3);
    bitangent = glm::normalize(bitangent);

    auto normalTexture = material->normalTexture.get();
    if (normalTexture != nullptr) {
        const auto &N = normal;
        auto buffer = normalTexture->getBuffer();
        auto width = normalTexture->getWidth();
        auto height = normalTexture->getHeight();
        // TODO: TEXTURE_WRAP
        const auto mapN = glm::vec3(bilinear(glm::repeat(texcoord0), buffer, width, height)) * 2.0f - 1.0f;
        const auto tbn = glm::mat3(tangent, bitangent, N);
        normal = glm::normalize(tbn * mapN);
    }

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

    glm::vec2 texCoord0(0.0f);
    rtcInterpolate0(geom, ray.hit.primID, ray.hit.u, ray.hit.v,
                    RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, SLOT_TEXCOORD_0, glm::value_ptr(texCoord0), 2);

    glm::vec3 emissive = material->emissiveFactor;
    auto emissiveTexture = material->emissiveTexture.get();
    if (emissiveTexture != nullptr) {
        auto buffer = emissiveTexture->getBuffer();
        auto width = emissiveTexture->getWidth();
        auto height = emissiveTexture->getHeight();
        // TODO: TEXTURE_WRAP
        const auto color = glm::vec3(bilinear(glm::repeat(texCoord0), buffer, width, height));
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
    randomState.a = 1;
    randomState.b = seed();
    std::vector<xorshift128plus_state> randomStates(tileSize);

    for (auto it = randomStates.begin(); it != randomStates.end(); it++) {
        it->a = 1;
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
        memcpy(image.getNormal().get(), image.getBuffer().get(), bufferSize);

        mode = ALBEDO;
        tbb::parallel_for(size_t(0), size_t(tileSize), [&](size_t tileIndex) {
            renderTile(scene, camera, randomStates[tileIndex],
                       static_cast<int>(tileIndex), numTilesX, numTilesY);
        });
        memcpy(image.getAlbedo().get(), image.getBuffer().get(), bufferSize);

        memset(image.getBuffer().get(), 0, bufferSize);

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
        auto length = image.getWidth() * image.getHeight();
        auto bufferSize = static_cast<uint64_t>(length) * sizeof(glm::vec3);
        auto filter = denoiser.newFilter("RT");

        auto temp = std::make_unique<glm::vec3[]>(bufferSize);
        memcpy(temp.get(), image.getBuffer().get(), bufferSize);

        filter.setImage("color", temp.get(), oidn::Format::Float3, width,
                        height);
        filter.setImage("albedo", image.getAlbedo().get(), oidn::Format::Float3,
                        width, height);
        filter.setImage("normal", image.getNormal().get(), oidn::Format::Float3,
                        width, height);
        filter.setImage("output", image.getBuffer().get(), oidn::Format::Float3,
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
