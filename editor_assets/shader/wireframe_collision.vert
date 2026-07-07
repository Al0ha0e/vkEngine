#version 460

layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    vec4 color;
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    gl_Position = pc.viewProj * vec4(inPosition, 1.0);
    outColor = pc.color;
}
