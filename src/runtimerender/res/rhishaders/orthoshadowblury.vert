// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#version 440

layout(location = 0) in vec3 attr_pos;
layout(location = 1) in vec2 attr_uv;

layout(location = 0) out vec2 uv_coords;

layout(std140, binding = 0) uniform buf {
    mat4 matrix;
    vec2 cameraProperties;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    gl_Position = ubuf.matrix * vec4(attr_pos, 1.0);
    uv_coords.xy = attr_uv.xy;
}
