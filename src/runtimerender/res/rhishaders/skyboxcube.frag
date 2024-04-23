#version 440

layout(location = 0) out vec4 fragOutput;

layout(location = 0) in vec3 tex_coords;

layout(binding = 1) uniform samplerCube tex;

void main()
{
    fragOutput = texture(tex, tex_coords);
}
