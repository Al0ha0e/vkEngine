#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "camera.glsl"

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out float outOcclusion;

layout(set = 1, binding = 0) uniform sampler2D ssaoRaw;
layout(set = 1, binding = 1) uniform sampler2D gNormal;
layout(set = 1, binding = 2) uniform sampler2D gLinearDepth;

layout(push_constant) uniform SSAOConstants {
    float radius;
    float bias;
    float intensity;
    float power;
} constants;

float DepthWeight(float centerDepth, float sampleDepth)
{
    if (sampleDepth <= 0.0 || sampleDepth >= CameraInfo.far)
        return 0.0;

    float tolerance = max(constants.radius * 0.75, 0.001);
    float delta = abs(centerDepth - sampleDepth);
    return exp(-(delta * delta) / (2.0 * tolerance * tolerance));
}

float NormalWeight(vec3 centerNormal, vec3 sampleNormal)
{
    return pow(max(dot(centerNormal, sampleNormal), 0.0), 12.0);
}

void main()
{
    float centerDepth = texture(gLinearDepth, vTexCoord).r;
    if (centerDepth <= 0.0 || centerDepth >= CameraInfo.far)
    {
        outOcclusion = 1.0;
        return;
    }

    vec3 centerNormal = normalize(texture(gNormal, vTexCoord).rgb * 2.0 - 1.0);
    vec2 texelSize = 1.0 / vec2(textureSize(ssaoRaw, 0));

    float sum = 0.0;
    float weightSum = 0.0;
    for (int y = -2; y <= 2; ++y)
    {
        for (int x = -2; x <= 2; ++x)
        {
            vec2 sampleUV = clamp(vTexCoord + vec2(x, y) * texelSize, vec2(0.0), vec2(1.0));
            float sampleDepth = texture(gLinearDepth, sampleUV).r;
            vec3 sampleNormal = normalize(texture(gNormal, sampleUV).rgb * 2.0 - 1.0);
            float spatial = exp(-dot(vec2(x, y), vec2(x, y)) * 0.075);
            float weight = spatial * DepthWeight(centerDepth, sampleDepth) * NormalWeight(centerNormal, sampleNormal);
            sum += texture(ssaoRaw, sampleUV).r * weight;
            weightSum += weight;
        }
    }

    outOcclusion = weightSum > 0.0 ? sum / weightSum : texture(ssaoRaw, vTexCoord).r;
}
