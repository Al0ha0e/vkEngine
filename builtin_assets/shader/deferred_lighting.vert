#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "camera.glsl"
#include "light.glsl"

layout(location = 0) out vec2 vTexCoord;

vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2(-1.0,  3.0),
    vec2( 3.0, -1.0)
);

void main()
{
    vec2 pos = positions[gl_VertexIndex];
    vTexCoord = pos * 0.5 + 0.5;
    gl_Position = vec4(pos, 0.0, 1.0);
}