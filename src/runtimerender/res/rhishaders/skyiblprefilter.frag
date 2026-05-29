// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#version 440
layout(location = 0) out vec4 FragColor;
layout(location = 0) in vec3 localPos;

layout(binding = 1) uniform samplerCube environmentMap;
layout(std140, binding = 2) uniform buf {
    float roughness;
    float resolution;
    uint sampleStart;       // first Hammersley index in this slice
    uint sampleEnd;         // one-past-last Hammersley index in this slice
    uint totalSampleCount;  // denominator for the Hammersley sequence
    uint _pad0;
    uint _pad1;
    uint _pad2;
} ubuf2;

// Time-sliced variant of IBL prefiltering. Each invocation evaluates a
// contiguous range [sampleStart, sampleEnd) of the full Hammersley sequence
// of length totalSampleCount and writes the partial sums (pre-divided by the
// total) so the caller can additively blend slices into an accumulator and
// normalize once.

const float PI = 3.14159265359;

float radicalInverseVdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

// Returns a Hammersley point with the dimensions swapped relative to the standard
// form. The standard Hammersley uses (i/N, vdc(i)); the first component is linear
// in i, which maps to phi in importanceSampleGGX. Partial sums [0, K) of that
// sequence cover only phi in [0, 2π·K/N) — a single sector around the reflection
// direction — producing visible directional streaks during accumulation. Swapping
// to (vdc(i), i/N) makes phi follow the low-discrepancy radical-inverse sequence
// so each new sample lands in a different azimuth, smoothing the partial-sum
// appearance. The full N-sample point set is identical (the swap is a bijection
// when N is a power of two, and a valid 2D low-discrepancy sequence otherwise),
// so the converged integral matches.
vec2 hammersley(uint i, uint N)
{
    return vec2(radicalInverseVdC(i), float(i) / float(N));
}

vec3 importanceSampleGGX(vec2 xi, float a)
{
    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

float distributionGGX(float nDotH, float a)
{
    float a2 = a * a;
    float nDotH2 = nDotH * nDotH;
    float denom = nDotH2 * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom);
}

void main()
{
    vec3 N = normalize(localPos);

    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    mat3 tangentToWorld = mat3(tangent, bitangent, N);

    float a = ubuf2.roughness * ubuf2.roughness;
    float saTexel = 4.0 * PI / (6.0 * ubuf2.resolution * ubuf2.resolution);

    vec3 sumColor = vec3(0.0);
    float sumWeight = 0.0;
    for (uint i = ubuf2.sampleStart; i < ubuf2.sampleEnd; ++i) {
        vec2 xi = hammersley(i, ubuf2.totalSampleCount);
        vec3 H = importanceSampleGGX(xi, a);
        vec3 L = 2.0 * H.z * H - vec3(0.0, 0.0, 1.0);
        L = normalize(L);
        float NdotL = L.z;
        if (NdotL > 0.0) {
            float d = distributionGGX(H.z, a);
            float pdf = d / 4.0 + 0.0001;
            float saSample = 1.0 / (float(ubuf2.totalSampleCount) * pdf + 0.0001);
            float mipLevel = ubuf2.roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);
            vec3 Lworld = tangentToWorld * L;
            sumColor += textureLod(environmentMap, Lworld, mipLevel).rgb * NdotL;
            sumWeight += NdotL;
        }
    }
    // Pre-divide by the total so values stay bounded for additive blending
    // into an RGBA16F accumulator. After all slices have been blended,
    // dst.rgb / dst.a recovers the correct prefiltered radiance.
    float inv = 1.0 / float(ubuf2.totalSampleCount);
    FragColor = vec4(sumColor * inv, sumWeight * inv);
}
