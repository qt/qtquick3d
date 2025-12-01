// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

VARYING vec2 texcoord;
#if QSSG_ENABLE_ORTHO_SHADOW_PASS
VARYING float var_depth;
#endif
#if QSSG_ENABLE_PERSPECTIVE_SHADOW_PASS
VARYING vec3 var_world_position;
#endif

void MAIN()
{
    texcoord = UV0;
    // POSITION = MODELVIEWPROJECTION_MATRIX * vec4(VERTEX, 1.0);
    vec2 ndc = UV0 * 2.0 - 1.0;
    POSITION = vec4(ndc.x, ndc.y, 0.0, 1.0);
#if QSSG_ENABLE_ORTHO_SHADOW_PASS
    var_depth = gl_Position.z / gl_Position.w;
#endif
#if QSSG_ENABLE_PERSPECTIVE_SHADOW_PASS
    var_world_position = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
#endif
}
