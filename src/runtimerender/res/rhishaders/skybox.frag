#version 440

#ifndef PI
#define PI          3.14159265358979
#define PI_HALF     ( 0.5 * PI )
#define PI_TWO      ( 2.0 * PI )
#endif

layout(location = 0) out vec4 fragOutput;

layout(std140, binding = 0) uniform buf {
    mat4 viewMatrix;
    mat4 projection;
    float adjustY;
} ubuf;

layout(location = 0) in vec3 eye_direction;
layout(binding = 1) uniform sampler2D skybox_image;

void main()
{
    // nlerp direction vector, not entirely correct, but simple/efficient
    vec3 eye = normalize(eye_direction);

    // Equirectangular textures project longitude and latitude to the xy plane
    float longitude = atan(eye.x, eye.z) / PI_TWO + 0.5;
    float latitude = asin(eye.y) / PI + 0.5;
    vec2 uv = vec2(longitude, latitude);

    // Because of the non-standard projection, the texture lookup for normal texture
    // filtering is messed up.
    // TODO: Alternatively, we could check if it's possible to disable some of the texture
    // filtering just for the skybox part.
    vec4 color = textureLod(skybox_image, uv, 0.0);
    vec3 rgbeColor = color.rgb * pow(2.0, color.a * 255.0 - 128.0);
    vec3 tonemappedColor = rgbeColor.rgb / (rgbeColor.rgb + vec3(1.0));
    vec3 gammaCorrectedColor = pow( tonemappedColor, vec3( 1.0 / 2.2 ));
    fragOutput = vec4(gammaCorrectedColor, 1.0);
}
