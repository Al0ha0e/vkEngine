#version 450 core

layout(set = 0, binding = 0) uniform VPBlockObject {
    mat4 view;
    mat4 projection;
    vec4 viewPos;
} VPBlock;

layout(location = 0) in vec3 Position;
layout(location=0) out vec3 Dir;

void main()
{
    vec3 rPos = vec3(Position.x, -Position.y, Position.z);
    Dir = rPos;
    gl_Position = VPBlock.projection * mat4(mat3(VPBlock.view)) * vec4(rPos, 1.0);
    gl_Position.z = gl_Position.w;
}