// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#version 440

layout(std140, binding = 0) uniform buf {
    mat4 qt_modelMatrix;
    mat4 qt_viewMatrix;
    mat4 qt_projectionMatrix;
#ifdef QSSG_PARTICLES_ENABLE_VERTEX_LIGHTING
    // ParticleLightData struct
    vec4 qt_pointLightPosition[4];
    vec4 qt_pointLightConstantAtt;
    vec4 qt_pointLightLinearAtt;
    vec4 qt_pointLightQuadAtt;
    vec4 qt_pointLightColor[4];
    vec4 qt_spotLightPosition[4];
    vec4 qt_spotLightConstantAtt;
    vec4 qt_spotLightLinearAtt;
    vec4 qt_spotLightQuadAtt;
    vec4 qt_spotLightColor[4];
    vec4 qt_spotLightDirection[4];
    vec4 qt_spotLightConeAngle;
    vec4 qt_spotLightInnerConeAngle;
#endif
    vec4 qt_spriteConfig;
    vec3 qt_light_ambient_total;
    vec2 qt_oneOverParticleImageSize;
    uint qt_countPerSlice;
    float qt_billboard;
    float qt_opacity;
#ifdef QSSG_PARTICLES_ENABLE_VERTEX_LIGHTING
    bool qt_pointLights;
    bool qt_spotLights;
#endif
} ubuf;

#ifdef QSSG_PARTICLES_ENABLE_VERTEX_LIGHTING
/* Simplified lighting for particles
 *
 * Treat particles as points -> do not take normal into account
 * -> directional lights become ambient
 * -> point lights only account for attenuation
 * -> spotlights only account for attenuation and spot angles
 */
vec4 qt_attenuate4(in vec4 dist, in vec4 catt, in vec4 latt, in vec4 qatt)
{
    vec4 factors = catt + dist * latt + dist * dist * qatt;
    return vec4(1.0) / factors;
}
vec3 qt_calcPointLights(in vec3 position)
{
    vec4 lengths = vec4(length(position - ubuf.qt_pointLightPosition[0].xyz),
                        length(position - ubuf.qt_pointLightPosition[1].xyz),
                        length(position - ubuf.qt_pointLightPosition[2].xyz),
                        length(position - ubuf.qt_pointLightPosition[3].xyz));
    vec4 attenuations = qt_attenuate4(lengths, ubuf.qt_pointLightConstantAtt, ubuf.qt_pointLightLinearAtt, ubuf.qt_pointLightQuadAtt);

    return ubuf.qt_pointLightColor[0].rgb * attenuations.x + ubuf.qt_pointLightColor[1].rgb * attenuations.y + ubuf.qt_pointLightColor[2].rgb * attenuations.z + ubuf.qt_pointLightColor[3].rgb * attenuations.w;
}
vec3 qt_calcSpotLights(in vec3 position)
{
    vec3 lightVector0 = position - ubuf.qt_spotLightPosition[0].xyz;
    vec3 lightVector1 = position - ubuf.qt_spotLightPosition[1].xyz;
    vec3 lightVector2 = position - ubuf.qt_spotLightPosition[2].xyz;
    vec3 lightVector3 = position - ubuf.qt_spotLightPosition[3].xyz;
    vec4 lengths = vec4(length(lightVector0), length(lightVector1), length(lightVector2), length(lightVector3));
    vec4 attenuations = qt_attenuate4(lengths, ubuf.qt_spotLightConstantAtt, ubuf.qt_spotLightLinearAtt, ubuf.qt_spotLightQuadAtt);
    vec4 spots = vec4(acos(dot(normalize(lightVector0), normalize(ubuf.qt_spotLightDirection[0].xyz))),
                      acos(dot(normalize(lightVector1), normalize(ubuf.qt_spotLightDirection[1].xyz))),
                      acos(dot(normalize(lightVector2), normalize(ubuf.qt_spotLightDirection[2].xyz))),
                      acos(dot(normalize(lightVector3), normalize(ubuf.qt_spotLightDirection[3].xyz))));
    vec4 cones = vec4(1.0 - smoothstep(ubuf.qt_spotLightInnerConeAngle, ubuf.qt_spotLightConeAngle, spots));
    attenuations *= cones;
    return ubuf.qt_spotLightColor[0].rgb * attenuations.x + ubuf.qt_spotLightColor[1].rgb * attenuations.y + ubuf.qt_spotLightColor[2].rgb * attenuations.z + ubuf.qt_spotLightColor[3].rgb * attenuations.w;
}
vec3 qt_calcLightColor(in vec3 position)
{
    vec3 color = vec3(0.0);

    if (ubuf.qt_pointLights)
        color += qt_calcPointLights(position);
    if (ubuf.qt_spotLights)
        color += qt_calcSpotLights(position);
    return color + ubuf.qt_light_ambient_total;
}
#endif // QSSG_PARTICLES_ENABLE_VERTEX_LIGHTING

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
    color.a *= ubuf.qt_opacity;

#ifdef QSSG_PARTICLES_ENABLE_VERTEX_LIGHTING
    color.rgb *= qt_calcLightColor(worldPos.xyz);
#endif

    gl_Position = ubuf.qt_projectionMatrix * viewPos;
#ifdef QSSG_PARTICLES_ENABLE_MAPPED
    spriteData.x = qt_ageToSpriteFactor(p.age);
#endif
#ifdef QSSG_PARTICLES_ENABLE_ANIMATED
    spriteData.y = p.animationFrame;
#endif
}

