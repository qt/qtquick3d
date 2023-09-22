#version 440

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec2 v_uv;

layout(std140, binding = 0) uniform buf {
    mat4 mvp;
    vec4 color;
};

void main()
{
    v_uv = uv;
    gl_Position = mvp * position;
}
