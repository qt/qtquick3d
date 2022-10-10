#version 440

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;

layout(location = 0) out vec4 position;
layout(location = 1) out vec4 normal;
layout(location = 2) out vec4 baseColor;
layout(location = 3) out vec4 emissive;

layout(std140, binding = 0) uniform buf {
    vec4 baseColorLinear;
    vec3 emissiveFactor;
    int flipY;
    int hasBaseColorMap;
    int hasEmissiveMap;
};

layout(binding = 1) uniform sampler2D baseColorMap;
layout(binding = 2) uniform sampler2D emissiveMap;

vec3 sRGBToLinear(vec3 c)
{
    return c * (c * (c * 0.305306011 + 0.682171111) + 0.012522878);
}
vec4 sRGBToLinear(vec4 c)
{
    return vec4(sRGBToLinear(c.rgb), c.a);
}

void main()
{
    position = vec4(v_pos, 1.0);
    normal = vec4(normalize(v_normal), 1.0);

    baseColor = baseColorLinear;
    if (hasBaseColorMap != 0)
        baseColor *= sRGBToLinear(texture(baseColorMap, v_uv));

    emissive = vec4(emissiveFactor, 1.0);
    if (hasEmissiveMap != 0)
        emissive.rgb *= sRGBToLinear(texture(emissiveMap, v_uv).rgb);
}
