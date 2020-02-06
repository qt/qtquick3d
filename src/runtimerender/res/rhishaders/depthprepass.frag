#version 440

layout(location = 0) out vec4 fragOutput;

layout(std140, binding = 0) uniform buf {
    mat4 modelViewProjection;
} ubuf;

void main()
{
    fragOutput = vec4(0.0);
}
