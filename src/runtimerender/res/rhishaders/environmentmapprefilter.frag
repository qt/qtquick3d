#version 440
layout(location = 0) out vec4 FragColor;
layout(location = 0) in vec3 localPos;

layout(binding = 1) uniform samplerCube environmentMap;
layout(std140, binding = 2) uniform buf {
    float roughness;
    float resolution;
} ubuf2;

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

float radicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
// ----------------------------------------------------------------------------
vec2 hammersley(uint i, uint N)
{
    return vec2(float(i) / float(N), radicalInverse_VdC(i));
}

vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness*roughness;

    float phi = 2.0 * M_PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}
#ifndef QSSG_ENABLE_RGBE_LIGHT_PROBE
float distributionGGX(float NdotH, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH2 = NdotH * NdotH;

  float nom = a2;
  float denom = NdotH2 * (a2 - 1.0) + 1.0;
  denom = M_PI * denom * denom;

  return nom / denom;
}
#endif

// This implements the pre-filtering technique described here:
// https://learnopengl.com/PBR/IBL/Specular-IBL
void main()
{
    vec3 N = normalize(localPos);
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    float totalWeight = 0.0;
    vec3 prefilteredColor = vec3(0.0);
    for(uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = hammersley(i, SAMPLE_COUNT);
        vec3 H  = importanceSampleGGX(Xi, N, ubuf2.roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
#ifndef QSSG_ENABLE_RGBE_LIGHT_PROBE
            float NdotH = max(dot(N, H), 0.0);
            float D = distributionGGX(NdotH, ubuf2.roughness);
            float HdotV = max(dot(H, V), 0.0);
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;

            float saTexel = 4.0 * M_PI / (6.0 * ubuf2.resolution * ubuf2.resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);
            float mipLevel = ubuf2.roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);
#endif
#ifdef QSSG_ENABLE_RGBE_LIGHT_PROBE
            prefilteredColor += decodeRGBE(texture(environmentMap, L)).rgb * NdotL;
#else
            prefilteredColor += textureLod(environmentMap, L, mipLevel).rgb * NdotL;
#endif
            totalWeight      += NdotL;
        }
    }
    prefilteredColor = prefilteredColor / totalWeight;
#ifdef QSSG_ENABLE_RGBE_LIGHT_PROBE
    FragColor = encodeRGBE(vec4(prefilteredColor, 1.0));
#else
    FragColor = vec4(prefilteredColor, 1.0);
#endif
}
