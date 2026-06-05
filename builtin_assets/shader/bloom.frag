#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D hdrColor;

layout(push_constant) uniform BloomConstants {
    float threshold;
    float intensity;
    float radius;
    float knee;
} constants;

vec3 ExtractBloom(vec3 color)
{
    float brightness = max(max(color.r, color.g), color.b);
    float soft = clamp((brightness - constants.threshold + constants.knee) / max(constants.knee * 2.0, 0.0001), 0.0, 1.0);
    float contribution = max(brightness - constants.threshold, 0.0) + soft * soft * constants.knee;
    return color * (contribution / max(brightness, 0.0001));
}

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(hdrColor, 0));
    vec3 hdr = texture(hdrColor, vTexCoord).rgb;
    vec3 bloom = ExtractBloom(hdr) * 0.227027;

    bloom += ExtractBloom(texture(hdrColor, vTexCoord + texelSize * vec2( constants.radius,  0.0)).rgb) * 0.1945946;
    bloom += ExtractBloom(texture(hdrColor, vTexCoord + texelSize * vec2(-constants.radius,  0.0)).rgb) * 0.1945946;
    bloom += ExtractBloom(texture(hdrColor, vTexCoord + texelSize * vec2( 0.0,  constants.radius)).rgb) * 0.1216216;
    bloom += ExtractBloom(texture(hdrColor, vTexCoord + texelSize * vec2( 0.0, -constants.radius)).rgb) * 0.1216216;
    bloom += ExtractBloom(texture(hdrColor, vTexCoord + texelSize * vec2( constants.radius,  constants.radius)).rgb) * 0.0702703;
    bloom += ExtractBloom(texture(hdrColor, vTexCoord + texelSize * vec2(-constants.radius,  constants.radius)).rgb) * 0.0702703;
    bloom += ExtractBloom(texture(hdrColor, vTexCoord + texelSize * vec2( constants.radius, -constants.radius)).rgb) * 0.0702703;
    bloom += ExtractBloom(texture(hdrColor, vTexCoord + texelSize * vec2(-constants.radius, -constants.radius)).rgb) * 0.0702703;

    outColor = vec4(hdr + bloom * constants.intensity, 1.0);
}
