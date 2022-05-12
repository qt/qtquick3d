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
    uint qt_lineSegmentCount;
    float qt_billboard;
    float qt_opacity;
    float qt_alphaFade;
    float qt_sizeModifier;
    float qt_texcoordScale;
#ifdef QSSG_PARTICLES_ENABLE_VERTEX_LIGHTING
    bool qt_pointLights;
    bool qt_spotLights;
#endif
} ubuf;


layout(binding = 1) uniform sampler2D qt_sprite;
layout(binding = 3) uniform sampler2D qt_colorTable;

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec4 color;
/*
    spriteData
    x: particle age, 0..1 during the lifetime
    y: particle lifetime over sprite animation duration
*/
layout(location = 1) in vec2 spriteData;
layout(location = 2) in vec2 texcoord;
layout(location = 3) flat in uint instanceIndex;

/*
    qt_spriteConfig
    x: number of images
    y: one over number of images
    z: unused
    w: blend between images
*/
float qt_spriteCoords(out vec2 coords[2])
{
    float imageOffset = (ubuf.qt_spriteConfig.x - ubuf.qt_spriteConfig.w) * spriteData.y;
    float factor = fract(imageOffset);
    vec2 imageOffsets = vec2(imageOffset - factor, imageOffset - factor + 1.0);
    imageOffsets = clamp(imageOffsets * ubuf.qt_spriteConfig.y, 0.0, 1.0);
    coords[0] = coords[1] = vec2(texcoord.x * ubuf.qt_spriteConfig.y, texcoord.y);
    coords[0].x += imageOffsets.x;
    coords[1].x += imageOffsets.y;
    return factor * ubuf.qt_spriteConfig.w;
}

vec4 qt_readSprite()
{
#ifdef QSSG_PARTICLES_ENABLE_ANIMATED
    vec2 coords[2];
    float factor = qt_spriteCoords(coords);
    return mix(texture(qt_sprite, coords[0]), texture(qt_sprite, coords[1]), factor);
#else
    return texture(qt_sprite, texcoord);
#endif
}

float qt_rand(vec2 co)
{
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

vec4 qt_readColor()
{
#ifdef QSSG_PARTICLES_ENABLE_MAPPED
    return color * texture(qt_colorTable, vec2(spriteData.x, qt_rand(vec2(instanceIndex, 0))));
#else
    return color;
#endif
}

void main()
{
    fragColor = qt_readColor() * qt_readSprite();
}
