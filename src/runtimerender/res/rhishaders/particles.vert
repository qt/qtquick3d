#version 440

layout(std140, binding = 0) uniform buf {
    mat4 qt_modelMatrix;
    mat4 qt_viewMatrix;
    mat4 qt_projectionMatrix;
    vec4 qt_material_base_color;
    vec4 qt_spriteConfig;
    vec3 qt_light_ambient_total;
    vec2 qt_oneOverParticleImageSize;
    vec2 qt_cameraProps;
    uint qt_countPerSlice;
    float qt_billboard;
} ubuf;

layout(binding = 2) uniform sampler2D qt_particleTexture;

out gl_PerVertex {
    vec4 gl_Position;
};

#ifdef QSSG_PARTICLES_ENABLE_ANIMATED
const uint particleSize = 4;
#else
const uint particleSize = 3;
#endif

struct Particle
{
    vec3 position;
    float size;
    vec3 rotation;
    float age;
    vec4 color;
#ifdef QSSG_PARTICLES_ENABLE_ANIMATED
    float animationFrame;
    vec3 unusedPadding;
#endif
};

vec2 qt_indexToUV(in uint index)
{
    uint v = index / ubuf.qt_countPerSlice;
    uint u = index - ubuf.qt_countPerSlice * v;
    return (vec2(u * particleSize, v) + vec2(0.5)) * ubuf.qt_oneOverParticleImageSize;
}

Particle qt_loadParticle(in uint index)
{
    Particle p;
    vec2 offset = qt_indexToUV(index);
    vec4 p0 = texture(qt_particleTexture, offset);
    vec4 p1 = texture(qt_particleTexture, vec2(offset.x + ubuf.qt_oneOverParticleImageSize.x, offset.y));
    vec4 p2 = texture(qt_particleTexture, vec2(offset.x + 2.0 * ubuf.qt_oneOverParticleImageSize.x, offset.y));
#ifdef QSSG_PARTICLES_ENABLE_ANIMATED
    vec4 p3 = texture(qt_particleTexture, vec2(offset.x + 3.0 * ubuf.qt_oneOverParticleImageSize.x, offset.y));
#endif
    p.position = p0.xyz;
    p.size = p0.w;
    p.rotation = p1.xyz;
    p.age = p1.w;
    p.color = p2;
#ifdef QSSG_PARTICLES_ENABLE_ANIMATED
    p.animationFrame = p3.x;
#endif
    return p;
}

mat3 qt_fromEulerRotation(in vec3 rotation)
{
    float a = cos(rotation.x);
    float b = sin(rotation.x);
    float c = cos(rotation.y);
    float d = sin(rotation.y);
    float e = cos(rotation.z);
    float f = sin(rotation.z);
    mat3 ret;
    float bd = b*d;
    float ad = a*d;
    ret[0][0] = c * e;
    ret[0][1] = -c * f;
    ret[0][2] = d;
    ret[1][0] = bd*e + a*f;
    ret[1][1] = a*e - bd*f;
    ret[1][2] = -b*c;
    ret[2][0] = b*f - ad*e;
    ret[2][1] = ad*f + b*e;
    ret[2][2] = a*c;
    return ret;
}

float qt_ageToSpriteFactor(in float age)
{
    // Age should be [0, 1], but just in case take the fraction
    return fract(age);
}

layout(location = 0) out vec4 color;
layout(location = 1) out vec2 spriteData;
layout(location = 2) out vec2 texcoord;
layout(location = 3) flat out uint instanceIndex;

const vec2 corners[4] = {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, {1.0, 1.0}};

void main()
{
    uint particleIndex = gl_InstanceIndex;
    instanceIndex = particleIndex;
    uint cornerIndex = gl_VertexIndex;
    vec2 corner = corners[cornerIndex];
    Particle p = qt_loadParticle(particleIndex);
    mat3 rotMat = qt_fromEulerRotation(p.rotation);
    vec4 worldPos = ubuf.qt_modelMatrix * vec4(p.position, 1.0);
    vec4 viewPos = worldPos;
    vec3 offset = (p.size * vec3(corner - vec2(0.5), 0.0));
    viewPos.xyz += offset * rotMat * (1.0 - ubuf.qt_billboard);
    viewPos = ubuf.qt_viewMatrix * viewPos;
    viewPos.xyz += rotMat * offset * ubuf.qt_billboard;
    texcoord = corner;
    color = p.color;
    gl_Position = ubuf.qt_projectionMatrix * viewPos;
#ifdef QSSG_PARTICLES_ENABLE_MAPPED
    spriteData.x = qt_ageToSpriteFactor(p.age);
#endif
#ifdef QSSG_PARTICLES_ENABLE_ANIMATED
    spriteData.y = p.animationFrame;
#endif
}

