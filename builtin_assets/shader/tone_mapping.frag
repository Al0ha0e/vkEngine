#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D hdrColor;

layout(push_constant) uniform ToneMappingConstants {
    float exposure;
    uint toneMappingMode;
} constants;

vec3 ACESFilm(vec3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 Reinhard(vec3 x)
{
    return x / (x + vec3(1.0));
}

void main()
{
    vec3 hdr = texture(hdrColor, vTexCoord).rgb;
    vec3 exposed = hdr * constants.exposure;
    vec3 mapped = constants.toneMappingMode == 1 ? Reinhard(exposed) : ACESFilm(exposed);

    outColor = vec4(mapped, 1.0);
}
