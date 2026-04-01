#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec4 vColor;

layout(location = 0) out vec4 outColor;


layout(set = 0, binding = 1) uniform sampler2D uGlyphAtlas;

void main()
{
    vec4 atlasSample = texture(uGlyphAtlas, vUV);
    float coverage = min(atlasSample.r, atlasSample.a);
    if (coverage <= 0.5)
        discard;

    outColor = vec4(vColor.rgb, 1.0);
}