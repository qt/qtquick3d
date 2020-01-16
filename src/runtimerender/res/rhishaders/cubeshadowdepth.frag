#version 440

layout(location = 0) in vec4 raw_pos;
layout(location = 1) in vec4 world_pos;

layout(location = 0) out vec4 fragOutput;

layout(std140, binding = 0) uniform buf {
    vec3 cameraPosition;
    vec2 cameraProperties;
} ubuf;

void main()
{
    vec3 camPos = vec3(ubuf.cameraPosition.x, ubuf.cameraPosition.y, -ubuf.cameraPosition.z );
    float dist = length(world_pos.xyz - camPos);
    dist = (dist - ubuf.cameraProperties.x) / (ubuf.cameraProperties.y - ubuf.cameraProperties.x);
    fragOutput = vec4(dist, dist, dist, 1.0);
}
