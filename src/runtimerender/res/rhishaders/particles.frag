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


layout(binding = 1) uniform sampler2D qt_sprite;

layout(location = 0) out vec4 fragColor;

layout(location = 0) in vec4 color;
layout(location = 1) in float spriteFactor;
layout(location = 2) in vec2 texcoord;

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

void main()
{
    fragColor = color * qt_readSprite();
}
