#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

struct Glyph
{
    vec4 quadRect; // xy = min, zw = max in target pixels
    vec4 uvRect;   // xy = min, zw = max in atlas pixels
    vec4 color;      // rgba tint
};

layout(set = 0, binding = 0, std430) readonly buffer GlyphBuffer
{
    Glyph glyphs[];
};
layout(set = 0, binding = 1) uniform sampler2D uGlyphAtlas;

layout(push_constant) uniform PushConstants
{
    vec2 viewportSize;
    vec2 atlasSize;
} pc;

layout(location = 0) out vec2 vUV;
layout(location = 1) out vec4 vColor;

const vec2 quadCorners[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 0.0),
    vec2(1.0, 1.0),
    vec2(0.0, 1.0));

void main()
{
    Glyph glyph = glyphs[gl_InstanceIndex];
    vec2 corner = quadCorners[gl_VertexIndex];

    vec2 screenPos = mix(glyph.quadRect.xw, glyph.quadRect.zy, corner);
    vec2 uv = mix(glyph.uvRect.xw, glyph.uvRect.zy, corner);

    vec2 ndc = vec2(
        screenPos.x / pc.viewportSize.x * 2.0 - 1.0,
        screenPos.y / pc.viewportSize.y * 2.0 - 1.0);

    gl_Position = vec4(ndc, 0.0, 1.0);

    vUV = uv / pc.atlasSize;
    vColor = glyph.color;
}