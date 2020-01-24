#version 440

layout(location = 0) in vec3 attr_pos;

layout(location = 0) out vec4 raw_pos;
layout(location = 1) out vec4 world_pos;

layout(std140, binding = 0) uniform buf {
    mat4 modelViewProjection;
    mat4 modelMatrix;
    vec3 cameraPosition;
    vec2 cameraProperties;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    world_pos = ubuf.modelMatrix * vec4(attr_pos, 1.0);
    world_pos /= world_pos.w;
    gl_Position = ubuf.modelViewProjection * vec4(attr_pos, 1.0);
    raw_pos = vec4(attr_pos, 1.0);
}
