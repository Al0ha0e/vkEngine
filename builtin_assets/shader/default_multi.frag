#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(push_constant) uniform PushConstants{
    mat4 model;
    uvec4 textureIndices;
};

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform sampler2D textures[];

void main() {
    outColor = texture(textures[nonuniformEXT(textureIndices.x)], fragTexCoord);
}