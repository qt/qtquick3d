#version 440

layout(location = 0) out vec4 fragOutput;

layout(location = 0) in vec2 uv_coord;

layout(std140, binding = 0) uniform buf {
    mat4 modelViewProjection;
    vec2 layerDimensions;
    float opacity;
} ubuf;

layout(binding = 1) uniform sampler2D layerImage;

void main()
{
    vec4 c = texture(layerImage, uv_coord);
    fragOutput = c * ubuf.opacity;
}
