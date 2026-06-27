#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec2 vUV;
layout(location = 1) in vec4 vColor;
layout(location = 2) flat in uint vAtlasType;

layout(location = 0) out vec4 outColor;


layout(set = 0, binding = 0) uniform sampler2D uStaticGlyphAtlas;
layout(set = 0, binding = 1) uniform sampler2D uDynamicGlyphAtlas;

void main()
{
    float dist = vAtlasType == 0u
                         ? texture(uStaticGlyphAtlas, vUV).r
                         : texture(uDynamicGlyphAtlas, vUV).r;
    float smoothing = max(fwidth(dist), 1.0 / 255.0);
    float coverage = smoothstep(0.5 - smoothing, 0.5 + smoothing, dist);
    if (coverage <= 0.001)
        discard;

    outColor = vec4(vColor.rgb, vColor.a * coverage);
}
