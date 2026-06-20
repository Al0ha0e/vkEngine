#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#define SHADOW_MAP_PASS
#include "shadow.glsl"

layout(push_constant) uniform PushConstants {
    mat4 model;
    uint shadowIndex;
    uint shadowType;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexCoord;

void main()
{
    mat4 lightViewProj = shadowType == 0
        ? DirectionalShadows.shadows[0].lightViewProj[shadowIndex]
        : SpotShadows.shadows[shadowIndex].lightViewProj;
    gl_Position = lightViewProj * model * vec4(inPosition, 1.0);
}
