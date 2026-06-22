#version 450
#extension GL_GOOGLE_include_directive : enable
#include "camera.glsl"

layout(push_constant) uniform PushConstants {
    layout(offset = 0) mat4 model;
    layout(offset = 64) vec4 baseColorFactor;
    layout(offset = 80) vec4 materialFactors;
    layout(offset = 96) vec4 emissiveAlpha;
    layout(offset = 112) uvec4 forwardInfo;
} constants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 vViewPos;
layout(location = 1) out vec2 vTexCoord;
layout(location = 2) out mat3 vTBN;

void main()
{
    mat4 modelView = CameraInfo.view * constants.model;
    vec4 viewPosition = modelView * vec4(inPosition, 1.0);
    mat3 normalMatrix = transpose(inverse(mat3(modelView)));
    vec3 N = normalize(normalMatrix * inNormal);
    vec3 T = normalize(normalMatrix * inTangent.xyz);
    T = normalize(T - N * dot(N, T));
    vec3 B = normalize(cross(N, T) * inTangent.w);

    vViewPos = viewPosition.xyz;
    vTexCoord = inTexCoord;
    vTBN = mat3(T, B, N);
    gl_Position = CameraInfo.projection * viewPosition;
}
