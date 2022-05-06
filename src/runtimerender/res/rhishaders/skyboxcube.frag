#version 440

layout(location = 0) out vec4 fragOutput;

layout(location = 0) in vec3 tex_coords;

layout(binding = 1) uniform samplerCube tex;


layout(std140, binding = 0) uniform buf {
    mat3 viewMatrix;
    mat4 inverseProjection;
    mat3 orientation;
    vec4 skyboxProperties;
    mat4 viewProjection;
} ubuf;


void main()
{
    fragOutput = texture(tex, tex_coords);

}
