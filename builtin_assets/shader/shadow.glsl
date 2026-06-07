#ifndef SHADOW_H
#define SHADOW_H

#define MAX_DIRECTIONAL_LIGHT_SHADOW_CNT 1
#define MAX_DIRECTIONAL_SHADOW_CASCADE_CNT 4
#define MAX_SPOT_LIGHT_SHADOW_CNT 32
#define MAX_POINT_LIGHT_SHADOW_CNT 8

#ifdef SHADOW_MAP_PASS
#define SHADOW_DESCRIPTOR_SET 0
#else
#define SHADOW_DESCRIPTOR_SET 2
#endif

struct DirectionalShadowInfo
{
    uvec4 lightIndex; // x: directional light index
    uvec4 cascadeCnt; // x: active cascade cnt
    mat4 lightViewProj[MAX_DIRECTIONAL_SHADOW_CASCADE_CNT];
    vec4 cascadeSplits;
    vec4 cascadeTexelSizes;
};

struct SpotShadowInfo
{
    uvec4 lightIndex; // x: spot light index
    mat4 lightViewProj;
};

struct PointShadowInfo
{
    uvec4 lightIndex; // x: point light index
    mat4 lightViewProj[6];
};

layout(std140, set = SHADOW_DESCRIPTOR_SET, binding = 0) uniform DirectionalShadowBlockObject {
    DirectionalShadowInfo shadows[MAX_DIRECTIONAL_LIGHT_SHADOW_CNT];
} DirectionalShadows;

layout(set = SHADOW_DESCRIPTOR_SET, binding = 1) uniform sampler2DArrayShadow DirectionalShadowMaps[MAX_DIRECTIONAL_LIGHT_SHADOW_CNT];

#ifndef SHADOW_DIRECTIONAL_ONLY

layout(std140, set = SHADOW_DESCRIPTOR_SET, binding = 2) uniform SpotShadowBlockObject {
    SpotShadowInfo shadows[MAX_SPOT_LIGHT_SHADOW_CNT];
} SpotShadows;

layout(set = SHADOW_DESCRIPTOR_SET, binding = 3) uniform sampler2DShadow SpotShadowMaps[MAX_SPOT_LIGHT_SHADOW_CNT];

layout(std140, set = SHADOW_DESCRIPTOR_SET, binding = 4) uniform PointShadowBlockObject {
    PointShadowInfo shadows[MAX_POINT_LIGHT_SHADOW_CNT];
} PointShadows;

layout(set = SHADOW_DESCRIPTOR_SET, binding = 5) uniform samplerCube PointShadowMaps[MAX_POINT_LIGHT_SHADOW_CNT];

#endif

#undef SHADOW_DESCRIPTOR_SET

#endif
