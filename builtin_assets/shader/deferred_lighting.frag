#version 450

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform VPBlockObject {
    mat4 view;
    mat4 projection;
    vec4 viewPos;
} VPBlock;

// GBuffer0: BaseColor + Occlusion
layout(set = 1, binding = 0) uniform sampler2D gBaseColor;   // BaseColor + Occlusion
layout(set = 1, binding = 1) uniform sampler2D gNormal;  // Normal
layout(set = 1, binding = 2) uniform sampler2D gMetalRough;   // Metallic + Roughness

void main()
{
    vec4 baseColorTex = texture(gBaseColor, vTexCoord);
    outColor = vec4(baseColorTex.rgb, 1.0);
}