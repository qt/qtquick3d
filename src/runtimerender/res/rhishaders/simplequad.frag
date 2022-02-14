#version 440

layout(location = 0) out vec4 fragOutput;

layout(location = 0) in vec2 uv_coord;

layout(binding = 0) uniform sampler2D tex;

void main()
{
#if QSHADER_HLSL || QSHADER_MSL
    vec2 uv = vec2(uv_coord.x, 1.0 - uv_coord.y);
    fragOutput = texture(tex, uv);
#else
    fragOutput = texture(tex, uv_coord);
#endif
}
