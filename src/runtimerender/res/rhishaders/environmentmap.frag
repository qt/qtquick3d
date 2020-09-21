#version 440

layout(location = 0) in vec3 localPos;
layout(location = 0) out vec4 FragColor;

layout(binding = 1) uniform sampler2D environmentMap;

vec2 sampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= vec2(0.1591, 0.3183);
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = sampleSphericalMap(normalize(localPos)); // make sure to normalize localPos

    FragColor = texture(environmentMap, uv);
}
