#version 450
#extension GL_GOOGLE_include_directive : enable
#include "camera.glsl"
#include "light.glsl"
#include "shadow.glsl"
#include "forward_pbr.glsl"

layout(push_constant) uniform PushConstants {
    layout(offset = 0) mat4 model;
    layout(offset = 64) vec4 baseColorFactor;
    layout(offset = 80) vec4 materialFactors;
    layout(offset = 96) vec4 emissiveAlpha;
    layout(offset = 112) uvec4 forwardInfo;
} constants;

layout(location = 0) in vec3 vViewPos;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in mat3 vTBN;
layout(location = 0) out vec4 outColor;

void main()
{
    float metallic = clamp(constants.materialFactors.x, 0.0, 1.0);
    float roughness = clamp(constants.materialFactors.y, 0.04, 1.0);
    vec3 color = EvaluateForwardPBR(
        constants.baseColorFactor.rgb, metallic, roughness, 1.0,
        normalize(vTBN[2]), vViewPos, constants.forwardInfo.x,
        constants.forwardInfo.yz);
    color += constants.emissiveAlpha.rgb;
    if (constants.forwardInfo.w == 1u)
        color *= constants.baseColorFactor.a;
    outColor = vec4(color, constants.baseColorFactor.a);
}
