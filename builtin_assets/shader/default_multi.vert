#version 450

layout(push_constant) uniform PushConstants{
    mat4 model;
    uvec4 textureIndices;
};

layout(set = 0, binding = 0) uniform VPBlockObject {
    mat4 view;
    mat4 projection;
    vec4 viewPos;
} VPBlock;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 vViewPos;
layout(location = 1) out vec2 vTexCoord;
layout(location = 2) out mat3 vTBN;

void main() {
    vec4 worldPos = model * vec4(inPosition, 1.0);
    vec4 viewPos = VPBlock.view * worldPos;
    vViewPos = viewPos.xyz;
    vTexCoord = inTexCoord;

    mat3 mvMat = mat3(VPBlock.view * model);
    vec3 T = normalize(mvMat * inTangent.xyz);
    vec3 N = normalize(mvMat * inNormal);
    vec3 B = normalize(cross(N, T) * inTangent.w);
    vTBN = mat3(T, B, N);

    gl_Position = VPBlock.projection * viewPos;
}