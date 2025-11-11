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

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec2 vTexCoord;
layout(location = 3) out mat3 vTBN;

void main() {
    vec4 worldPos = model * vec4(inPosition, 1.0);
    vWorldPos = worldPos.xyz;

    vec3 N = normalize(mat3(model) * inNormal);
    vec3 T = normalize(mat3(model) * inTangent.xyz);
    vec3 B = cross(N, T) * inTangent.w;
    vTBN = mat3(T, B, N);
    vNormal = N;

    vTexCoord = inTexCoord;

    gl_Position = VPBlock.projection * VPBlock.view * worldPos;
}