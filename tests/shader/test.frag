#version 450
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : enable
#extension GL_EXT_shader_8bit_storage : enable
//https://github.com/KhronosGroup/GLSL/blob/main/extensions/ext/GL_EXT_shader_16bit_storage.txt

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main() {
    int8_t a = int8_t(10);
    outColor = texture(texSampler, fragTexCoord);//vec4(fragTexCoord, 1.0);
    outColor.r /= float(a);
}