#version 440

layout(location = 0) out vec4 fragOutput;

layout(location = 0) in vec2 uv_coord;

layout(binding = 0) uniform sampler2D tex;

void main()
{
    fragOutput = texture(tex, uv_coord);
}
