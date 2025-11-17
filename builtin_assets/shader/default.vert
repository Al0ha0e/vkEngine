#version 450

layout(push_constant) uniform PushConstants{
    mat4 model;
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

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec3 vViewPos;
layout(location = 2) out vec2 vTexCoord;

void main() {
    mat4 mvMat = VPBlock.view * model;
    vec4 viewPos = mvMat * vec4(inPosition, 1.0);
    vViewPos = viewPos.xyz;
    
    vNormal = normalize(mat3(mvMat) * inNormal);
    vTexCoord = inTexCoord;

    gl_Position = VPBlock.projection * viewPos;
}