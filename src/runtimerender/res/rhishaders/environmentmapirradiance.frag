#version 440
layout(location = 0) out vec4 FragColor;
layout(location = 0) in vec3 localPos;

layout(binding = 1) uniform samplerCube environmentMap;

const float M_PI = 3.14159265359;

#ifdef QSSG_ENABLE_RGBE_LIGHT_PROBE
vec4 decodeRGBE(in vec4 rgbe)
{
    float f = pow(2.0, 255.0 * rgbe.a - 128.0);
    return vec4(rgbe.rgb * f, 1.0);
}

vec4 encodeRGBE(in vec4 rgba)
{
    float maxMan = max(rgba.r, max(rgba.g, rgba.b));
    float maxExp = 1.0 + floor(log2(maxMan));
    return vec4(rgba.rgb / pow(2.0, maxExp), (maxExp + 128.0) / 255.0);
}
#endif

// This implements the irradiance convolution described here:
// https://learnopengl.com/PBR/IBL/Diffuse-irradiance
void main()
{
    // the sample direction equals the hemisphere's orientation
    vec3 normal = normalize(localPos);

    vec3 irradiance = vec3(0.0);

    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = cross(up, normal);
    up         = cross(normal, right);

    float sampleDelta = 0.1;
    float nrSamples = 0.0;
    for (float phi = 0.0; phi < 2.0 * M_PI; phi += sampleDelta)
    {
        for (float theta = 0.0; theta < 0.5 * M_PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;
            vec4 sampleValue = textureLod(environmentMap, sampleVec, 0);
#ifdef QSSG_ENABLE_RGBE_LIGHT_PROBE
            sampleValue = decodeRGBE(sampleValue);
#endif
            irradiance += sampleValue.rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }

    irradiance = M_PI * irradiance * (1.0 / float(nrSamples));
#ifdef QSSG_ENABLE_RGBE_LIGHT_PROBE
    FragColor = encodeRGBE(vec4(irradiance, 1.0));
#else
    FragColor = vec4(irradiance, 1.0);
#endif
}
