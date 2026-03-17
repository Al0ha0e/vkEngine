#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "camera.glsl"

layout(push_constant) uniform PushConstants{
    mat4 model;
    uvec4 textureIndices;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 vViewPos;
layout(location = 1) out vec2 vTexCoord;
layout(location = 2) out mat3 vTBN;

void main() {
    vec4 worldPos = model * vec4(inPosition, 1.0);
    vec4 viewPos = CameraInfo.view * worldPos;
    vViewPos = viewPos.xyz;
    vTexCoord = inTexCoord;

    mat3 mvMat = mat3(CameraInfo.view * model);
    vec3 T = normalize(mvMat * inTangent.xyz);
    vec3 N = normalize(mvMat * inNormal);
    vec3 B = normalize(cross(N, T) * inTangent.w);
    vTBN = mat3(T, B, N);

    gl_Position = CameraInfo.projection * viewPos;
}