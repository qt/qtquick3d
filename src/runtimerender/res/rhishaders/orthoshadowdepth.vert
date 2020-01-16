#version 440

layout(location = 0) in vec3 attr_pos;

layout(location = 0) out vec3 out_depth;

layout(std140, binding = 0) uniform buf {
    mat4 modelViewProjection;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    gl_Position = ubuf.modelViewProjection * vec4(attr_pos, 1.0);
    out_depth.x = gl_Position.z / gl_Position.w;
}
