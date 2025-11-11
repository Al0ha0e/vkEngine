#version 450

layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vTexCoord;

layout(location = 0) out vec4 outBaseColor;     // GBuffer0
layout(location = 1) out vec4 outNormalFlags;   // GBuffer1
layout(location = 2) out vec4 outMetalRough;    // GBuffer2

layout(set = 1, binding = 0) uniform sampler2D uBaseColorTex;

void main() {
    // BaseColor (RGB) + Occlusion (A)
    outBaseColor = vec4(texture(uBaseColorTex, vTexCoord).rgb, 0);
    // Normal
    outNormalFlags = vec4(normalize(vNormal) * 0.5 + 0.5, 0);
    // Metallic + Roughness
    outMetalRough = vec4(0.0);
}