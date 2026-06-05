#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "camera.glsl"

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out float outOcclusion;

layout(set = 1, binding = 0) uniform sampler2D gNormal;
layout(set = 1, binding = 1) uniform sampler2D gLinearDepth;

layout(push_constant) uniform SSAOConstants {
    float radius;
    float bias;
    float intensity;
    float power;
} constants;

const int SAMPLE_COUNT = 24;
const vec3 SAMPLE_KERNEL[SAMPLE_COUNT] = vec3[](
    vec3( 0.5381,  0.1856,  0.4319),
    vec3( 0.1379,  0.2486,  0.4430),
    vec3( 0.3371,  0.5679,  0.0057),
    vec3(-0.6999, -0.0451,  0.0019),
    vec3( 0.0689, -0.1598,  0.8547),
    vec3( 0.0560,  0.0069,  0.1843),
    vec3(-0.0146,  0.1402,  0.0762),
    vec3( 0.0100, -0.1924,  0.0344),
    vec3(-0.3577, -0.5301,  0.4358),
    vec3(-0.3169,  0.1063,  0.0158),
    vec3( 0.0103, -0.5869,  0.0046),
    vec3(-0.0897, -0.4940,  0.3287),
    vec3( 0.7119, -0.0154,  0.0918),
    vec3(-0.0533,  0.0596,  0.5411),
    vec3( 0.0352, -0.0631,  0.5460),
    vec3(-0.4776,  0.2847,  0.0271),
    vec3( 0.6124,  0.4201,  0.1813),
    vec3(-0.2458,  0.7813,  0.2186),
    vec3( 0.1987, -0.7104,  0.3062),
    vec3(-0.6502, -0.3521,  0.1924),
    vec3( 0.3964,  0.0991,  0.7265),
    vec3(-0.3051,  0.1887,  0.8214),
    vec3( 0.1568, -0.3776,  0.9125),
    vec3(-0.0913, -0.6422,  0.6391)
);

float InterleavedGradientNoise(vec2 pixel)
{
    return fract(52.9829189 * fract(0.06711056 * pixel.x + 0.00583715 * pixel.y));
}

vec3 ReconstructViewPos(vec2 uv, float depth)
{
    vec2 ndc = uv * 2.0 - 1.0;
    vec4 ray = CameraInfo.invProjection * vec4(ndc, 1.0, 1.0);
    vec3 viewRay = ray.xyz / ray.w;
    float t = depth / -viewRay.z;
    return viewRay * t;
}

mat3 BuildTBN(vec3 normal, vec3 randomVec)
{
    vec3 tangent = randomVec - normal * dot(randomVec, normal);
    if (dot(tangent, tangent) < 1e-4)
    {
        vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(0.0, 1.0, 0.0);
        tangent = cross(up, normal);
    }
    tangent = normalize(tangent);
    vec3 bitangent = cross(normal, tangent);
    return mat3(tangent, bitangent, normal);
}

void main()
{
    float depth = texture(gLinearDepth, vTexCoord).r;
    if (depth <= 0.0 || depth >= CameraInfo.far)
    {
        outOcclusion = 1.0;
        return;
    }

    vec3 normal = normalize(texture(gNormal, vTexCoord).rgb * 2.0 - 1.0);
    vec3 viewPos = ReconstructViewPos(vTexCoord, depth);
    float angle = InterleavedGradientNoise(gl_FragCoord.xy) * 6.28318530718;
    vec3 randomVec = vec3(cos(angle), sin(angle), 0.0);
    mat3 tbn = BuildTBN(normal, randomVec);

    float occlusion = 0.0;
    for (int i = 0; i < SAMPLE_COUNT; ++i)
    {
        vec3 samplePos = viewPos + (tbn * SAMPLE_KERNEL[i]) * constants.radius;
        vec4 offset = CameraInfo.projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        vec2 sampleUV = offset.xy * 0.5 + 0.5;
        if (any(lessThan(sampleUV, vec2(0.0))) || any(greaterThan(sampleUV, vec2(1.0))))
            continue;

        float sampleDepth = texture(gLinearDepth, sampleUV).r;
        if (sampleDepth <= 0.0 || sampleDepth >= CameraInfo.far)
            continue;

        vec3 sampleViewPos = ReconstructViewPos(sampleUV, sampleDepth);
        float rangeCheck = smoothstep(0.0, 1.0, constants.radius / max(abs(viewPos.z - sampleViewPos.z), 0.0001));
        float depthDelta = sampleViewPos.z - samplePos.z;
        float hit = smoothstep(constants.bias, constants.bias + constants.radius * 0.2, depthDelta);
        occlusion += hit * rangeCheck;
    }

    float ao = 1.0 - (occlusion / float(SAMPLE_COUNT)) * constants.intensity;
    outOcclusion = pow(clamp(ao, 0.0, 1.0), constants.power);
}
