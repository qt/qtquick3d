#version 440

layout(location = 0) in vec2 uv_coords;

layout(location = 0) out vec4 frag0;
layout(location = 1) out vec4 frag1;
layout(location = 2) out vec4 frag2;
layout(location = 3) out vec4 frag3;
layout(location = 4) out vec4 frag4;
layout(location = 5) out vec4 frag5;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec2 cameraProperties;
} ubuf;

layout(binding = 1) uniform samplerCube depthCube;

void main()
{
    float ofsScale = ubuf.cameraProperties.x / 2500.0;
    vec3 dir0 = vec3(1.0, -uv_coords.y, -uv_coords.x);
    vec3 dir1 = vec3(-1.0, -uv_coords.y, uv_coords.x);
    vec3 dir2 = vec3(uv_coords.x, 1.0, uv_coords.y);
    vec3 dir3 = vec3(uv_coords.x, -1.0, -uv_coords.y);
    vec3 dir4 = vec3(uv_coords.x, -uv_coords.y, 1.0);
    vec3 dir5 = vec3(-uv_coords.x, -uv_coords.y, -1.0);
    float depth0;
    float depth1;
    float depth2;
    float outDepth;
    depth0 = texture(depthCube, dir0).x;
    depth1 = texture(depthCube, dir0 + vec3(0.0, -ofsScale, 0.0)).x;
    depth1 += texture(depthCube, dir0 + vec3(0.0, ofsScale, 0.0)).x;
    depth2 = texture(depthCube, dir0 + vec3(0.0, -2.0*ofsScale, 0.0)).x;
    depth2 += texture(depthCube, dir0 + vec3(0.0, 2.0*ofsScale, 0.0)).x;
    outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;
    frag0 = vec4(outDepth);

    depth0 = texture(depthCube, dir1).x;
    depth1 = texture(depthCube, dir1 + vec3(0.0, -ofsScale, 0.0)).x;
    depth1 += texture(depthCube, dir1 + vec3(0.0, ofsScale, 0.0)).x;
    depth2 = texture(depthCube, dir1 + vec3(0.0, -2.0*ofsScale, 0.0)).x;
    depth2 += texture(depthCube, dir1 + vec3(0.0, 2.0*ofsScale, 0.0)).x;
    outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;
    frag1 = vec4(outDepth);

    depth0 = texture(depthCube, dir2).x;
    depth1 = texture(depthCube, dir2 + vec3(0.0, 0.0, -ofsScale)).x;
    depth1 += texture(depthCube, dir2 + vec3(0.0, 0.0, ofsScale)).x;
    depth2 = texture(depthCube, dir2 + vec3(0.0, 0.0, -2.0*ofsScale)).x;
    depth2 += texture(depthCube, dir2 + vec3(0.0, 0.0, 2.0*ofsScale)).x;
    outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;
    frag2 = vec4(outDepth);

    depth0 = texture(depthCube, dir3).x;
    depth1 = texture(depthCube, dir3 + vec3(0.0, 0.0, -ofsScale)).x;
    depth1 += texture(depthCube, dir3 + vec3(0.0, 0.0, ofsScale)).x;
    depth2 = texture(depthCube, dir3 + vec3(0.0, 0.0, -2.0*ofsScale)).x;
    depth2 += texture(depthCube, dir3 + vec3(0.0, 0.0, 2.0*ofsScale)).x;
    outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;
    frag3 = vec4(outDepth);

    depth0 = texture(depthCube, dir4).x;
    depth1 = texture(depthCube, dir4 + vec3(0.0, -ofsScale, 0.0)).x;
    depth1 += texture(depthCube, dir4 + vec3(0.0, ofsScale, 0.0)).x;
    depth2 = texture(depthCube, dir4 + vec3(0.0, -2.0*ofsScale, 0.0)).x;
    depth2 += texture(depthCube, dir4 + vec3(0.0, 2.0*ofsScale, 0.0)).x;
    outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;
    frag4 = vec4(outDepth);

    depth0 = texture(depthCube, dir5).x;
    depth1 = texture(depthCube, dir5 + vec3(0.0, -ofsScale, 0.0)).x;
    depth1 += texture(depthCube, dir5 + vec3(0.0, ofsScale, 0.0)).x;
    depth2 = texture(depthCube, dir5 + vec3(0.0, -2.0*ofsScale, 0.0)).x;
    depth2 += texture(depthCube, dir5 + vec3(0.0, 2.0*ofsScale, 0.0)).x;
    outDepth = 0.38774 * depth0 + 0.24477 * depth1 + 0.06136 * depth2;
    frag5 = vec4(outDepth);
}
