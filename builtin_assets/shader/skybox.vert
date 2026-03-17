#version 450 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "camera.glsl"

layout(location = 0) in vec3 Position;
layout(location=0) out vec3 Dir;

void main()
{
    vec3 rPos = vec3(Position.x, -Position.y, Position.z);
    Dir = rPos;
    gl_Position = CameraInfo.projection * mat4(mat3(CameraInfo.view)) * vec4(rPos, 1.0);
    gl_Position.z = gl_Position.w;
}