#include <stb.h>
#include <stb_image.h>
#include <tbb/parallel_for.h>
#include <algorithm>
#include <glm/ext.hpp>
#include <random>

#include "color.hpp"
#include "mesh.hpp"
#include "ray_tracer.hpp"

namespace {
uint64_t murmurHash3(uint64_t h) {
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccd;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53;
    h ^= h >> 33;
    return h;
}

template <typename T>
T srgb2linear(const T &x) {
    return glm::pow(x, 2.2f);
}

glm::vec3 srgb2linear(const glm::vec3 &x) {
    return glm::vec3(srgb2linear(x.r), srgb2linear(x.g), srgb2linear(x.b));
}

glm::vec4 srgb2linear(const glm::vec4 &x) {
    return glm::vec4(srgb2linear(x.r), srgb2linear(x.g), srgb2linear(x.b), x.a);
}

template <typename T>
T nearest(const glm::vec2 &uv, const T *image, int32_t width, int32_t height,
          bool sRGB) {
    auto u = static_cast<int32_t>(glm::round((width - 1) * uv.x));
    auto v = static_cast<int32_t>(glm::round((height - 1) * uv.y));

    const auto &p = image[v * width + u];
    return sRGB ? srgb2linear(p) : p;
}

template <typename T>
T bilinear(const glm::vec2 &uv, const T *image, int32_t width, int32_t height,
           bool sRGB) {
    auto u = uv.x * (width - 1);
    auto v = uv.y * (height - 1);
    auto u0 = static_cast<int32_t>(glm::floor(u));
    auto u1 = static_cast<int32_t>(glm::ceil(u));
    auto v0 = static_cast<int32_t>(glm::floor(v));
    auto v1 = static_cast<int32_t>(glm::ceil(v));

    auto x = u - static_cast<float>(u0);
    auto y = v - static_cast<float>(v0);

    auto p00 = image[v0 * width + u0];
    auto p01 = image[v0 * width + u1];
    auto p10 = image[v1 * width + u0];
    auto p11 = image[v1 * width + u1];

    if (sRGB) {
        p00 = srgb2linear(p00);
        p01 = srgb2linear(p01);
        p10 = srgb2linear(p10);
        p11 = srgb2linear(p11);
    }

    auto p0 = glm::lerp(p00, p01, x);
    auto p1 = glm::lerp(p10, p11, x);
    return glm::lerp(p0, p1, y);
}
}  // namespace

glm::vec2 RayTracer::toRadialCoords(glm::vec3 coords) {
    auto normalizedCoords = glm::normalize(coords);
    auto latitude = std::acos(-normalizedCoords.y);
    auto longitude = std::atan2(normalizedCoords.z, -normalizedCoords.x);
    auto sphereCoords =
        glm::vec2(longitude, latitude) * glm::vec2(0.5f / M_PI, 1.0f / M_PI);
    return glm::vec2(0.5f, 1.0f) - sphereCoords;
}

void RayTracer::initIntersectContext(IntersectContext *context, RTCScene scene,
                                     xorshift128plus_state *randomState) {
    rtcInitIntersectContext(context);
    context->raytracer = this;
    context->randomState = randomState;
    context->scene = scene;
    context->depth = 0;
}

void RayTracer::intersectionFilter(
    const struct RTCFilterFunctionNArguments *args) {
    assert(args->N == 1);

    auto context = (IntersectContext *)args->context;
    auto material = (Material *)args->geometryUserPtr;
    auto depth = context->depth;
    auto scene = context->scene;

    auto Ng = glm::vec3(RTCHitN_Ng_x(args->hit, args->N, 0),
                        RTCHitN_Ng_y(args->hit, args->N, 0),
                        RTCHitN_Ng_z(args->hit, args->N, 0));

    auto rayDir = glm::vec3(RTCRayN_dir_x(args->ray, args->N, 0),
                            RTCRayN_dir_y(args->ray, args->N, 0),
                            RTCRayN_dir_z(args->ray, args->N, 0));

    auto u = RTCHitN_u(args->hit, args->N, 0);
    auto v = RTCHitN_v(args->hit, args->N, 0);

    auto geomId = RTCHitN_geomID(args->hit, args->N, 0);
    auto primId = RTCHitN_primID(args->hit, args->N, 0);

    auto geom = rtcGetGeometry(scene, geomId);

    glm::vec3 normal(0.0f);
    rtcInterpolate0(geom, primId, u, v, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                    SLOT_NORMAL, glm::value_ptr(normal), 3);
    normal = glm::normalize(normal);

    if (material->materialType == REFLECTION) {
        if (glm::dot(normal, rayDir) > 0.0f) {
            args->valid[0] = 0;
            return;
        }
    }

    glm::vec2 texcoord0(0.0f);
    rtcInterpolate0(geom, primId, u, v, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                    SLOT_TEXCOORD_0, glm::value_ptr(texcoord0), 2);
    context->texcoord0 = texcoord0;

    glm::vec4 baseColor = material->baseColorFactor;
    auto baseColorTexture = material->baseColorTexture.get();
    if (baseColorTexture != nullptr) {
        auto buffer = baseColorTexture->getBuffer();
        auto width = baseColorTexture->getWidth();
        auto height = baseColorTexture->getHeight();
        // TODO: TEXTURE_WRAP
        const auto color =
            bilinear(glm::repeat(texcoord0), buffer, width, height, true);
        baseColor = baseColor * color;
    }
    context->baseColor = baseColor;

    if (context->randomState != nullptr) {
        auto &randomState = *context->randomState;
        if (baseColor.a < xorshift128plus01f(randomState)) {
            args->valid[0] = 0;
            return;
        }
    } else {
        if (baseColor.a < 0.5f) {
            args->valid[0] = 0;
            return;
        }
    }

    glm::vec3 emissive = material->emissiveFactor;
    auto emissiveTexture = material->emissiveTexture.get();
    if (emissiveTexture != nullptr) {
        auto buffer = emissiveTexture->getBuffer();
        auto width = emissiveTexture->getWidth();
        auto height = emissiveTexture->getHeight();
        // TODO: TEXTURE_WRAP
        const auto color = glm::vec3(
            bilinear(glm::repeat(texcoord0), buffer, width, height, false));
        emissive = emissive * color;
    }
    context->emissive = emissive;

    glm::vec3 tangent(0.0f);
    rtcInterpolate0(geom, primId, u, v, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                    SLOT_TANGENT, glm::value_ptr(tangent), 3);
    tangent = glm::normalize(tangent);
    context->tangent = tangent;

    glm::vec3 bitangent(0.0f);
    rtcInterpolate0(geom, primId, u, v, RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE,
                    SLOT_BITANGENT, glm::value_ptr(bitangent), 3);
    bitangent = glm::normalize(bitangent);
    context->bitangent = bitangent;

    auto normalTexture = material->normalTexture.get();
    if (normalTexture != nullptr) {
        auto buffer = normalTexture->getBuffer();
        auto width = normalTexture->getWidth();
        auto height = normalTexture->getHeight();
        // TODO: TEXTURE_WRAP
        auto mapN = glm::vec3(bilinear(glm::repeat(texcoord0), buffer, width,
                                       height, false)) *
                        2.0f -
                    1.0f;
        normal = glm::normalize(glm::mat3(tangent, bitangent, normal) * mapN);
    }

    if (glm::isnan(normal.x) || glm::isnan(normal.y) || glm::isnan(normal.z)) {
        normal = glm::normalize(Ng);
    }

    context->normal = normal;

    auto metalness = material->metalnessFactor;
    auto roughness = material->roughnessFactor;
    auto metalRoughnessTexture = material->metallicRoughnessTexture.get();
    if (metalRoughnessTexture != nullptr) {
        auto buffer = metalRoughnessTexture->getBuffer();
        auto width = metalRoughnessTexture->getWidth();
        auto height = metalRoughnessTexture->getHeight();
        // TODO: TEXTURE_WRAP
        const auto color = glm::vec3(
            bilinear(glm::repeat(texcoord0), buffer, width, height, false));
        roughness = roughness * color.y;
        metalness = metalness * color.z;
    }
    context->metalness = metalness;
    context->roughness = roughness;
}

void RayTracer::setRenderingMode(RenderingMode mode) { this->mode = mode; }

void RayTracer::loadSkybox(const std::string &path) {
    int channels;
    stbi_ldr_to_hdr_gamma(1.0f);
    stbi_ldr_to_hdr_scale(1.0f);
    auto image =
        stbi_loadf(path.c_str(), &skyboxWidth, &skyboxHeight, &channels, 3);
    const auto length = skyboxWidth * skyboxHeight;
    this->skybox = std::shared_ptr<glm::vec3>(new glm::vec3[length]);
    memcpy(this->skybox.get(), image, sizeof(glm::vec3) * length);
    stbi_image_free(image);

    // for (auto i = 0; i < length; i++) {
    //     auto &x = this->skybox.get()[i];
    //     x = toneMapping(x);
    // }
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
    switch (this->mode) {
        case PATHTRACING:
            return this->maxSamples;
        default:
            return 1;
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
    float cosTheta = glm::sqrt((1.0f - Xi.y) / (1.0f + (alpha2 - 1.0f) * Xi.y));
    float sinTheta = glm::sqrt(1.0f - cosTheta * cosTheta);

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

glm::vec3 RayTracer::computeDiffuse(RTCScene scene,
                                    const RayTracerCamera &camera,
                                    xorshift128plus_state &randomState,
                                    IntersectContext context, int32_t depth,
                                    const glm::vec4 &baseColor,
                                    const glm::vec3 &p, const glm::vec3 &N) {
    const auto kEPS = 0.001f;
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
        glm::normalize((u * glm::cos(r1) * r2s + v * glm::sin(r1) * r2s +
                        N * glm::sqrt(1.0f - r2)));

    /* initialize ray */
    auto nextRay = RTCRayHit();
    nextRay.ray.dir_x = L.x;
    nextRay.ray.dir_y = L.y;
    nextRay.ray.dir_z = L.z;
    nextRay.ray.org_x = p.x;
    nextRay.ray.org_y = p.y;
    nextRay.ray.org_z = p.z;
    nextRay.ray.tnear = camera.getNear();
    nextRay.ray.tfar = camera.getFar();
    nextRay.ray.time = 0.0f;
    nextRay.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    auto incomingRadiance =
        radiance(scene, camera, randomState, context, nextRay, depth + 1);

    return glm::vec3(baseColor) * incomingRadiance;
}

glm::vec3 RayTracer::computeSpecular(RTCScene scene,
                                     const RayTracerCamera &camera,
                                     xorshift128plus_state &randomState,
                                     IntersectContext context, int32_t depth,
                                     const glm::vec4 &baseColor,
                                     float roughness, const glm::vec3 &p,
                                     const glm::vec3 &N, const glm::vec3 &V) {
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
    nextRay.ray.tnear = camera.getNear();
    nextRay.ray.tfar = camera.getFar();
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
        return incomingRadiance * (F * G * VoH / (NoH * NoV + 1e-6));
    }

    return glm::vec3(0.0f);
}

glm::vec3 RayTracer::computeReflection(
    RTCScene scene, const RayTracerCamera &camera,
    xorshift128plus_state &randomState, IntersectContext context, int32_t depth,
    const glm::vec4 &baseColor, float roughness, float metalness,
    const glm::vec3 &p, const glm::vec3 &N, const glm::vec3 &V) {
    auto specular = glm::vec3(0.0f);
    auto diffuse = glm::vec3(0.0f);

    if (depth > 2) {
        if (xorshift128plus01f(randomState) < metalness) {
            specular = computeSpecular(scene, camera, randomState, context,
                                       depth, baseColor, roughness, p, N, V);
        } else {
            diffuse = computeDiffuse(scene, camera, randomState, context, depth,
                                     baseColor, p, N);
        }
    } else {
        if (metalness != 1.0f) {
            diffuse = (1.0f - metalness) *
                      computeDiffuse(scene, camera, randomState, context, depth,
                                     baseColor, p, N);
        }

        if (metalness != 0.0f) {
            specular = metalness * computeSpecular(scene, camera, randomState,
                                                   context, depth, baseColor,
                                                   roughness, p, N, V);
        }
    }

    return specular + diffuse;
}

glm::vec3 RayTracer::computeRefraction(
    RTCScene scene, const RayTracerCamera &camera,
    xorshift128plus_state &randomState, IntersectContext context, int32_t depth,
    const glm::vec4 &baseColor, float roughness, float metalness,
    const glm::vec3 &p, const glm::vec3 &normal,
    const glm::vec3 &orientingNormal, const glm::vec3 &rayDir) {
    const bool into = glm::dot(normal, orientingNormal) > 0.0f;

    const auto nc = 1.0f;
    const auto nt = 1.5f;
    const auto nnt = into ? nc / nt : nt / nc;
    const auto ddn = glm::dot(rayDir, orientingNormal);
    const auto cos2t = 1.0f - nnt * nnt * (1.0f - ddn * ddn);

    if (cos2t < 0.0) {
        auto reflection = computeReflection(
            scene, camera, randomState, context, depth, baseColor, roughness,
            metalness, p, orientingNormal, -rayDir);
        return reflection;
    }

    const auto refractionDir =
        glm::normalize(rayDir * nnt - normal * (into ? 1.0f : -1.0f) *
                                          (ddn * nnt + glm::sqrt(cos2t)));
    auto refractionRay = RTCRayHit();
    refractionRay.ray.dir_x = refractionDir.x;
    refractionRay.ray.dir_y = refractionDir.y;
    refractionRay.ray.dir_z = refractionDir.z;
    refractionRay.ray.org_x = p.x;
    refractionRay.ray.org_y = p.y;
    refractionRay.ray.org_z = p.z;
    refractionRay.ray.tnear = camera.getNear();
    refractionRay.ray.tfar = camera.getFar();
    refractionRay.ray.time = 0.0f;
    refractionRay.hit.geomID = RTC_INVALID_GEOMETRY_ID;

    const auto a = nt - nc, b = nt + nc;
    const auto R0 = (a * a) / (b * b);

    const auto c =
        1.0f - (into ? -ddn : glm::dot(refractionDir, -1.0f * orientingNormal));
    const auto Re = R0 + (1.0f - R0) * glm::pow(c, 5.0f);
    const auto nnt2 = glm::pow(into ? nc / nt : nt / nc, 2.0f);
    const auto Tr = (1.0f - Re) * nnt2;

    const auto probability = 0.25f + 0.5f * Re;
    if (depth > 2) {
        if (xorshift128plus01f(randomState) < probability) {
            auto reflection =
                computeReflection(scene, camera, randomState, context, depth,
                                  baseColor, roughness, metalness, p,
                                  orientingNormal, -rayDir) *
                Re;
            return reflection / probability;
        } else {
            auto refraction = glm::vec3(baseColor) *
                              radiance(scene, camera, randomState, context,
                                       refractionRay, depth + 1) *
                              Tr;
            return refraction / (1.0f - probability);
        }
    } else {
        auto reflection =
            computeReflection(scene, camera, randomState, context, depth,
                              baseColor, roughness, metalness, p,
                              orientingNormal, -rayDir) *
            Re;
        auto refraction = glm::vec3(baseColor) *
                          radiance(scene, camera, randomState, context,
                                   refractionRay, depth + 1) *
                          Tr;
        return reflection + refraction;
    }
}

glm::vec3 RayTracer::radiance(RTCScene scene, const RayTracerCamera &camera,
                              xorshift128plus_state &randomState,
                              IntersectContext context, RTCRayHit &ray,
                              int32_t depth) {
    const auto kDepthLimit = 64;
    const auto kDepth = 5;
    const auto kEPS = 0.001f;

    /* intersect ray with scene */
    context.depth = depth;
    rtcIntersect1(scene, &context, &ray);

    auto Ng = glm::vec3(ray.hit.Ng_x, ray.hit.Ng_y, ray.hit.Ng_z);
    const auto rayDir = glm::vec3(ray.ray.dir_x, ray.ray.dir_y, ray.ray.dir_z);
    const auto p = glm::vec3(ray.ray.org_x, ray.ray.org_y, ray.ray.org_z) +
                   ray.ray.tfar * rayDir;

    if (ray.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
        const auto skybox = this->skybox.get();
        auto uv = toRadialCoords(rayDir);
        return bilinear(uv, skybox, skyboxWidth, skyboxHeight, false);
    }

    auto geom = rtcGetGeometry(scene, ray.hit.geomID);
    auto material = (const Material *)rtcGetGeometryUserData(geom);

    auto baseColor = context.baseColor;
    auto emissive = context.emissive;
    auto normal = context.normal;
    auto roughness = context.roughness;
    auto metalness = context.metalness;
    auto tangent = context.tangent;
    auto bitangent = context.bitangent;

    auto russianRouletteProbability = glm::max(
        material->baseColorFactor.x,
        glm::max(material->baseColorFactor.y, material->baseColorFactor.z));

    if (depth > kDepthLimit) {
        russianRouletteProbability *= pow(0.5f, depth - kDepthLimit);
    }

    if (depth > kDepth) {
        if (xorshift128plus01f(randomState) >= russianRouletteProbability) {
            return emissive;
        }
    } else {
        russianRouletteProbability = 1.0f;
    }

    const glm::vec3 orientingNormal =
        glm::dot(normal, rayDir) < 0.0f ? normal : (-1.0f * normal);

    switch (material->materialType) {
        case REFRACTION:
            auto refraction = computeRefraction(
                scene, camera, randomState, context, depth, baseColor,
                roughness, metalness, p, normal, orientingNormal, rayDir);
            return emissive + refraction / russianRouletteProbability;
        default:
        case REFLECTION:
            auto reflection = computeReflection(
                scene, camera, randomState, context, depth, baseColor,
                roughness, metalness, p, orientingNormal, -rayDir);
            return emissive + reflection / russianRouletteProbability;
    }
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

            const auto uv = glm::vec2(x + r1 / width, y + r2 / height);

            const auto rayDir =
                camera.getIsEquirectangula()
                    ? camera.getRayDirEquirectangular(uv.x, uv.y, width, height)
                    : camera.getRayDir(uv.x, uv.y);

            IntersectContext context;
            initIntersectContext(&context, scene, nullptr);

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
                const auto skybox = this->skybox.get();
                auto uv = toRadialCoords(rayDir);
                totalColor +=
                    bilinear(uv, skybox, skyboxWidth, skyboxHeight, false);
                continue;
            }

            totalColor += glm::vec3(context.baseColor);
        }
    }

    return totalColor / (samples * samples);
}

glm::vec3 RayTracer::renderNormal(RTCScene scene, const RayTracerCamera &camera,
                                  float x, float y) {
    const auto tnear = camera.getNear();
    const auto tfar = camera.getFar();
    const auto cameraFrom = camera.getCameraOrigin();

    const auto width = image.getWidth();
    const auto height = image.getHeight();

    const auto uv = glm::vec2(x, y);
    const auto rayDir =
        camera.getIsEquirectangula()
            ? camera.getRayDirEquirectangular(uv.x, uv.y, width, height)
            : camera.getRayDir(uv.x, uv.y);

    IntersectContext context;
    initIntersectContext(&context, scene, nullptr);

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

    return context.normal;
}

glm::vec3 RayTracer::renderTangent(RTCScene scene,
                                   const RayTracerCamera &camera, float x,
                                   float y) {
    const auto tnear = camera.getNear();
    const auto tfar = camera.getFar();
    const auto cameraFrom = camera.getCameraOrigin();

    const auto width = image.getWidth();
    const auto height = image.getHeight();

    const auto uv = glm::vec2(x, y);
    const auto rayDir =
        camera.getIsEquirectangula()
            ? camera.getRayDirEquirectangular(uv.x, uv.y, width, height)
            : camera.getRayDir(uv.x, uv.y);

    IntersectContext context;
    initIntersectContext(&context, scene, nullptr);

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

    return context.tangent;
}

glm::vec3 RayTracer::renderBitangent(RTCScene scene,
                                     const RayTracerCamera &camera, float x,
                                     float y) {
    const auto tnear = camera.getNear();
    const auto tfar = camera.getFar();
    const auto cameraFrom = camera.getCameraOrigin();

    const auto width = image.getWidth();
    const auto height = image.getHeight();

    const auto uv = glm::vec2(x, y);
    const auto rayDir =
        camera.getIsEquirectangula()
            ? camera.getRayDirEquirectangular(uv.x, uv.y, width, height)
            : camera.getRayDir(uv.x, uv.y);

    IntersectContext context;
    initIntersectContext(&context, scene, nullptr);

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

    return context.bitangent;
}

glm::vec3 RayTracer::renderEmissive(RTCScene scene,
                                    const RayTracerCamera &camera, float x,
                                    float y) {
    const auto tnear = camera.getNear();
    const auto tfar = camera.getFar();
    const auto cameraFrom = camera.getCameraOrigin();

    const auto width = image.getWidth();
    const auto height = image.getHeight();

    const auto uv = glm::vec2(x, y);
    const auto rayDir =
        camera.getIsEquirectangula()
            ? camera.getRayDirEquirectangular(uv.x, uv.y, width, height)
            : camera.getRayDir(uv.x, uv.y);

    IntersectContext context;
    initIntersectContext(&context, scene, nullptr);

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

    return context.emissive;
}

glm::vec3 RayTracer::renderPathTrace(RTCScene scene,
                                     const RayTracerCamera &camera,
                                     xorshift128plus_state &randomState,
                                     float x, float y) {
    auto totalRadiance = glm::vec3(0.0f);
    const auto width = image.getWidth();
    const auto height = image.getHeight();

    const auto samples = enableSuperSamples ? 2 : 1;
    for (int sy = 0; sy < samples; sy++) {
        for (int sx = 0; sx < samples; sx++) {
            const auto rate = 1.0f / samples;
            const auto r1 = (sx * rate + rate / 2.0f);
            const auto r2 = (sy * rate + rate / 2.0f);
            const auto uv = glm::vec2(x + r1 / width, y + r2 / height);

            glm::vec3 rayFrom, rayDir;

            if (camera.getIsEquirectangula()) {
                camera.getRayInfoEquirectangular(rayDir, rayFrom, uv.x, uv.y,
                                                 randomState, width, height);
            } else {
                camera.getRayInfo(rayDir, rayFrom, uv.x, uv.y, randomState);
            }

            IntersectContext context;
            initIntersectContext(&context, scene, &randomState);

            auto ray = RTCRayHit();
            ray.ray.dir_x = rayDir.x;
            ray.ray.dir_y = rayDir.y;
            ray.ray.dir_z = rayDir.z;
            ray.ray.org_x = rayFrom.x;
            ray.ray.org_y = rayFrom.y;
            ray.ray.org_z = rayFrom.z;
            ray.ray.tnear = camera.getNear();
            ray.ray.tfar = camera.getFar();
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
        case TANGENT: {
            return renderTangent(scene, camera, x, y);
        } break;
        case BITANGENT: {
            return renderBitangent(scene, camera, x, y);
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

bool RayTracer::render(RTCScene scene, const RayTracerCamera &camera) {
    if (getSamples() >= getMaxSamples()) {
        return false;
    }

    const auto width = this->image.getWidth();
    const auto height = this->image.getHeight();
    const auto numTilesX = (width + TILE_SIZE_X - 1) / TILE_SIZE_X;
    const auto numTilesY = (height + TILE_SIZE_Y - 1) / TILE_SIZE_Y;
    const auto tileSize = numTilesX * numTilesY;

    std::random_device random;
    xorshift128plus_state randomState;
    auto seed = (static_cast<uint64_t>(random()) << 32) + random();
    randomState.a = murmurHash3(seed);
    randomState.b = murmurHash3(~randomState.a);
    std::vector<xorshift128plus_state> randomStates(tileSize);

    for (auto it = randomStates.begin(); it != randomStates.end(); it++) {
        it->a = murmurHash3(xorshift128plus(randomState));
        it->b = murmurHash3(~it->a);
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

    return this->getSamples() >= this->getMaxSamples();
}

void RayTracer::denoise(oidn::DeviceRef denoiser) {
    const auto width = this->image.getWidth();
    const auto height = this->image.getHeight();
    const auto length = image.getWidth() * image.getHeight();
    const auto bufferSize = static_cast<uint64_t>(length) * sizeof(glm::vec3);
    auto filter = denoiser.newFilter("RT");

    auto temp = std::make_unique<glm::vec3[]>(bufferSize);
    memcpy(temp.get(), image.getBuffer().get(), bufferSize);

    filter.setImage("color", temp.get(), oidn::Format::Float3, width, height);
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
}

void RayTracer::finish(bool filtered) {
    this->image.updateTextureBuffer(filtered);
}
