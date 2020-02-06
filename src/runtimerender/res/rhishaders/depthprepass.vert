#version 440

layout(location = 0) in vec3 attr_pos;

layout(std140, binding = 0) uniform buf {
    mat4 modelViewProjection;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    gl_Position = ubuf.modelViewProjection * vec4(attr_pos, 1.0);
}
