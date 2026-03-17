#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "camera.glsl"
#include "light.glsl"

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Constants {
    uint directionalLightCnt;
} constants;

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

vec3 ReconstructViewPos(float depth)
{
    vec2 ndc = vTexCoord * 2.0 - 1.0;
    vec4 ray = inverse(CameraInfo.projection) * vec4(ndc, 1.0, 1.0);
    vec3 viewRay = ray.xyz / ray.w;
    float t = depth / -viewRay.z;
    return viewRay * t;
}

uint GetClusterIndex(vec3 viewPos)
{
    uint clusterX = uint(vTexCoord.x * CLUSTER_DIM_X);
    uint clusterY = uint(vTexCoord.y * CLUSTER_DIM_Y);
    float z = viewPos.z;     // view-space depth (negative)
    float slice = log(-z / CameraInfo.near) / log(CameraInfo.far / CameraInfo.near);
    uint clusterZ = uint(slice * float(CLUSTER_DIM_Z));
    clusterZ = clamp(clusterZ, 0u, CLUSTER_DIM_Z - 1u);
    return clusterX
         + clusterY * CLUSTER_DIM_X
         + clusterZ * CLUSTER_DIM_X * CLUSTER_DIM_Y;
}

vec3 ACESFilm(vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e),0.0,1.0);
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

    vec3 N = normalize(normalTex.rgb * 2.0 - 1.0);
    vec3 viewPos = ReconstructViewPos(depth);
    vec2 ndc = vTexCoord * 2.0 - 1.0;
    vec3 V = -normalize(vec3(ndc.x, -ndc.y, -1.0));
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    vec3 Lo = vec3(0.0);
    // ---------------------------
    // Directional Lights
    // ---------------------------
    for(uint i = 0; i < constants.directionalLightCnt; i++)
    {
        DirectionalLight light = DirectionalLights.lights[i];
        vec3 vViewSpaceLightDir = (CameraInfo.view * vec4(light.direction.xyz, 0.0)).xyz;
        vec3 L = normalize(-vViewSpaceLightDir);
        vec3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3  F = fresnelSchlick(F0, max(dot(H, V), 0.0));
        vec3 spec = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-6);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
        vec3 diffuse = albedo / PI;
        vec3 radiance = light.colorWithIntensity.rgb * light.colorWithIntensity.w;
        Lo += (kD * diffuse + spec) * radiance * NdotL;
    }

    // ---------------------------
    // Cluster Index
    // ---------------------------
    uint clusterIndex = GetClusterIndex(viewPos);
    uint offset = clusterIndex * MAX_LIGHT_PER_CLUSTER;

    // ---------------------------
    // Point Lights
    // ---------------------------
    uint pointLightCnt = 0;
    for(uint i = 0; i < MAX_LIGHT_PER_CLUSTER; i++)
    {
        uint lightIndex = ClusterPointLights.indices[offset + i];
        if(lightIndex == 0xFFFFFFFFu)
            break;
        pointLightCnt++;
        PointLight light = PointLights.lights[lightIndex];
        vec3 lightPosVS = (CameraInfo.view * vec4(light.positionWithRadius.xyz,1.0)).xyz;
        vec3 L = lightPosVS - viewPos;
        float dist = length(L);
        L = normalize(L);
        float radius = light.positionWithRadius.w;
        float attenuation = clamp(1.0 - dist / radius, 0.0, 1.0);
        attenuation *= attenuation;
        vec3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3  F = fresnelSchlick(F0, max(dot(H, V), 0.0));
        vec3 spec = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-6);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
        vec3 diffuse = albedo / PI;
        vec3 radiance = light.colorWithIntensity.rgb * light.colorWithIntensity.w;
        Lo += (kD * diffuse + spec) * radiance * NdotL * attenuation;
    }

    // ---------------------------
    // Spot Lights
    // ---------------------------
    for(uint i = 0; i < MAX_LIGHT_PER_CLUSTER; i++)
    {
        uint lightIndex = ClusterSpotLights.indices[offset + i];
        if(lightIndex == 0xFFFFFFFFu)
            break;
        SpotLight light = SpotLights.lights[lightIndex];
        vec3 lightPosVS = (CameraInfo.view * vec4(light.positionWithRadius.xyz,1.0)).xyz;
        vec3 lightDirVS = normalize((CameraInfo.view * vec4(light.direction.xyz,0.0)).xyz);
        vec3 L = lightPosVS - viewPos;
        float dist = length(L);
        L = normalize(L);
        float radius = light.positionWithRadius.w;
        float attenuation = clamp(1.0 - dist / radius, 0.0, 1.0);
        attenuation *= attenuation;
        float theta = dot(L, -lightDirVS);
        float inner = light.cone.x;
        float outer = light.cone.y;
        float cone = clamp((theta - outer) / (inner - outer), 0.0, 1.0);
        attenuation *= cone;
        vec3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3  F = fresnelSchlick(F0, max(dot(H, V), 0.0));
        vec3 spec = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-6);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
        vec3 diffuse = albedo / PI;
        vec3 radiance = light.colorWithIntensity.rgb * light.colorWithIntensity.w;
        Lo += (kD * diffuse + spec) * radiance * NdotL * attenuation;
    }


    // small ambient fallback (no IBL)
    vec3 ambient = albedo * 0.1; //* occlusion; // low ambient

    vec3 color = Lo + ambient;
    vec3 mapped = ACESFilm(color);
    outColor = vec4(mapped, 1.0);
    //outColor += vec4(float(pointLightCnt) * 0.1, 0.0, 0.0, 1.0); //TODO DEBUG
}