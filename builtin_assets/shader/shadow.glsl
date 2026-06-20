#ifndef SHADOW_H
#define SHADOW_H

#define MAX_DIRECTIONAL_LIGHT_SHADOW_CNT 1
#define MAX_DIRECTIONAL_SHADOW_CASCADE_CNT 4
#define MAX_SPOT_LIGHT_SHADOW_CNT 32
#define MAX_POINT_LIGHT_SHADOW_CNT 8

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
    mat4 lightViewProj;
};

struct PointShadowInfo
{
    uvec4 lightIndex; // x: point light index
    mat4 lightViewProj[6];
};

#ifdef SHADOW_MAP_PASS

layout(std140, set = 0, binding = 0) uniform DirectionalShadowBlockObject {
    DirectionalShadowInfo shadows[MAX_DIRECTIONAL_LIGHT_SHADOW_CNT];
} DirectionalShadows;

layout(std140, set = 0, binding = 1) uniform SpotShadowBlockObject {
    SpotShadowInfo shadows[MAX_SPOT_LIGHT_SHADOW_CNT];
} SpotShadows;

#else

layout(std140, set = 2, binding = 0) uniform DirectionalShadowBlockObject {
    DirectionalShadowInfo shadows[MAX_DIRECTIONAL_LIGHT_SHADOW_CNT];
} DirectionalShadows;

layout(set = 2, binding = 1) uniform sampler2DArrayShadow DirectionalShadowMaps[MAX_DIRECTIONAL_LIGHT_SHADOW_CNT];

layout(std140, set = 2, binding = 2) uniform SpotShadowBlockObject {
    SpotShadowInfo shadows[MAX_SPOT_LIGHT_SHADOW_CNT];
} SpotShadows;

layout(set = 2, binding = 3) uniform sampler2DArrayShadow SpotShadowMap;

// Point light shadow descriptors are intentionally disabled until point shadow rendering is implemented.
// layout(std140, set = 2, binding = 4) uniform PointShadowBlockObject {
//     PointShadowInfo shadows[MAX_POINT_LIGHT_SHADOW_CNT];
// } PointShadows;

// layout(set = 2, binding = 5) uniform samplerCube PointShadowMaps[MAX_POINT_LIGHT_SHADOW_CNT];

#endif

#endif
