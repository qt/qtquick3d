#version 440

layout(location = 0) in vec3 out_depth;

layout(location = 0) out vec4 fragOutput;

void main()
{
    float depth = (out_depth.x + 1.0) * 0.5;
    fragOutput = vec4(depth);
}
