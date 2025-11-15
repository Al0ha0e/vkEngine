#version 450

#extension GL_GOOGLE_include_directive : enable

#include "light.glsl"

layout(location = 0) in vec2 vTexCoord;
layout(location = 1) in vec3 vViewSpaceLightDir;

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
layout(set = 1, binding = 3) uniform sampler2D gLinearDepth;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N,H),0.0);
    float NdotH2 = NdotH*NdotH;
    float denom = (NdotH2*(a2-1.0)+1.0);
    denom = PI * denom * denom;
    return a2 / max(denom, 1e-6);
}
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness+1.0);
    float k = (r*r)/8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N,V),0.0);
    float NdotL = max(dot(N,L),0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}
vec3 fresnelSchlick(vec3 F0, float cosTheta) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}


void main()
{
    vec4 baseColorTex = texture(gBaseColor, vTexCoord);
    vec4 normalTex    = texture(gNormal, vTexCoord);
    vec4 metalRough   = texture(gMetalRough, vTexCoord);
    float depth = texture(gLinearDepth, vTexCoord).r;

    vec3 albedo = baseColorTex.rgb;

    float metallic = clamp(metalRough.r, 0.0, 1.0);
    float roughness = clamp(metalRough.g, 0.04, 1.0);

    vec2 ndc = vTexCoord * 2.0 - 1.0;

    vec3 L = normalize(-vViewSpaceLightDir);
    vec3 V = -normalize(vec3(ndc.x, -ndc.y, -1.0));
    vec3 N = normalize(normalTex.rgb * 2.0 - 1.0);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    float D = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(F0, max(dot(H,V),0.0));
    vec3 spec = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-6);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 diffuse = albedo / PI;

    DirectionalLight light = DirectionalLights.lights[0];
    vec3 radiance = light.color.rgb * light.color.w;

    vec3 Lo = (kD * diffuse + spec) * radiance * NdotL;

    // small ambient fallback (no IBL)
    vec3 ambient = albedo * 0.1; //* occlusion; // low ambient

    vec3 color = Lo + ambient;
    vec3 mapped = color / (color + vec3(1.0));

    outColor = vec4(mapped, 1.0);
}