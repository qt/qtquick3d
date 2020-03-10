#version 440

layout(location = 0) out vec4 fragOutput;

layout(location = 0) in vec2 uv_coord;

layout(std140, binding = 0) uniform buf {
    vec2 blend;
} ubuf;

layout(binding = 1) uniform sampler2D tex1;
layout(binding = 2) uniform sampler2D tex2;

void main()
{
    vec4 t1 = texture(tex1, uv_coord);
    vec4 t2 = texture(tex2, uv_coord);
    fragOutput = t1 * ubuf.blend.x + t2 * ubuf.blend.y;
}
