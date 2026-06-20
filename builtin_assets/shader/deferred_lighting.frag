#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "camera.glsl"
#include "light.glsl"
#include "shadow.glsl"

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Constants {
    uint directionalLightCnt;
    uint useSSAO;
} constants;

// GBuffer0: BaseColor + Occlusion
layout(set = 1, binding = 0) uniform sampler2D gBaseColor;   // BaseColor + Occlusion
layout(set = 1, binding = 1) uniform sampler2D gNormal;  // Normal
layout(set = 1, binding = 2) uniform sampler2D gMetalRough;   // Metallic + Roughness
layout(set = 1, binding = 3) uniform sampler2D gLinearDepth;
layout(set = 1, binding = 4) uniform samplerCube skyIrradianceLUT;
layout(set = 1, binding = 5) uniform samplerCube skySpecularLUT;
layout(set = 1, binding = 6) uniform sampler2D brdfLUT;
layout(set = 1, binding = 7) uniform sampler2D ssaoMap;

const float PI = 3.14159265359;
const float SKY_SPECULAR_ROUGHNESS_LEVELS = 6.0;
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
vec3 fresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 ReconstructViewPos(float depth)
{
    vec2 ndc = vTexCoord * 2.0 - 1.0;
    vec4 ray = CameraInfo.invProjection * vec4(ndc, 1.0, 1.0);
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

uint SelectDirectionalShadowCascade(float viewDepth, vec4 cascadeSplits, uint cascadeCnt)
{
    uint cascadeIndex = 0u;
    for (uint cascade = 1u; cascade < cascadeCnt; ++cascade)
    {
        if (viewDepth > cascadeSplits[cascade - 1u])
            cascadeIndex = cascade;
    }
    return cascadeIndex;
}

float SampleDirectionalShadow(uint lightIndex, vec3 worldPos, vec3 worldNormal, vec3 lightDir, float viewDepth)
{
    DirectionalShadowInfo shadowInfo = DirectionalShadows.shadows[0];
    if (shadowInfo.lightIndex.x == 0xFFFFFFFFu || shadowInfo.lightIndex.x != lightIndex)
        return 1.0;

    uint cascadeIndex = SelectDirectionalShadowCascade(viewDepth, shadowInfo.cascadeSplits, shadowInfo.cascadeCnt.x);
    float NdotL = max(dot(worldNormal, lightDir), 0.0);
    const float NORMAL_BIAS_TEXELS = 4.5;
    const float DEPTH_BIAS_MIN = 0.0005;
    const float DEPTH_BIAS_SLOPE = 0.0005;
    float normalOffset = shadowInfo.cascadeTexelSizes[int(cascadeIndex)] * NORMAL_BIAS_TEXELS;
    vec3 shadowWorldPos = worldPos + worldNormal * normalOffset;
    vec4 lightClip = shadowInfo.lightViewProj[cascadeIndex] * vec4(shadowWorldPos, 1.0);
    vec3 shadowCoord = lightClip.xyz / lightClip.w;
    vec2 uv = shadowCoord.xy * 0.5 + 0.5;
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0 || shadowCoord.z < 0.0 || shadowCoord.z > 1.0)
        return 1.0;

    float bias = max(DEPTH_BIAS_SLOPE * (1.0 - NdotL), DEPTH_BIAS_MIN);
    const int pcfRadius = 2;//cascadeIndex == 0u ? 3 : 1;
    const float PCF_TEXEL_RADIUS = 1.2;
    vec2 texelSize = 1.0 / vec2(textureSize(DirectionalShadowMaps[0], 0).xy);
    float visibility = 0.0;
    float weightSum = 0.0;
    for (int x = -pcfRadius; x <= pcfRadius; ++x)
    {
        for (int y = -pcfRadius; y <= pcfRadius; ++y)
        {
            vec2 sampleOffset = vec2(x, y) * texelSize * PCF_TEXEL_RADIUS;
            float weight = (float(pcfRadius + 1) - abs(float(x))) * (float(pcfRadius + 1) - abs(float(y)));
            visibility += texture(DirectionalShadowMaps[0],
                                  vec4(uv + sampleOffset,
                                       float(cascadeIndex), shadowCoord.z - bias)) * weight;
            weightSum += weight;
        }
    }
    return visibility / weightSum;
}

float SampleSpotShadow(SpotLight light, vec3 worldPos, vec3 worldNormal, vec3 lightDir)
{
    if (light.cone.z <= 0.0)
        return 1.0;

    uint slot = uint(light.cone.z) - 1u;
    if (slot >= MAX_SPOT_LIGHT_SHADOW_CNT)
        return 1.0;

    float NdotL = max(dot(worldNormal, lightDir), 0.0);
    const float NORMAL_BIAS = 0.003;
    const float DEPTH_BIAS_MIN = 0.00015;
    const float DEPTH_BIAS_SLOPE = 0.00045;
    vec3 shadowWorldPos = worldPos + worldNormal * NORMAL_BIAS;
    vec4 lightClip = SpotShadows.shadows[slot].lightViewProj * vec4(shadowWorldPos, 1.0);
    vec3 shadowCoord = lightClip.xyz / lightClip.w;
    vec2 uv = shadowCoord.xy * 0.5 + 0.5;
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0 || shadowCoord.z < 0.0 || shadowCoord.z > 1.0)
        return 1.0;

    float bias = max(DEPTH_BIAS_SLOPE * (1.0 - NdotL), DEPTH_BIAS_MIN);
    const int pcfRadius = 1;
    vec2 texelSize = 1.0 / vec2(textureSize(SpotShadowMap, 0));
    float visibility = 0.0;
    float weightSum = 0.0;
    for (int x = -pcfRadius; x <= pcfRadius; ++x)
    {
        for (int y = -pcfRadius; y <= pcfRadius; ++y)
        {
            vec2 sampleOffset = vec2(x, y) * texelSize;
            float weight = (float(pcfRadius + 1) - abs(float(x))) * (float(pcfRadius + 1) - abs(float(y)));
            visibility += texture(SpotShadowMap, vec4(uv + sampleOffset, float(slot), shadowCoord.z - bias)) * weight;
            weightSum += weight;
        }
    }
    return visibility / weightSum;
}

void main()
{
    vec4 baseColorTex = texture(gBaseColor, vTexCoord);
    vec4 normalTex    = texture(gNormal, vTexCoord);
    vec4 metalRough   = texture(gMetalRough, vTexCoord);
    float depth = texture(gLinearDepth, vTexCoord).r;

    vec3 albedo = baseColorTex.rgb;
    float occlusion = baseColorTex.a;
    float ssao = constants.useSSAO != 0u ? texture(ssaoMap, vTexCoord).r : 1.0;

    float metallic = clamp(metalRough.r, 0.0, 1.0);
    float roughness = clamp(metalRough.g, 0.04, 1.0);

    vec3 N = normalize(normalTex.rgb * 2.0 - 1.0);
    vec3 viewPos = ReconstructViewPos(depth);
    vec2 ndc = vTexCoord * 2.0 - 1.0;
    vec3 V = normalize(-viewPos);
    mat4 invView = CameraInfo.invView;
    mat3 viewToWorld = mat3(invView);
    vec3 worldPos = (invView * vec4(viewPos, 1.0)).xyz;
    vec3 worldN = normalize(viewToWorld * N);
    vec3 worldV = normalize(viewToWorld * V);
    vec3 worldR = normalize(reflect(-worldV, worldN));
    float NdotV = max(dot(N, V), 0.0);
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
        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3  F = fresnelSchlick(F0, max(dot(H, V), 0.0));
        vec3 spec = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-6);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
        vec3 diffuse = albedo / PI;
        vec3 radiance = light.colorWithIntensity.rgb * light.colorWithIntensity.w;
        vec3 worldLightDir = normalize(-light.direction.xyz);
        float shadow = SampleDirectionalShadow(i, worldPos, worldN, worldLightDir, -viewPos.z);
        Lo += (kD * diffuse + spec) * radiance * NdotL * shadow;
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
        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3  F = fresnelSchlick(F0, max(dot(H, V), 0.0));
        vec3 spec = (D * G * F) / max(4.0 * NdotV * NdotL, 1e-6);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
        vec3 diffuse = albedo / PI;
        vec3 radiance = light.colorWithIntensity.rgb * light.colorWithIntensity.w;
        vec3 worldLightDir = normalize(light.positionWithRadius.xyz - worldPos);
        float shadow = SampleSpotShadow(light, worldPos, worldN, worldLightDir);
        Lo += (kD * diffuse + spec) * radiance * NdotL * attenuation * shadow;
    }

    vec3 F = fresnelSchlickRoughness(F0, NdotV, roughness);
    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    vec3 irradiance = texture(skyIrradianceLUT, normalize(worldN)).rgb;
    vec3 diffuseIBL = irradiance * albedo;

    vec2 brdf = texture(brdfLUT, vec2(NdotV, roughness)).rg;
    float lod = clamp(roughness, 0.0, 1.0) * (SKY_SPECULAR_ROUGHNESS_LEVELS - 1.0);
    vec3 prefilteredColor = textureLod(skySpecularLUT, normalize(worldR), lod).rgb;
    vec3 specularIBL = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuseIBL + specularIBL) * occlusion * ssao;

    vec3 color = Lo + ambient;
    outColor = vec4(color, 1.0);
    //outColor += vec4(float(pointLightCnt) * 0.1, 0.0, 0.0, 1.0); //TODO DEBUG
}
