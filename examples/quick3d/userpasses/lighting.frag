// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

VARYING vec2 texcoord;
#if QSSG_ENABLE_ORTHO_SHADOW_PASS
VARYING float var_depth;
#endif

#if !(QSSG_ENABLE_DEPTH_PASS || QSSG_ENABLE_ORTHO_SHADOW_PASS || QSSG_ENABLE_PERSPECTIVE_SHADOW_PASS || QSSG_ENABLE_OPAQUE_DEPTH_PRE_PASS)
#define LIGHTING_PASS
#endif

#if defined (LIGHTING_PASS)
#include "lightsData.glsllib"
#include "funcprocessPunctualLighting.glsllib"
#include "tonemapping.glsllib"

#if QSSG_ENABLE_LIGHT_PROBE
#include "sampleProbe.glsllib"
#endif
#endif // LIGHTING_PASS

// Helper to decode normal from 0..1 back to -1..1
vec3 decodeNormal(vec3 enc)
{
    return normalize(enc * 2.0 - 1.0);
}

// Simple Fresnel F0 helper
vec3 computeF0(vec3 baseColor, float metalness)
{
    // 4 % default specular for dielectrics
    vec3 dielectricF0 = vec3(0.04);
    return mix(dielectricF0, baseColor, metalness);
}

void MAIN()
{
#if defined (LIGHTING_PASS)
    vec2 uv = texcoord;
    if (FRAMEBUFFER_Y_UP < 0.0)
        uv.y = 1.0 - uv.y;

    // Sample G-buffer
    vec4 gb0 = texture(gbuffer0, uv);
    vec4 gb1 = texture(gbuffer1, uv);
    vec4 gb2 = texture(gbuffer2, uv);

    vec3 baseColor  = gb0.rgb;
    float metalness = gb0.a;

    vec3 N = decodeNormal(gb1.rgb);
    float roughness = gb1.a;

    vec3 worldPos = gb2.xyz;

    // View vector (from fragment to camera)
    vec3 V = normalize(CAMERA_POSITION - worldPos);

    vec3 diffuseAccum = vec3(0.0);
    vec3 specAccum    = vec3(0.0);

    vec3 F0 = computeF0(baseColor, metalness);

    qt_processPunctualLighting(diffuseAccum,
                               specAccum,
                               baseColor,
                               worldPos,
                               N,
                               V,
                               vec3(1.0), // specularAmount
                               vec3(1.0), // specularTint
                               roughness,
                               metalness,
                               F0,
                               vec3(1.0)  // F90
                               );

#if QSSG_ENABLE_LIGHT_PROBE
    vec4 probeDiffuse = vec4(baseColor, 1.0) * qt_sampleDiffuse(N);
    vec4 probeSpecular = qt_sampleGlossyPrincipled(N, V, F0, roughness);
    diffuseAccum += probeDiffuse.rgb;
    specAccum += probeSpecular.rgb;
#endif


    vec3 color = diffuseAccum + specAccum;

    // Clamp a bit so you do not get wild values before you add tone mapping
    color = max(color, vec3(0.0));

    FRAGCOLOR = vec4(qt_tonemap(color), 1.0);
#else
#if QSSG_ENABLE_PERSPECTIVE_SHADOW_PASS
    // Perspective shadow pass
    vec3 qt_shadowCamPos = vec3(qt_cameraPosition.x, qt_cameraPosition.y, qt_cameraPosition.z);
    float qt_shadowDist = length(var_world_position - qt_shadowCamPos);
    qt_shadowDist = (qt_shadowDist - qt_cameraProperties.x) / (qt_cameraProperties.y - qt_cameraProperties.x);
    FRAGCOLOR = vec4(qt_shadowDist, qt_shadowDist, qt_shadowDist, 1.0);
#elif QSSG_ENABLE_ORTHO_SHADOW_PASS
    // Ortho Shadow pass (directional)
    float qt_shadowDepth = (var_depth + qt_shadowDepthAdjust.x) * qt_shadowDepthAdjust.y;
    FRAGCOLOR = vec4(qt_shadowDepth);
#else
    FRAGCOLOR = vec4(1.0, 0, 0, 1);
#endif
#endif // LIGHTING_PASS

}
