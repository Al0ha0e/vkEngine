#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(push_constant) uniform PushConstants{
    mat4 model;
    uvec4 textureIndices;
};

layout(location = 0) in vec3 vViewPos;
layout(location = 1) in vec2 vTexCoord;
layout(location = 2) in mat3 vTBN;

layout(location = 0) out vec4 outBaseColor;     // GBuffer0: BaseColor.rgb + Occlusion.a
layout(location = 1) out vec4 outNormal;   // GBuffer1: Normal.rgb
layout(location = 2) out vec4 outMetalRough;    // GBuffer2: Metallic.r + Roughness.g
layout(location = 3) out float outLinearDepth;

layout(set = 1, binding = 0) uniform sampler2D textures[];

void main() {
    // ===== Base Color & Occlusion =====
    vec4 baseColorTex = texture(textures[nonuniformEXT(textureIndices.x)], vTexCoord);
    outBaseColor = vec4(baseColorTex.rgb, 0.0);
 
    // ===== Metallic + Roughness =====
    vec4 mrTex = texture(textures[nonuniformEXT(textureIndices.y)], vTexCoord);
    outMetalRough = vec4(mrTex.r, mrTex.g, 0.0, 0.0);

    // ===== Normal =====
    vec3 nTex = texture(textures[nonuniformEXT(textureIndices.z)], vTexCoord).rgb * 2.0 - vec3(1.0);
    vec3 viewNormal = normalize(vTBN * nTex);
    outNormal = vec4(viewNormal * 0.5 + 0.5, 0.0);

    outLinearDepth = -vViewPos.z;
}