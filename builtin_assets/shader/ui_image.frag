#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 1) uniform sampler2D textures[];
layout(location = 0) in vec2 vUV;
layout(location = 1) in vec4 vColor;
layout(location = 2) flat in uint vTextureIndex;
layout(location = 0) out vec4 outColor;

void main()
{
    outColor = texture(textures[nonuniformEXT(vTextureIndex)], vUV) * vColor;
}
