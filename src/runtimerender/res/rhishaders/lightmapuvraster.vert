// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#version 440

layout(location = 0) in vec3 attr_pos;
layout(location = 1) in vec3 attr_normal;
layout(location = 2) in vec2 attr_lightmap_uv;
#if defined(QSSG_LIGHTMAPUVRASTER_UV) || defined(QSSG_LIGHTMAPUVRASTER_UV_TANGENT)
layout(location = 3) in vec2 attr_uv;
#endif
#if defined(QSSG_LIGHTMAPUVRASTER_UV_TANGENT)
layout(location = 4) in vec3 attr_tangent;
layout(location = 5) in vec3 attr_binormal;
#endif

layout(location = 0) out vec3 v_pos;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec2 v_uv;
layout(location = 3) out vec3 v_tangent;
layout(location = 4) out vec3 v_binormal;

layout(std140, binding = 0) uniform buf {
    vec4 baseColorLinear;
    vec3 emissiveFactor;
    int flipY;
    int hasBaseColorMap;
    int hasEmissiveMap;
    int hasNormalMap;
    float bumpAmount;
};

void main()
{
    v_pos = attr_pos;
    v_normal = attr_normal;
#if defined(QSSG_LIGHTMAPUVRASTER_UV) || defined(QSSG_LIGHTMAPUVRASTER_UV_TANGENT)
    v_uv = attr_uv;
#else
    v_uv = vec2(0.0);
#endif
#if defined(QSSG_LIGHTMAPUVRASTER_UV_TANGENT)
    v_tangent = attr_tangent;
    v_binormal = attr_binormal;
#else
    v_tangent = vec3(0.0);
    v_binormal = vec3(0.0);
#endif

    vec2 uv = vec2(attr_lightmap_uv.x * 2.0 - 1.0, attr_lightmap_uv.y * 2.0 - 1.0);
    if (flipY != 0)
        uv.y *= -1.0;

    gl_Position = vec4(uv, 0.0, 1.0);
}
