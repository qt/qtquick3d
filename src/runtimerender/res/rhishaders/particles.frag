#version 440

layout(std140, binding = 0) uniform buf {
    mat4 qt_modelMatrix;
    mat4 qt_viewMatrix;
    mat4 qt_projectionMatrix;
    vec4 qt_material_base_color;
    vec4 qt_spriteConfig;
    vec4 qt_colorConfig;
    vec3 qt_light_ambient_total;
    vec2 qt_oneOverParticleImageSize;
    vec2 qt_cameraProps;
    uint qt_countPerSlice;
    float qt_billboard;
} ubuf;


layout(binding = 1) uniform sampler2D qt_sprite;
layout(binding = 3) uniform sampler2D qt_colorTable;

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec4 color;
layout(location = 1) in float spriteFactor;
layout(location = 2) in vec2 texcoord;
layout(location = 3) flat in uint instanceIndex;

/*
    qt_spriteConfig
    x: number of images
    y: one over number of images
    z: enable gradient
    w: blend between images
*/
float qt_spriteCoords(out vec2 coords[2])
{
    float imageOffset = (ubuf.qt_spriteConfig.x - ubuf.qt_spriteConfig.w) * spriteFactor;
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
    vec4 ret;
    if (ubuf.qt_spriteConfig.x > 1.0) {
        vec2 coords[2];
        float factor = qt_spriteCoords(coords);
        ret = mix(texture(qt_sprite, coords[0]), texture(qt_sprite, coords[1]), factor);
    } else {
        ret = texture(qt_sprite, vec2(mix(texcoord.x, spriteFactor, ubuf.qt_spriteConfig.z), texcoord.y));
    }
    return ret;
}

float qt_rand(vec2 co)
{
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

vec4 qt_readColor()
{
    if (ubuf.qt_colorConfig.x > 0.0) {
        return color * texture(qt_colorTable, vec2(spriteFactor, qt_rand(vec2(instanceIndex, 0))));
    }
    return color;
}

void main()
{
    fragColor = qt_readColor() * qt_readSprite();
}
