#version 440

layout(location = 0) in vec2 uv_coords;

layout(location = 0) out vec4 fragOutput;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec4 cameraProperties;
} ubuf;

layout(binding = 1) uniform sampler2DArray depthSrc;

void main()
{
    vec2 ofsScale = vec2(ubuf.cameraProperties.x / 7680.0, 0.0);
    float depth0 = texture(depthSrc, vec3(uv_coords, ubuf.cameraProperties.z)).x;
    float depth1 = texture(depthSrc, vec3(uv_coords + ofsScale, ubuf.cameraProperties.z)).x;
    depth1 += texture(depthSrc, vec3(uv_coords - ofsScale, ubuf.cameraProperties.z)).x;
    float depth2 = texture(depthSrc, vec3(uv_coords + 2.0 * ofsScale, ubuf.cameraProperties.z)).x;
    depth2 += texture(depthSrc, vec3(uv_coords - 2.0 * ofsScale, ubuf.cameraProperties.z)).x;
    float outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;
    fragOutput = vec4(outDepth);
}
