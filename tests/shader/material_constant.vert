#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform CameraBlockObject {
    float near;
    float far;
    float fov;
    float aspect;
    mat4 view;
    mat4 projection;
    mat4 invView;
    mat4 invProjection;
    vec4 viewPos;
} CameraInfo;

layout(push_constant) uniform PushConstants {
    layout(offset = 0) mat4 model;
    layout(offset = 64) vec4 baseColor;
    layout(offset = 80) float metallic;
    layout(offset = 84) float roughness;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec3 vViewPos;

void main() {
    mat4 mvMat = CameraInfo.view * model;
    vec4 viewPos = mvMat * vec4(inPosition, 1.0);
    vViewPos = viewPos.xyz;
    vNormal = normalize(mat3(mvMat) * inNormal);
    gl_Position = CameraInfo.projection * viewPos;
}
