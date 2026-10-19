#version 450

struct Quad
{
    vec4 quadRect;
    vec4 uvRect;
    vec4 color;
    uvec4 atlasInfo; // x = image texture index
};

layout(location = 0) in uint quadID;
layout(set = 0, binding = 0, std430) readonly buffer QuadBuffer
{
    Quad quads[];
} quadBuffer;

layout(push_constant) uniform PushConstants
{
    mat4 model;
    vec2 viewportSize;
    vec2 atlasSize;
} pc;

layout(location = 0) out vec2 vUV;
layout(location = 1) out vec4 vColor;
layout(location = 2) flat out uint vTextureIndex;

const vec2 quadCorners[6] = vec2[](
    vec2(0, 0), vec2(1, 0), vec2(1, 1),
    vec2(0, 0), vec2(1, 1), vec2(0, 1));

void main()
{
    Quad quad = quadBuffer.quads[quadID];
    vec2 corner = quadCorners[gl_VertexIndex];
    vec2 localPos = mix(quad.quadRect.xw, quad.quadRect.zy, corner);
    vec2 screenPos = (pc.model * vec4(localPos, 0, 1)).xy;
    gl_Position = vec4(screenPos / pc.viewportSize * 2.0 - 1.0, 0, 1);
    vUV = mix(quad.uvRect.xw, quad.uvRect.zy, corner);
    vColor = quad.color;
    vTextureIndex = quad.atlasInfo.x;
}
