// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#version 440

layout(location = 0) in vec3 attr_pos;

layout(location = 0) out vec3 tex_coords;

layout(std140, binding = 0) uniform buf {
    mat3 viewMatrix;
    mat4 inverseProjection;
    mat3 orientation;
    vec4 skyboxProperties;
    mat4 viewProjection;
} ubuf;

out gl_PerVertex { vec4 gl_Position; };

void main()
{
    //mirror texture lookup since we're looking from the inside of the cube
    tex_coords = attr_pos * vec3(-1, 1, 1);

    vec4 pos = ubuf.viewProjection * vec4(attr_pos, 1.0);
    pos.y *= ubuf.skyboxProperties.x;
    gl_Position = pos.xyww;
}
