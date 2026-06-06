#version 450

layout(push_constant) uniform PushConstants {
    layout(offset = 0) mat4 model;
    layout(offset = 64) vec4 baseColor;
    layout(offset = 80) float metallic;
    layout(offset = 84) float roughness;
};

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec3 vViewPos;

layout(location = 0) out vec4 outBaseColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outMetalRough;
layout(location = 3) out float outLinearDepth;

void main() {
    outBaseColor = baseColor;
    outNormal = vec4(normalize(vNormal) * 0.5 + 0.5, 0.0);
    outMetalRough = vec4(metallic, roughness, 0.0, 0.0);
    outLinearDepth = -vViewPos.z;
}
