#ifndef LIGHT_H
#define LIGHT_H

#define MAX_DIRECTIONAL_LIGHT_CNT 4
#define MAX_POINT_LIGHT_CNT 1024
#define MAX_SPOT_LIGHT_CNT 512
#define MAX_LIGHT_PER_CLUSTER 64
#define CLUSTER_DIM_X 16
#define CLUSTER_DIM_Y 8
#define CLUSTER_DIM_Z 16

struct DirectionalLight {
    vec4 direction; //xyz
    vec4 colorWithIntensity;     //w: intensity
};

struct PointLight
{
    vec4 positionWithRadius; // w: radius
    vec4 colorWithIntensity; // w: intensity
};

struct SpotLight
{
    vec4 positionWithRadius; // w: radius
    vec4 direction;          // xyz
    vec4 colorWithIntensity; // w: intensity
    vec4 cone;               // x:inner, y: outer
};


layout(std140, set = 0, binding = 1) uniform DirectionalLightBlockObject {
   DirectionalLight lights[MAX_DIRECTIONAL_LIGHT_CNT];
} DirectionalLights;

layout(std140, set = 0, binding = 2) uniform PointLightBlockObject {
   PointLight lights[MAX_POINT_LIGHT_CNT];
} PointLights;

layout(std140, set = 0, binding = 3) uniform SpotLightBlockObject {
   SpotLight lights[MAX_SPOT_LIGHT_CNT];
} SpotLights;

#ifdef CULLER_SIDE

layout(set = 0, binding = 4) writeonly buffer ClusterPointLightIndices {
    uint indices[];
} ClusterPointLights;

layout(set = 0, binding = 5) writeonly buffer ClusterSpotLightIndices {
    uint indices[];
} ClusterSpotLights;

#else

layout(set = 0, binding = 4) readonly buffer ClusterPointLightIndices {
    uint indices[];
} ClusterPointLights;

layout(set = 0, binding = 5) readonly buffer ClusterSpotLightIndices {
    uint indices[];
} ClusterSpotLights;

#endif

#endif