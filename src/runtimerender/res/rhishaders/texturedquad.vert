// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#version 440

layout(location = 0) in vec3 attr_pos;
layout(location = 1) in vec2 attr_uv;

layout(location = 0) out vec2 uv_coord;

layout(std140, binding = 0) uniform buf {
    mat4 modelViewProjection;
    vec2 layerDimensions;
    float opacity;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    vec3 layerPos = vec3(attr_pos.x * ubuf.layerDimensions.x / 2.0,
                         attr_pos.y * ubuf.layerDimensions.y / 2.0,
                         attr_pos.z);
    gl_Position = ubuf.modelViewProjection * vec4(layerPos, 1.0);

    // always flip UV.y, this, combined with the clipSpaceCorrMatrix() in
    // gl_ModelViewProjectionMatrix, gives correct results with all APIs
    uv_coord = vec2(attr_uv.x, 1.0 - attr_uv.y);
}
