#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#define SHADOW_MAP_PASS
#define SHADOW_DIRECTIONAL_ONLY
#include "shadow.glsl"

layout(push_constant) uniform PushConstants {
    mat4 model;
    uint cascadeIndex;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexCoord;

void main()
{
    gl_Position = DirectionalShadows.shadows[0].lightViewProj[cascadeIndex] * model * vec4(inPosition, 1.0);
}