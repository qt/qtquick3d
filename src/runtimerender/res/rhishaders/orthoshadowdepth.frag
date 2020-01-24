#version 440

layout(location = 0) in vec3 out_depth;

layout(location = 0) out vec4 fragOutput;

layout(std140, binding = 0) uniform buf {
    mat4 modelViewProjection;
    vec2 depthAdjust;
} ubuf;

void main()
{
    float depth = (out_depth.x + ubuf.depthAdjust.x) * ubuf.depthAdjust.y;
    fragOutput = vec4(depth);
}
