#version 440

#ifndef PI
#define PI          3.14159265358979
#define PI_HALF     ( 0.5 * PI )
#define PI_TWO      ( 2.0 * PI )
#endif

layout(location = 0) out vec4 fragOutput;

layout(std140, binding = 0) uniform buf {
    mat4 viewMatrix;
    mat4 inverseProjection;
    mat4 orientation;
    float adjustY;
    float exposure;
} ubuf;

layout(location = 0) in vec3 eye_direction;
layout(binding = 1) uniform samplerCube skybox_image;

vec3 qt_linearTosRGB(vec3 color)
{
    vec3 S1 = sqrt(color);
    vec3 S2 = sqrt(S1);
    vec3 S3 = sqrt(S2);
    return 0.585122381 * S1 + 0.783140355 * S2 - 0.368262736 * S3;
}

#ifdef QSSG_ENABLE_ACES_TONEMAPPING
vec3 qt_toneMapACES(vec3 color)
{
const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;
    return qt_linearTosRGB(clamp((color * (A * color + B)) / (color * (C * color + D) + E), 0.0, 1.0));
}
#endif

#ifdef QSSG_ENABLE_HEJLDAWSON_TONEMAPPING
vec3 qt_tonemapHejlDawson(vec3 color)
{
    color = max(vec3(0.0), color - vec3(0.004));
    return (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);
}
#endif

#ifdef QSSG_ENABLE_FILMIC_TONEMAPPING
vec3 qt_toneMapFilmicSub(vec3 color)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((color * ( A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
}

vec3 qt_toneMapFilmic(vec3 color)
{
    color = qt_toneMapFilmicSub(color * 2.0);
    vec3 whiteScale = 1.0 / qt_toneMapFilmicSub(vec3(11.2, 11.2, 11.2));
    return qt_linearTosRGB(color * whiteScale);
}
#endif

// Call this method to perform tonemapping with the correct tonemapper
vec3 qt_tonemap(vec3 color)
{
    // ACES
#ifdef QSSG_ENABLE_ACES_TONEMAPPING
    return qt_toneMapACES(color);
#endif
    // Hejl-Dawson
#ifdef QSSG_ENABLE_HEJLDAWSON_TONEMAPPING
    return qt_tonemapHejlDawson(color);
#endif

    // FILMIC
#ifdef QSSG_ENABLE_FILMIC_TONEMAPPING
    return qt_toneMapFilmic(color);
#endif

    // Linear
#ifdef QSSG_ENABLE_LINEAR_TONEMAPPING
    return qt_linearTosRGB(color);
#endif

    return color;
}

void main()
{
    vec3 eye = normalize(eye_direction);
    vec4 color = textureLod(skybox_image, eye, 0.0);
#ifdef QSSG_ENABLE_RGBE_LIGHT_PROBE
    color = vec4(color.rgb * pow(2.0, color.a * 255.0 - 128.0), 1.0);
#endif

    // exposure
    vec3 exposureCorrectedColor = vec3(1.0) - exp(-color.rgb * ubuf.exposure);

    // tonemapping
    color = vec4(qt_tonemap(exposureCorrectedColor), 1.0);

    fragOutput = color;
}
