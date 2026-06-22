#ifndef FORWARD_PBR_H
#define FORWARD_PBR_H

layout(set = 3, binding = 0) uniform samplerCube forwardSkyIrradianceLUT;
layout(set = 3, binding = 1) uniform samplerCube forwardSkySpecularLUT;
layout(set = 3, binding = 2) uniform sampler2D forwardBrdfLUT;

const float FORWARD_PI = 3.14159265359;
const float FORWARD_SKY_SPECULAR_LEVELS = 6.0;

float ForwardDistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a2 = pow(roughness, 4.0);
    float nh2 = pow(max(dot(N, H), 0.0), 2.0);
    float d = nh2 * (a2 - 1.0) + 1.0;
    return a2 / max(FORWARD_PI * d * d, 1e-6);
}

float ForwardGeometrySchlickGGX(float nv, float roughness)
{
    float k = pow(roughness + 1.0, 2.0) / 8.0;
    return nv / max(nv * (1.0 - k) + k, 1e-6);
}

float ForwardGeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    return ForwardGeometrySchlickGGX(max(dot(N, V), 0.0), roughness) *
           ForwardGeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

vec3 ForwardFresnel(vec3 f0, float cosTheta)
{
    return f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0);
}

vec3 ForwardFresnelRoughness(vec3 f0, float cosTheta, float roughness)
{
    return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(1.0 - cosTheta, 5.0);
}

uint ForwardClusterIndex(vec3 viewPos, vec2 fragCoord, uvec2 viewportSize)
{
    vec2 uv = fragCoord / max(vec2(viewportSize), vec2(1.0));
    uint x = min(uint(uv.x * CLUSTER_DIM_X), CLUSTER_DIM_X - 1u);
    uint y = min(uint(uv.y * CLUSTER_DIM_Y), CLUSTER_DIM_Y - 1u);
    float slice = log(max(-viewPos.z, CameraInfo.near) / CameraInfo.near) /
                  log(CameraInfo.far / CameraInfo.near);
    uint z = uint(clamp(slice, 0.0, 0.999999) * float(CLUSTER_DIM_Z));
    return x + y * CLUSTER_DIM_X + z * CLUSTER_DIM_X * CLUSTER_DIM_Y;
}

uint ForwardDirectionalCascade(float viewDepth, vec4 splits, uint cascadeCnt)
{
    uint cascade = 0u;
    for (uint i = 1u; i < cascadeCnt; ++i)
        if (viewDepth > splits[i - 1u])
            cascade = i;
    return cascade;
}

float ForwardDirectionalShadow(uint lightIndex, vec3 worldPos, vec3 worldNormal,
                               vec3 lightDir, float viewDepth)
{
    DirectionalShadowInfo info = DirectionalShadows.shadows[0];
    if (info.lightIndex.x == 0xFFFFFFFFu || info.lightIndex.x != lightIndex)
        return 1.0;
    uint cascade = ForwardDirectionalCascade(viewDepth, info.cascadeSplits, info.cascadeCnt.x);
    float ndotl = max(dot(worldNormal, lightDir), 0.0);
    vec3 biasedPosition = worldPos + worldNormal * info.cascadeTexelSizes[int(cascade)] * 4.5;
    vec4 clip = info.lightViewProj[cascade] * vec4(biasedPosition, 1.0);
    vec3 coord = clip.xyz / clip.w;
    vec2 uv = coord.xy * 0.5 + 0.5;
    if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0))) ||
        coord.z < 0.0 || coord.z > 1.0)
        return 1.0;
    float bias = max(0.0005 * (1.0 - ndotl), 0.0005);
    vec2 texel = 1.0 / vec2(textureSize(DirectionalShadowMaps[0], 0).xy);
    float visibility = 0.0;
    float weightSum = 0.0;
    for (int x = -2; x <= 2; ++x)
        for (int y = -2; y <= 2; ++y)
        {
            float weight = float(3 - abs(x)) * float(3 - abs(y));
            visibility += texture(DirectionalShadowMaps[0],
                                  vec4(uv + vec2(x, y) * texel * 1.2,
                                       float(cascade), coord.z - bias)) * weight;
            weightSum += weight;
        }
    return visibility / weightSum;
}

float ForwardSpotShadow(SpotLight light, vec3 worldPos, vec3 worldNormal, vec3 lightDir)
{
    if (light.cone.z <= 0.0)
        return 1.0;
    uint slot = uint(light.cone.z) - 1u;
    if (slot >= MAX_SPOT_LIGHT_SHADOW_CNT)
        return 1.0;
    float ndotl = max(dot(worldNormal, lightDir), 0.0);
    vec4 clip = SpotShadows.shadows[slot].lightViewProj *
                vec4(worldPos + worldNormal * 0.003, 1.0);
    vec3 coord = clip.xyz / clip.w;
    vec2 uv = coord.xy * 0.5 + 0.5;
    if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0))) ||
        coord.z < 0.0 || coord.z > 1.0)
        return 1.0;
    float bias = max(0.00045 * (1.0 - ndotl), 0.00015);
    vec2 texel = 1.0 / vec2(textureSize(SpotShadowMap, 0));
    float visibility = 0.0;
    float weightSum = 0.0;
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
        {
            float weight = float(2 - abs(x)) * float(2 - abs(y));
            visibility += texture(SpotShadowMap,
                                  vec4(uv + vec2(x, y) * texel, float(slot), coord.z - bias)) * weight;
            weightSum += weight;
        }
    return visibility / weightSum;
}

vec3 ForwardBRDF(vec3 albedo, float metallic, float roughness, vec3 N, vec3 V, vec3 L,
                 vec3 radiance)
{
    vec3 H = normalize(V + L);
    float ndotl = max(dot(N, L), 0.0);
    float ndotv = max(dot(N, V), 0.0);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = ForwardFresnel(f0, max(dot(H, V), 0.0));
    vec3 specular = ForwardDistributionGGX(N, H, roughness) *
                    ForwardGeometrySmith(N, V, L, roughness) * F /
                    max(4.0 * ndotv * ndotl, 1e-6);
    vec3 diffuse = (vec3(1.0) - F) * (1.0 - metallic) * albedo / FORWARD_PI;
    return (diffuse + specular) * radiance * ndotl;
}

vec3 EvaluateForwardPBR(vec3 albedo, float metallic, float roughness, float occlusion,
                        vec3 N, vec3 viewPos, uint directionalLightCnt, uvec2 viewportSize)
{
    vec3 V = normalize(-viewPos);
    mat3 viewToWorld = mat3(CameraInfo.invView);
    vec3 worldPos = (CameraInfo.invView * vec4(viewPos, 1.0)).xyz;
    vec3 worldN = normalize(viewToWorld * N);
    vec3 worldV = normalize(viewToWorld * V);
    vec3 color = vec3(0.0);

    for (uint i = 0u; i < directionalLightCnt; ++i)
    {
        DirectionalLight light = DirectionalLights.lights[i];
        vec3 L = normalize(-(CameraInfo.view * vec4(light.direction.xyz, 0.0)).xyz);
        vec3 radiance = light.colorWithIntensity.rgb * light.colorWithIntensity.w;
        float shadow = ForwardDirectionalShadow(i, worldPos, worldN,
                                                normalize(-light.direction.xyz), -viewPos.z);
        color += ForwardBRDF(albedo, metallic, roughness, N, V, L, radiance) * shadow;
    }

    uint offset = ForwardClusterIndex(viewPos, gl_FragCoord.xy, viewportSize) * MAX_LIGHT_PER_CLUSTER;
    for (uint i = 0u; i < MAX_LIGHT_PER_CLUSTER; ++i)
    {
        uint lightIndex = ClusterPointLights.indices[offset + i];
        if (lightIndex == 0xFFFFFFFFu)
            break;
        PointLight light = PointLights.lights[lightIndex];
        vec3 delta = (CameraInfo.view * vec4(light.positionWithRadius.xyz, 1.0)).xyz - viewPos;
        float distanceToLight = length(delta);
        vec3 L = normalize(delta);
        float attenuation = pow(clamp(1.0 - distanceToLight / light.positionWithRadius.w, 0.0, 1.0), 2.0);
        color += ForwardBRDF(albedo, metallic, roughness, N, V, L,
                            light.colorWithIntensity.rgb * light.colorWithIntensity.w) * attenuation;
    }

    for (uint i = 0u; i < MAX_LIGHT_PER_CLUSTER; ++i)
    {
        uint lightIndex = ClusterSpotLights.indices[offset + i];
        if (lightIndex == 0xFFFFFFFFu)
            break;
        SpotLight light = SpotLights.lights[lightIndex];
        vec3 delta = (CameraInfo.view * vec4(light.positionWithRadius.xyz, 1.0)).xyz - viewPos;
        float distanceToLight = length(delta);
        vec3 L = normalize(delta);
        vec3 lightDirVS = normalize((CameraInfo.view * vec4(light.direction.xyz, 0.0)).xyz);
        float cone = clamp((dot(L, -lightDirVS) - light.cone.y) /
                           max(light.cone.x - light.cone.y, 1e-6), 0.0, 1.0);
        float attenuation = pow(clamp(1.0 - distanceToLight / light.positionWithRadius.w, 0.0, 1.0), 2.0) * cone;
        float shadow = ForwardSpotShadow(light, worldPos, worldN,
                                         normalize(light.positionWithRadius.xyz - worldPos));
        color += ForwardBRDF(albedo, metallic, roughness, N, V, L,
                            light.colorWithIntensity.rgb * light.colorWithIntensity.w) * attenuation * shadow;
    }

    float ndotv = max(dot(N, V), 0.0);
    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    vec3 F = ForwardFresnelRoughness(f0, ndotv, roughness);
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
    vec3 diffuse = texture(forwardSkyIrradianceLUT, worldN).rgb * albedo;
    vec3 reflected = normalize(reflect(-worldV, worldN));
    vec3 prefiltered = textureLod(forwardSkySpecularLUT, reflected,
                                  roughness * (FORWARD_SKY_SPECULAR_LEVELS - 1.0)).rgb;
    vec2 brdf = texture(forwardBrdfLUT, vec2(ndotv, roughness)).rg;
    vec3 specular = prefiltered * (F * brdf.x + brdf.y);
    return color + (kD * diffuse + specular) * occlusion;
}

#endif
