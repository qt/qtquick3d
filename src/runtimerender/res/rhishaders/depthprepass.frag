#version 440

// no fragOutput, we do not want those validation warnings about outputting to (non-existing) attachment for location 0

layout(std140, binding = 0) uniform buf {
    mat4 modelViewProjection;
} ubuf;

void main()
{
}
