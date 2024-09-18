#version 450 core

layout(set = 0, binding = 0) uniform VPBlockObject {
    mat4 view;
    mat4 projection;
    vec4 viewPos;
} VPBlock;

layout(location = 0) in vec3 aPos;
layout(location = 0) out vec3 TexCoords;

void main()
{
    vec3 rPos = vec3(aPos.x, -aPos.y, aPos.z);
    TexCoords = rPos;
    gl_Position = VPBlock.projection * mat4(mat3(VPBlock.view)) * vec4(rPos, 1.0);
}