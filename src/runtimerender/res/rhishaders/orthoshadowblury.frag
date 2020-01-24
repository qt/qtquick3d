#version 440

layout(location = 0) in vec2 uv_coords;

layout(location = 0) out vec4 fragOutput;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec2 cameraProperties;
} ubuf;

layout(binding = 1) uniform sampler2D depthSrc;

void main()
{
    vec2 ofsScale = vec2(0.0, ubuf.cameraProperties.x / 7680.0);
    float depth0 = texture(depthSrc, uv_coords).x;
    float depth1 = texture(depthSrc, uv_coords + ofsScale).x;
    depth1 += texture(depthSrc, uv_coords - ofsScale).x;
    float depth2 = texture(depthSrc, uv_coords + 2.0 * ofsScale).x;
    depth2 += texture(depthSrc, uv_coords - 2.0 * ofsScale).x;
    float outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;
    fragOutput = vec4(outDepth);
}
