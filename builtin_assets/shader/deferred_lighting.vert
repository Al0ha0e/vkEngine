#version 450

#extension GL_GOOGLE_include_directive : enable

#include "light.glsl"

layout(location = 0) out vec2 vTexCoord;
layout(location = 1) out vec3 vViewSpaceLightDir;

layout(set = 0, binding = 0) uniform VPBlockObject {
    mat4 view;
    mat4 projection;
    vec4 viewPos;
} VPBlock;


vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0,  3.0),
    vec2( 3.0, -1.0)
);

void main()
{
    vec2 pos = positions[gl_VertexIndex];
    vTexCoord = pos * 0.5 + 0.5;
    DirectionalLight light = DirectionalLights.lights[0];
    vViewSpaceLightDir = (VPBlock.view * vec4(light.direction.xyz, 0.0)).xyz;
    gl_Position = vec4(pos, 0.0, 1.0);
}