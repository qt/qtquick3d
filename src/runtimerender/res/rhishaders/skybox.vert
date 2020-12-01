#version 440

layout(location = 0) in vec3 attr_pos;

layout(std140, binding = 0) uniform buf {
    mat4 viewMatrix;
    mat4 inverseProjection;
    mat4 orientation;
    float adjustY;
    float exposure;
} ubuf;

layout(location = 0) out vec3 eye_direction;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    gl_Position = vec4(attr_pos, 1.0);
    vec3 unprojected = (ubuf.inverseProjection * gl_Position).xyz;
    eye_direction = normalize(mat3(ubuf.viewMatrix) * unprojected);
    eye_direction = normalize(mat3(ubuf.orientation) * eye_direction);
    gl_Position.y *= ubuf.adjustY;
}
